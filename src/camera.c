#pragma save
#pragma stackauto

#include "camera.h"
#include "i2c_device.h"
#include "global.h"
#include "i2c.h"
#include "print.h"
#include "msp_displayport.h"
#include "isr.h"
#include "hardware.h"

uint8_t cameraID = 0;
uint8_t camProfile_EEP;
uint8_t camProfile;
uint8_t camProfile_Menu;
cameraConfig_t camCfg;
cameraConfig_t camCfg_Menu;
cameraConfig_t camCfg_Cur;
cameraConfig_t camCfg_EEP[RUNCAM_PROFILE_M_TYPE];

// {brightness, sharpness, saturation, contrast, hvFlip, nightMode, wbMode,
//  wbRed[0], wbRed[1], wbRed[2], wbRed[3],
//  wbBlue[0], wbBlue[1], wbBlue[2], wbBlue[3],}
const uint8_t camParameterInit[RUNCAM_PROFILE_M_TYPE][15] = 
{
    //Micro V1
    {0x80, 0x01, 0x03, 0x01, 0x00, 0x01, 0x00,
    0xC7, 0xC7, 0xC7, 0xC7,
    0xCA, 0xCA, 0xCA, 0xCA},
    
    //Micro V2
    {0x80, 0x01, 0x05, 0x01, 0x00, 0x01, 0x00,
    0xC7, 0xC7, 0xC7, 0xC7,
    0xCA, 0xCA, 0xCA, 0xCA,},
    
    //Nano V2
    {0x80, 0x02, 0x05, 0x01, 0x00, 0x01, 0x00,
    0xC7, 0xC7, 0xC7, 0xC7,
    0xCA, 0xCA, 0xCA, 0xCA,},
    
    //Nano Lite
    {0x80, 0x02, 0x05, 0x02, 0x00, 0x01, 0x00,
    0xC7, 0xC7, 0xC7, 0xC7,
    0xCA, 0xCA, 0xCA, 0xCA,},
};

uint8_t CAM_MODE = CAM_720P60;
/*
RUNCAM_PROFILE_M_TYPE==>
    0: Micro V1 M
    1: Micro V2 M
    2: Nano V2 M
    3: Nano lite M
*/
uint8_t cam_4_3 = 0;
uint8_t camMenuStatus = CAM_STATUS_IDLE;
uint8_t bw_changed = 0;

void camMenuStringUpdate(uint8_t status);
void camMenuSetVdoRatioInit();

uint8_t CamDetect()
{
    int cycles = 4;
    int fps = CAM_720P60;
    int loss = 0;
    
    WriteReg(0, 0x8F, 0x91);

    while(cycles){
        if(fps == CAM_720P50){
            Init_TC3587();
            Set_720P50(IS_RX);
            //debugf("\r\nCamDetect: Set 50fps.");
        }
        else if(fps == CAM_720P60){
            Init_TC3587();
            Set_720P60(IS_RX);
            //debugf("\r\nCamDetect: Set 60fps.");
        }
		
        WAIT(100);
        if((ReadReg(0, 0x02) >> 4))
            loss = 1;
        WAIT(5);
        if((ReadReg(0, 0x02) >> 4))
            loss = 1;
        WAIT(5);
        if((ReadReg(0, 0x02) >> 4))
            loss = 1;
        WAIT(5);
        if((ReadReg(0, 0x02) >> 4))
            loss = 1;
        WAIT(5);
        if((ReadReg(0, 0x02) >> 4))
            loss = 1;
        
        if(loss == 0)
            break;
        
        fps = (fps == CAM_720P60) ? CAM_720P50 : CAM_720P60;

        loss = 0;
        cycles--;
    }
    
    if(cycles == 0){
        fps = CAM_720P60_NEW;
        Set_720P60(IS_RX);
        I2C_Write(ADDR_TC3587, 0x0058, 0x00e0, 1, 1);
        if(!RUNCAM_Write(RUNCAM_MICRO_V1,0x50, 0x0452484E))
            cameraID = RUNCAM_MICRO_V1;
        else if(!RUNCAM_Write(RUNCAM_MICRO_V2,0x50, 0x0452484E))
            cameraID = RUNCAM_MICRO_V2;
        else
            cameraID = 0;
        
        #ifdef _DEBUG_MODE
        debugf("\r\ncameraID: %x", (uint16_t)cameraID);
        #endif
    }

    return fps;
}

void Cam_Button_INIT()
{
    WriteReg(0, 0x17, 0xC0);
    WriteReg(0, 0x16, 0x00);
    WriteReg(0, 0x15, 0x02);
    WriteReg(0, 0x14, 0x00);
}

/////////////////////////////////////////////////////////////////
// runcam config
void Runcam_SetBrightness(uint8_t val)
{
    uint32_t d = 0x04004800;
    uint32_t val_32;

    camCfg_Cur.brightness = val;

    if(cameraID == RUNCAM_MICRO_V1)
        d = 0x0452484e;
    else if(cameraID == RUNCAM_MICRO_V2)
        d = 0x04504850;
    
    val_32 = (uint32_t)val;
    
    //indoor
    d += val_32;
    d -= CAM_BRIGHTNESS_INITIAL;
    
    //outdoor
    d += (val_32 << 16);
    d -= ((uint32_t)CAM_BRIGHTNESS_INITIAL << 16);
    
    RUNCAM_Write(cameraID, 0x50, d);
    #ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM brightness:%x", (uint16_t)val);
    #endif
}

void Runcam_SetSharpness(uint8_t val)
{
    uint32_t d = 0;

    camCfg_Cur.sharpness = val;

    if(cameraID == RUNCAM_MICRO_V1)
        d = 0x03FF0100;
    else if(cameraID == RUNCAM_MICRO_V2)
        d = 0x03FF0000;
    
    if(val == 2){
        RUNCAM_Write(cameraID, 0x0003C4, 0x03FF0000);
        RUNCAM_Write(cameraID, 0x0003CC, 0x28303840);
        RUNCAM_Write(cameraID, 0x0003D8, 0x28303840);
    }else if(val == 1){
        RUNCAM_Write(cameraID, 0x0003C4, 0x03FF0000);
        RUNCAM_Write(cameraID, 0x0003CC, 0x14181C20);
        RUNCAM_Write(cameraID, 0x0003D8, 0x14181C20);
    }else if(val == 0){
        RUNCAM_Write(cameraID, 0x0003C4, d         );
        RUNCAM_Write(cameraID, 0x0003CC, 0x0A0C0E10);
        RUNCAM_Write(cameraID, 0x0003D8, 0x0A0C0E10);
    }
    #ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM sharpness:%x", (uint16_t)val);
    #endif
}

uint8_t Runcam_SetSaturation(uint8_t val)
{
    uint8_t ret = 1;
    uint32_t d = 0x20242626;
    

    camCfg_Cur.saturation = val;

    //initial
    if(cameraID == RUNCAM_MICRO_V1)
        d = 0x20242626;
    else if(cameraID == RUNCAM_MICRO_V2)
        d = 0x24282c30;
    
    if(val == 0)
        d = 0x00000000;
    else if(val == 1)
        d -= 0x18181414;
    else if(val == 2)
        d -= 0x14141010;
    else if(val == 3)
        d -= 0x0c0c0808;
    else if(val == 4)//low
        d -= 0x08080404;
    else if(val == 5)//normal
        ;
    else if(val == 6) //high
        d += 0x04041418;
    #ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM saturation:%x", (uint16_t)val);
    #endif
    
    ret = RUNCAM_Write(cameraID, 0x0003A4, d);
    return ret;
}

void Runcam_SetContrast(uint8_t val)
{
    uint32_t d = 0x46484A4C;
    
     camCfg_Cur.contrast = val;

    if(cameraID == RUNCAM_MICRO_V1)
        d = 0x46484A4C;
    else if(cameraID == RUNCAM_MICRO_V2)
        d = 0x36383a3c;
    
    if(val==0)//low
        d -= 0x06040404;
    else if(val == 1)//normal
        ;
    else if(val == 2) //high
        d += 0x04040404;
    
    RUNCAM_Write(cameraID, 0x00038C, d);
    #ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM contrast:%x", (uint16_t)val);
    #endif
}

void Runcam_SetVdoRatio(uint8_t ratio)
{
/*
    0: 720p60 4:3 default
    1: 720p60 16:9 crop
    2: 720p30 16:9 fill
*/
    if(cameraID != RUNCAM_MICRO_V2)
        return;
    if(ratio == 0)
        RUNCAM_Write(cameraID, 0x000008, 0x0008910B);
    else if(ratio == 1)
        RUNCAM_Write(cameraID, 0x000008, 0x00089102);
    else if(ratio == 2)
        RUNCAM_Write(cameraID, 0x000008, 0x00089110);

    //save
    RUNCAM_Write(cameraID, 0x000694, 0x00000310);
    RUNCAM_Write(cameraID, 0x000694, 0x00000311);
    #ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM VdoRatio:%x", (uint16_t)ratio);
    #endif
}

#if(0)
uint8_t Runcam_GetVdoFormat(uint8_t cameraID)
{
/*
    0: 720p60 16:9
    0: 720p60 4:3
    1: 720p30 16:9
    1: 720p30 4:3
*/
    uint32_t val;
    uint8_t ret = 2;
    
    if(cameraID != RUNCAM_MICRO_V2)
        return ret;
    
    while(ret == 2){
        val = RUNCAM_Read(cameraID, 0x000008);
        if(val == 0x00089102){              //720p60  16:9
            ret = 0;
            cam_4_3 = 0;
        }else if(val == 0x0008910B){        //720p60  4:3
            ret = 0;
            cam_4_3 = 1;
        }else if(val == 0x00089104){        //720p30 16:9
            ret = 1;
            cam_4_3 = 0;
        }else if(val == 0x00089109){        //720p60 4:3
            ret = 1;
            cam_4_3 = 1;
        }else{
            ret = 2;
            cam_4_3 = 0;
        }
        WAIT(100);
        LED_BLUE_OFF;
        WAIT(100);
        LED_BLUE_ON;
        led_status = ON;
    }
    
    #ifdef _DEBUG_CAMERA
    debugf("\r\nVdoFormat:%x, 4_3:%x", (uint16_t)ret, (uint16_t)cam_4_3);
    #endif
    return ret;
}
#endif
void Runcam_SetHVFlip(uint8_t val)
{
    if(cameraID != RUNCAM_MICRO_V2)
        return;
    
    camCfg_Cur.hvFlip = val;

    if(val == 0)
        RUNCAM_Write(cameraID, 0x000040, 0x0022ffa9);
    else if(val==1)
        RUNCAM_Write(cameraID, 0x000040, 0x002effa9);
}

void Runcam_SetNightMode(uint8_t val)
{
/*
    0: night mode off
    1: night mode on
*/
    if(cameraID != RUNCAM_MICRO_V2)
        return;

    camCfg_Cur.nightMode = val;

    if(val == 1){  //Max gain on
        RUNCAM_Write(cameraID, 0x000070, 0x10000040);
        //RUNCAM_Write(cameraID, 0x000070, 0x04000040);
        RUNCAM_Write(cameraID, 0x000718, 0x28002700);
        RUNCAM_Write(cameraID, 0x00071c, 0x29002800);
        RUNCAM_Write(cameraID, 0x000720, 0x29002900);
    }else if(val == 0){ //Max gain off
        RUNCAM_Write(cameraID, 0x000070, 0x10000040);
        //RUNCAM_Write(cameraID, 0x000070, 0x04000040);
        RUNCAM_Write(cameraID, 0x000718, 0x30002900);
        RUNCAM_Write(cameraID, 0x00071c, 0x32003100);
        RUNCAM_Write(cameraID, 0x000720, 0x34003300);
    }
    
    //save
    RUNCAM_Write(cameraID, 0x000694, 0x00000310);
    RUNCAM_Write(cameraID, 0x000694, 0x00000311);
    #ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM NightMode:%x", (uint16_t)val);
    #endif
}
void Runcam_SetWB(uint8_t* wbRed, uint8_t* wbBlue, uint8_t wbMode)
{
    uint32_t wbRed_u32  = 0x02000000;
    uint32_t wbBlue_u32 = 0x00000000;
    uint8_t i;
    
    camCfg_Cur.wbMode = wbMode;
    for(i=0;i<WBMODE_MAX;i++)
    {
        camCfg_Cur.wbRed[i] = wbRed[i];
        camCfg_Cur.wbBlue[i] = wbBlue[i];
    }

    if(wbMode){
        wbRed_u32 += ((uint32_t)wbRed[wbMode-1] << 2);
        wbBlue_u32 += ((uint32_t)wbBlue[wbMode-1] << 2);
        wbRed_u32 ++;
        wbBlue_u32 ++;
    }

    if(wbMode){//MWB
        RUNCAM_Write(cameraID, 0x0001b8, 0x020b007b);
        RUNCAM_Write(cameraID, 0x000204, wbRed_u32);
        RUNCAM_Write(cameraID, 0x000208, wbBlue_u32);
    }else{//AWB
        RUNCAM_Write(cameraID, 0x0001b8, 0x020b0079);
    }
}

void CheckCameraParameter(void)
{
    uint8_t i, j;

    if(cameraID == 0)
        return;

    if((camProfile_EEP & 0x0F) > 1)
    {
        camProfile_EEP = (camProfile_EEP & 0xF0);
        WAIT(10);I2C_Write(ADDR_EEPROM, EEP_ADDR_CAM_PROFILE, camProfile_EEP, 0, 0);
    }

    if(((camProfile_EEP>>4) & 0xF) > 5)
    {
        camProfile_EEP = camProfile_EEP & 0x01;
        WAIT(10);I2C_Write(ADDR_EEPROM, EEP_ADDR_CAM_PROFILE, camProfile_EEP, 0, 0);
    }

    for(i=0;i<RUNCAM_PROFILE_M_TYPE; i++)
    {
        if(camCfg_EEP[i].brightness < BRIGHTNESS_MIN || camCfg_EEP[i].brightness > BRIGHTNESS_MIN)
        {
            camCfg_EEP[i].brightness = camParameterInit[i][0];
            WAIT(10);I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_BRIGHTNESS, camCfg_EEP[i].brightness, 0, 0);
        }
        
        if(camCfg_EEP[i].sharpness > SHARPNESS_MAX)
        {
            camCfg_EEP[i].sharpness = camParameterInit[i][1];
            WAIT(10);I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_SHARPNESS, camCfg_EEP[i].sharpness, 0, 0);
        }

        if(camCfg_EEP[i].saturation > SATURATION_MAX)
        {
            camCfg_EEP[i].saturation = camParameterInit[i][2];
            WAIT(10);I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_SATURATION, camCfg_EEP[i].saturation, 0, 0);
        }

        if(camCfg_EEP[i].contrast > CONTRAST_MAX)
        {
            camCfg_EEP[i].contrast = camParameterInit[i][3];
            WAIT(10);I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_CONTRAST, camCfg_EEP[i].contrast, 0, 0);
        }
        
        if(camCfg_EEP[i].hvFlip > HVFLIP_MAX)
        {
            camCfg_EEP[i].hvFlip = camParameterInit[i][4];
            WAIT(10);I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_HVFLIP, camCfg_EEP[i].hvFlip, 0, 0);
        }

        if(camCfg_EEP[i].nightMode > NIGHTMODE_MAX)
        {
            camCfg_EEP[i].nightMode = camParameterInit[i][5];
            WAIT(10);I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_NIGHTMODE, camCfg_EEP[i].nightMode, 0, 0);
        }

        if(camCfg_EEP[i].wbMode > WBMODE_MAX)
        {
            camCfg_EEP[i].wbMode = camParameterInit[i][6];
            WAIT(10);I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_WBMODE, camCfg_EEP[i].wbMode, 0, 0);

            for(j=0;j<WBMODE_MAX;j++)
            {
                camCfg_EEP[i].wbRed[j] = camParameterInit[i][7+j];
                WAIT(10);I2C_Write(ADDR_EEPROM, (i<<4)+j+EEP_ADDR_CAM_WBRED, camCfg_EEP[i].wbRed[j], 0, 0);
                camCfg_EEP[i].wbBlue[j] = camParameterInit[i][11+j];
                WAIT(10);I2C_Write(ADDR_EEPROM, (i<<4)+j+EEP_ADDR_CAM_WBBLUE, camCfg_EEP[i].wbBlue[j], 0, 0);
            }
        }
    }
}

void SaveCameraParameter(void)
{
    uint8_t i,j;

    CheckCameraParameter();

    for(i=0;i<RUNCAM_PROFILE_M_TYPE;i++)
    {
        WAIT(10); I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_BRIGHTNESS, camCfg_EEP[i].brightness, 0, 0);
        WAIT(10); I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_SHARPNESS, camCfg_EEP[i].sharpness, 0, 0);
        WAIT(10); I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_SATURATION, camCfg_EEP[i].saturation, 0, 0);
        WAIT(10); I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_CONTRAST,camCfg_EEP[i].contrast, 0, 0);
        WAIT(10); I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_HVFLIP, camCfg_EEP[i].hvFlip, 0, 0);
        WAIT(10); I2C_Write(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_NIGHTMODE, camCfg_EEP[i].nightMode, 0, 0);
        WAIT(10); I2C_Write(ADDR_EEPROM, EEP_ADDR_CAM_WBMODE+(i<<4), camCfg_EEP[i].wbMode, 0, 0);
        for(j=0;j<WBMODE_MAX;j++)
        {
            WAIT(10); I2C_Write(ADDR_EEPROM, (i<<4)+j+EEP_ADDR_CAM_WBRED, camCfg_EEP[i].wbRed[j], 0, 0);
            WAIT(10); I2C_Write(ADDR_EEPROM, (i<<4)+j+EEP_ADDR_CAM_WBBLUE, camCfg_EEP[i].wbBlue[j], 0, 0);
        }
    }
}

void GetCamCfg_EEP(void)
{
    uint8_t i, j;

    if(cameraID == 0)
        return;

    else if(cameraID == RUNCAM_MICRO_V1 || cameraID == RUNCAM_MICRO_V2)
    {
        WAIT(10); camProfile_EEP = I2C_Read(ADDR_EEPROM, EEP_ADDR_CAM_PROFILE, 0, 0);
        for(i=0;i<RUNCAM_PROFILE_M_TYPE;i++)
        {
            WAIT(10); camCfg_EEP[i].brightness = I2C_Read(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_BRIGHTNESS, 0, 0);
            WAIT(10); camCfg_EEP[i].sharpness  = I2C_Read(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_SHARPNESS, 0, 0);
            WAIT(10); camCfg_EEP[i].saturation = I2C_Read(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_SATURATION, 0, 0);
            WAIT(10); camCfg_EEP[i].contrast   = I2C_Read(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_CONTRAST, 0, 0);
            WAIT(10); camCfg_EEP[i].hvFlip     = I2C_Read(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_HVFLIP, 0, 0);
            WAIT(10); camCfg_EEP[i].nightMode  = I2C_Read(ADDR_EEPROM, (i<<4)+EEP_ADDR_CAM_NIGHTMODE, 0, 0);
            WAIT(10); camCfg_EEP[i].wbMode     = I2C_Read(ADDR_EEPROM, EEP_ADDR_CAM_WBMODE+(i<<4), 0, 0);
            for(j=0;j<WBMODE_MAX;j++)
            {
                WAIT(10); camCfg_EEP[i].wbRed[j]  = I2C_Read(ADDR_EEPROM, (i<<4)+j+EEP_ADDR_CAM_WBRED, 0, 0);
                WAIT(10); camCfg_EEP[i].wbBlue[j] = I2C_Read(ADDR_EEPROM, (i<<4)+j+EEP_ADDR_CAM_WBBLUE, 0, 0);
            }
        }
    }

    CheckCameraParameter();
}

void GetCamCfg(uint8_t USE_EEP_PROFILE)
{
    uint8_t i;

    if(cameraID == 0)
        return;

    if(cameraID == RUNCAM_MICRO_V1)
    {
        if(USE_EEP_PROFILE)
            camProfile = camProfile_EEP & 0x0f;
        if(camProfile == 0) //Micro V1 default setting
        {
            camCfg.brightness = camParameterInit[0][0];
            camCfg.sharpness = camParameterInit[0][1];
            camCfg.saturation = camParameterInit[0][2];
            camCfg.contrast = camParameterInit[0][3];
            camCfg.hvFlip = camParameterInit[0][4];
            camCfg.nightMode = camParameterInit[0][5];
            camCfg.wbMode = camParameterInit[0][6];
            for(i=0;i<WBMODE_MAX;i++)
            {
                camCfg.wbRed[i] = camParameterInit[0][7+i];
                camCfg.wbBlue[i] = camParameterInit[0][11+i];
            }
        }
        else //Micro V1 manual setting
        {
            camCfg.brightness = camCfg_EEP[0].brightness;
            camCfg.sharpness = camCfg_EEP[0].sharpness;
            camCfg.saturation = camCfg_EEP[0].saturation;
            camCfg.contrast = camCfg_EEP[0].contrast;
            camCfg.hvFlip = camCfg_EEP[0].hvFlip;
            camCfg.nightMode = camCfg_EEP[0].nightMode;
            camCfg.wbMode = camCfg_EEP[0].wbMode;
            for(i=0;i<WBMODE_MAX;i++)
            {
                camCfg.wbRed[i] = camCfg_EEP[0].wbRed[i];
                camCfg.wbBlue[i] = camCfg_EEP[0].wbBlue[i];
            }
        }
    }
    else if(cameraID == RUNCAM_MICRO_V2)
    {
        if(USE_EEP_PROFILE)
            camProfile = camProfile_EEP >> 4;
        if(camProfile <= Profile_NanoLite_Auto)
        {
            camCfg.brightness = camParameterInit[1+camProfile][0];
            camCfg.sharpness = camParameterInit[1+camProfile][1];
            camCfg.saturation = camParameterInit[1+camProfile][2];
            camCfg.contrast = camParameterInit[1+camProfile][3];
            camCfg.hvFlip = camParameterInit[1+camProfile][4];
            camCfg.nightMode = camParameterInit[1+camProfile][5];
            camCfg.wbMode = camParameterInit[1+camProfile][6];
            for(i=0;i<WBMODE_MAX;i++)
            {
                camCfg.wbRed[i] = camParameterInit[1+camProfile][7+i];
                camCfg.wbBlue[i] = camParameterInit[1+camProfile][11+i];
            }
        }
        else if(camProfile <= Profile_NanoLite_Manual)
        {
            camCfg.brightness = camCfg_EEP[camProfile-2].brightness;
            camCfg.sharpness = camCfg_EEP[camProfile-2].sharpness;
            camCfg.saturation = camCfg_EEP[camProfile-2].saturation;
            camCfg.contrast = camCfg_EEP[camProfile-2].contrast;
            camCfg.hvFlip = camCfg_EEP[camProfile-2].hvFlip;
            camCfg.nightMode = camCfg_EEP[camProfile-2].nightMode;
            camCfg.wbMode = camCfg_EEP[camProfile-2].wbMode;
            for(i=0;i<WBMODE_MAX;i++)
            {
                camCfg.wbRed[i] = camCfg_EEP[camProfile-2].wbRed[i];
                camCfg.wbBlue[i] = camCfg_EEP[camProfile-2].wbBlue[i];
            }
        }
    }

    #ifdef _DEBUG_CAMERA
    debugf("\r\n    profile:    %x", (uint16_t)camProfile);
    debugf("\r\n    brightness: %x", (uint16_t)camCfg.brightness);
    debugf("\r\n    sharpness:  %x", (uint16_t)camCfg.sharpness);
    debugf("\r\n    saturation: %x", (uint16_t)camCfg.saturation);
    debugf("\r\n    contrast:   %x", (uint16_t)camCfg.contrast);
    debugf("\r\n    hvFlip:     %x", (uint16_t)camCfg.hvFlip);
    debugf("\r\n    nightMode:  %x", (uint16_t)camCfg.nightMode);
    debugf("\r\n    wbMode:     %x", (uint16_t)camCfg.wbMode);
    #endif
}

void GetCamCfg_Menu(uint8_t INIT_PROFILE)
{
    uint8_t i;

    if(cameraID==0)
        return;
    
    if(INIT_PROFILE)
        camProfile_Menu = camProfile;

    camCfg_Menu.brightness = camCfg.brightness;
    camCfg_Menu.sharpness = camCfg.sharpness;
    camCfg_Menu.saturation = camCfg.saturation;
    camCfg_Menu.contrast = camCfg.contrast;
    camCfg_Menu.hvFlip = camCfg.hvFlip;
    camCfg_Menu.nightMode = camCfg.nightMode;
    camCfg_Menu.wbMode = camCfg.wbMode;
    for(i=0;i<WBMODE_MAX;i++)
    {
        camCfg_Menu.wbRed[i] = camCfg.wbRed[i];
        camCfg_Menu.wbBlue[i] = camCfg.wbBlue[i];
    }
}

void SaveCamCfg_Menu(void)
{
    uint8_t i;

    if(cameraID == 0)
        return;
    
    camProfile = camProfile_Menu;
    camCfg.brightness = camCfg_Menu.brightness;
    camCfg.sharpness = camCfg_Menu.sharpness;
    camCfg.saturation = camCfg_Menu.saturation;
    camCfg.contrast = camCfg_Menu.contrast;
    camCfg.hvFlip = camCfg_Menu.hvFlip;
    camCfg.nightMode = camCfg_Menu.nightMode;
    camCfg.wbMode = camCfg_Menu.wbMode;
    for(i=0;i<WBMODE_MAX;i++)
    {
        camCfg.wbRed[i] = camCfg_Menu.wbRed[i];
        camCfg.wbBlue[i] = camCfg_Menu.wbBlue[i];
    }
    
    if(cameraID == RUNCAM_MICRO_V1)
    {
        camProfile_EEP &= 0xf0;
        camProfile_EEP |= (camProfile & 0x0f);
        WAIT(10); I2C_Write(ADDR_EEPROM, EEP_ADDR_CAM_PROFILE, camProfile_EEP, 0, 0);

        if(camProfile == 1)
        {
            camCfg_EEP[0].brightness = camCfg.brightness;
            camCfg_EEP[0].sharpness = camCfg.sharpness;
            camCfg_EEP[0].saturation = camCfg.saturation;
            camCfg_EEP[0].contrast = camCfg.contrast;
            camCfg_EEP[0].hvFlip = camCfg.hvFlip;
            camCfg_EEP[0].nightMode = camCfg.nightMode;
            camCfg_EEP[0].wbMode = camCfg.wbMode;
            for(i=0;i<WBMODE_MAX;i++)
            {
                camCfg_EEP[0].wbRed[i] = camCfg.wbRed[i];
                camCfg_EEP[0].wbBlue[i] = camCfg.wbBlue[i];
            }
            SaveCameraParameter();
        }
    }
    else if(cameraID == RUNCAM_MICRO_V2)
    {
        camProfile_EEP &= 0x0f;
        camProfile_EEP |= (camProfile << 4);
        WAIT(10); I2C_Write(ADDR_EEPROM, EEP_ADDR_CAM_PROFILE, camProfile_EEP, 0, 0);

        if(camProfile > 2)
        {
            camCfg_EEP[camProfile-2].brightness = camCfg.brightness;
            camCfg_EEP[camProfile-2].sharpness = camCfg.sharpness;
            camCfg_EEP[camProfile-2].saturation = camCfg.saturation;
            camCfg_EEP[camProfile-2].contrast = camCfg.contrast;
            camCfg_EEP[camProfile-2].hvFlip = camCfg.hvFlip;
            camCfg_EEP[camProfile-2].nightMode = camCfg.nightMode;
            camCfg_EEP[camProfile-2].wbMode = camCfg.wbMode;
            for(i=0;i<WBMODE_MAX;i++)
            {
                camCfg_EEP[camProfile-2].wbRed[i] = camCfg.wbRed[i];
                camCfg_EEP[camProfile-2].wbBlue[i] = camCfg.wbBlue[i];
            }
            SaveCameraParameter();
        }
    }
}

void SetCamCfg(cameraConfig_t *cfg, uint8_t INIT)
{
    uint8_t i,j;

    if(cfg == 0)
        return;
    if(cameraID == 0)
        return;
    if(INIT)
    {
        Runcam_SetBrightness(cfg->brightness);
        Runcam_SetSharpness(cfg->sharpness);
        Runcam_SetSaturation(cfg->saturation);
        Runcam_SetContrast(cfg->contrast);
        Runcam_SetHVFlip(cfg->hvFlip);
        Runcam_SetNightMode(cfg->nightMode);
        Runcam_SetWB(cfg->wbRed, cfg->wbBlue, cfg->wbMode);
    }
    else
    {
        if(camCfg_Cur.brightness != cfg->brightness)
            Runcam_SetBrightness(cfg->brightness);

        if(camCfg_Cur.sharpness != cfg->sharpness)
            Runcam_SetSharpness(cfg->sharpness);

        if(camCfg_Cur.saturation != cfg->saturation)
            Runcam_SetSaturation(cfg->saturation);

        if(camCfg_Cur.contrast != cfg->contrast)
            Runcam_SetContrast(cfg->contrast);

        if(camCfg_Cur.hvFlip != cfg->hvFlip)
            Runcam_SetHVFlip(cfg->hvFlip);

        if(camCfg_Cur.nightMode != cfg->nightMode)
            Runcam_SetNightMode(cfg->nightMode);

        j = 0;
        if(camCfg_Cur.wbMode != cfg->wbMode)
            j = 1;
        else
        {
            for(i=0;i<WBMODE_MAX;i++)
            {
                if(camCfg_Cur.wbRed[i] != cfg->wbRed[i])
                    j |= 1;
                if(camCfg_Cur.wbBlue[i] != cfg->wbBlue[i])
                    j |= 1;
            }
        }
        if(j==1)
            Runcam_SetWB(cfg->wbRed, cfg->wbBlue, cfg->wbMode);

    }
    #ifdef _DEBUG_MODE
        debugf("\r\nSet camera parameter done!");
    #endif
}

void CameraInit()
{
    CAM_MODE = CamDetect();
    Cam_Button_INIT();

    GetCamCfg_EEP();
    GetCamCfg(1);
    SetCamCfg(&camCfg, 1);
}

#if(0)
uint8_t Cmp_3dat(uint8_t d0, uint8_t d1, uint8_t d2)
{
    if(d2 == d1)
        return d2;
    else if(d2 == d0)
        return d2;
    else
        return d1;
}
void Cam_Read_Stable(uint8_t *d0, uint8_t *d1, uint8_t *d2, uint8_t *d3, uint8_t *d4)
{
    uint8_t dmy;
    uint8_t d00,d10,d20,d30,d40;
    uint8_t d01,d11,d21,d31,d41;
    uint8_t d02,d12,d22,d32,d42;
    
    CAM_ReadCFG(&dmy,&d00,&d10,&d20,&d30,&d40);
    CAM_ReadCFG(&dmy,&d01,&d11,&d21,&d31,&d41);
    CAM_ReadCFG(&dmy,&d02,&d12,&d22,&d32,&d42);

#if 1    
    *d0 = ((d0 == '\0') ? '\0' : Cmp_3dat(d00, d01, d02));
    *d1 = ((d1 == '\0') ? '\0' : Cmp_3dat(d10, d11, d12));
    *d2 = ((d2 == '\0') ? '\0' : Cmp_3dat(d20, d21, d22));
    *d3 = ((d3 == '\0') ? '\0' : Cmp_3dat(d30, d31, d32));
    *d4 = ((d4 == '\0') ? '\0' : Cmp_3dat(d40, d41, d42));   
#else    
    *d0 = Cmp_3dat(d00, d01, d02);
    *d1 = Cmp_3dat(d10, d11, d12);
    *d2 = Cmp_3dat(d20, d21, d22);
    *d3 = Cmp_3dat(d30, d31, d32);
    *d4 = Cmp_3dat(d40, d41, d42);
#endif
}
#endif


void Cam_Button_ENTER() { WriteReg(0, 0x14, 0x32);}
void Cam_Button_RIGHT() { WriteReg(0, 0x14, 0x58);}
void Cam_Button_DOWN()  { WriteReg(0, 0x14, 0x64);}
void Cam_Button_LEFT()  { WriteReg(0, 0x14, 0x3F);}
void Cam_Button_UP()    { WriteReg(0, 0x14, 0x4B);}
void Cam_Button_MID()   { WriteReg(0, 0x14, 0x00);}

#ifdef USE_MSP
uint8_t camStatusUpdate(uint8_t op)
{
    uint8_t ret = 0;
    uint8_t i;
    static uint8_t step = 1;
    static uint8_t cnt;
    static uint8_t last_op = BTN_RIGHT;
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;

    if(op >= BTN_INVALID)
        return ret;

    if(cameraID == 0)
    {
        switch(op)
        {
            case BTN_UP:    Cam_Button_UP();    break;
            case BTN_DOWN:  Cam_Button_DOWN();  break;
            case BTN_LEFT:  Cam_Button_LEFT();  break;
            case BTN_RIGHT: Cam_Button_RIGHT(); break;
            case BTN_ENTER: Cam_Button_ENTER(); break;
            case BTN_MID:   Cam_Button_MID();   break;
            default: break;
        }//switch(op)

        return ret;
    }

    switch(camMenuStatus)
    {
        case CAM_STATUS_IDLE:
            if(op == BTN_MID)
            {
                GetCamCfg_Menu(1);
                cnt = 0;
                camMenuStatus = CAM_STATUS_PROFILE;
            }
        break;
        
        case CAM_STATUS_PROFILE:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_SAVE_EXIT;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_BRIGHTNESS;
            else if(op == BTN_LEFT)
            {
                camProfile_Menu --;
                if(camProfile_Menu == 0xff)
                    camProfile_Menu = 0;
                else
                {
                    camProfile = camProfile_Menu;
                    GetCamCfg(0);
                    GetCamCfg_Menu(0);
                    SetCamCfg(&camCfg_Menu, 0);
                }
            }
            else if(op == BTN_RIGHT)
            {
                camProfile_Menu ++;
                if(cameraID == RUNCAM_MICRO_V1)
                {
                    if(camProfile_Menu > PROFILE_MAX_V1)
                        camProfile_Menu = PROFILE_MAX_V1;
                    else
                    {
                        camProfile = camProfile_Menu;
                        GetCamCfg(0);
                        GetCamCfg_Menu(0);
                        SetCamCfg(&camCfg_Menu, 0);
                    }
                }
                else// if(cameraID == RUNCAM_MICRO_V2)
                {
                    if(camProfile_Menu > PROFILE_MAX_V2)
                        camProfile_Menu = PROFILE_MAX_V2;
                    else
                    { 
                        camProfile = camProfile_Menu;
                        GetCamCfg(0);
                        GetCamCfg_Menu(0);
                        SetCamCfg(&camCfg_Menu, 0);
                    }
                }
            }
        break;

        case CAM_STATUS_BRIGHTNESS:
            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_PROFILE;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_SHARPNESS;
            else if(cameraID == RUNCAM_MICRO_V1 && camProfile_Menu == 0)
                break;
            else if(cameraID == RUNCAM_MICRO_V2 && camProfile_Menu <= 2)
                break;
            else if(op == BTN_RIGHT)
            {
                if(last_op == BTN_MID)
                {
                    cnt = 0;
                    step = 1;
                }
                else if(last_op == BTN_RIGHT)
                {
                    cnt++;
                    cnt &= 0x7f;
                }

                step = cnt >> 3;
                if(!step) step = 1;
                
                if((camCfg_Menu.brightness + step) > BRIGHTNESS_MAX)
                    camCfg_Menu.brightness = BRIGHTNESS_MAX;
                else
                    camCfg_Menu.brightness += step;

                Runcam_SetBrightness(camCfg_Menu.brightness);
            }
            else if(op == BTN_LEFT)
            {
                if(last_op == BTN_MID)
                {
                    cnt = 0;
                    step = 1;
                }else if(last_op == BTN_LEFT)
                {
                    cnt++;
                    cnt &= 0x7f;
                }
                
                step = cnt >> 3;
                if(!step) step = 1;

                if(camCfg_Menu.brightness - step < BRIGHTNESS_MIN)
                    camCfg_Menu.brightness = BRIGHTNESS_MIN;
                else
                    camCfg_Menu.brightness -= step;

                Runcam_SetBrightness(camCfg_Menu.brightness);
            }
        break;
            
        case CAM_STATUS_SHARPNESS:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_BRIGHTNESS;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_CONTRAST;
            else if(cameraID == RUNCAM_MICRO_V1 && camProfile_Menu == 0)
                break;
            else if(cameraID == RUNCAM_MICRO_V2 && camProfile_Menu <= 2)
                break;
            else if(op == BTN_RIGHT)
            {
                camCfg_Menu.sharpness ++;
                if(camCfg_Menu.sharpness > SHARPNESS_MAX)
                    camCfg_Menu.sharpness = SHARPNESS_MAX;
                Runcam_SetSharpness(camCfg_Menu.sharpness);
            }
            else if(op == BTN_LEFT)
            {
                camCfg_Menu.sharpness --;
                if(camCfg_Menu.sharpness > SHARPNESS_MAX) //0xff
                    camCfg_Menu.sharpness = SHARPNESS_MIN;
                Runcam_SetSharpness(camCfg_Menu.sharpness);
            }
        break;
        
        case CAM_STATUS_CONTRAST:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
               camMenuStatus = CAM_STATUS_SHARPNESS;
            if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_SATURATION;
            else if(cameraID == RUNCAM_MICRO_V1 && camProfile_Menu == 0)
                break;
            else if(cameraID == RUNCAM_MICRO_V2 && camProfile_Menu <= 2)
                break;
            else if(op == BTN_RIGHT)
            {
                camCfg_Menu.contrast ++;
                if(camCfg_Menu.contrast > CONTRAST_MAX)
                    camCfg_Menu.contrast = CONTRAST_MAX;
                Runcam_SetContrast(camCfg_Menu.contrast);
            }
            else if(op == BTN_LEFT)
            {
                camCfg_Menu.contrast --;
                if(camCfg_Menu.contrast > CONTRAST_MAX) //0xff
                    camCfg_Menu.contrast = CONTRAST_MIN;
                Runcam_SetContrast(camCfg_Menu.contrast);
            }
        break;

        case CAM_STATUS_SATURATION:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_CONTRAST;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_WBMODE;
            else if(cameraID == RUNCAM_MICRO_V1 && camProfile_Menu == 0)
                break;
            else if(cameraID == RUNCAM_MICRO_V2 && camProfile_Menu <= 2)
                break;
            else if(op == BTN_RIGHT)
            {
                camCfg_Menu.saturation ++;
                if(camCfg_Menu.saturation > SATURATION_MAX)
                    camCfg_Menu.saturation = SATURATION_MAX;
                Runcam_SetSaturation(camCfg_Menu.saturation);
            }
            else if(op == BTN_LEFT)
            {
                camCfg_Menu.saturation --;
                if(camCfg_Menu.saturation > SATURATION_MAX) //0xff
                    camCfg_Menu.saturation = SATURATION_MIN;
                Runcam_SetSaturation(camCfg_Menu.saturation);
            }
        break;

        case CAM_STATUS_WBMODE:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_SATURATION;
            else if(op == BTN_DOWN)
            {
                if(camCfg_Menu.wbMode)
                    camMenuStatus = CAM_STATUS_WBRED;
                else if(cameraID == RUNCAM_MICRO_V2)
                    camMenuStatus = CAM_STATUS_HVFLIP;
                else
                    camMenuStatus = CAM_STATUS_RESET;
            }
            else if(cameraID == RUNCAM_MICRO_V1 && camProfile_Menu == 0)
                break;
            else if(cameraID == RUNCAM_MICRO_V2 && camProfile_Menu <= 2)
                break;
            else if(op == BTN_RIGHT)
            {
                camCfg_Menu.wbMode ++;
                if(camCfg_Menu.wbMode > WBMODE_MAX)
                    camCfg_Menu.wbMode = WBMODE_MAX;
                Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
            }
            else if(op == BTN_LEFT)
            {
                camCfg_Menu.wbMode --;
                if(camCfg_Menu.wbMode > WBMODE_MAX)
                    camCfg_Menu.wbMode = WBMODE_MIN;
                Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
            }
        break;

        case CAM_STATUS_WBRED:
            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_WBMODE;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_WBBLUE;
            else if(cameraID == RUNCAM_MICRO_V1 && camProfile_Menu == 0)
                break;
            else if(cameraID == RUNCAM_MICRO_V2 && camProfile_Menu <= 2)
                break;
            else if(op == BTN_RIGHT)
            {
                if(last_op == BTN_MID)
                {
                    cnt = 0;
                    step = 1;
                }
                else if(last_op == BTN_RIGHT)
                {
                    cnt++;
                    cnt &= 0x7f;
                }
                
                step = cnt >> 3;
                if(!step) step = 1;
                camCfg_Menu.wbRed[camCfg_Menu.wbMode-1] += step;

                Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
            }
            else if(op == BTN_LEFT)
            {
                if(last_op == BTN_MID)
                {
                    cnt = 0;
                    step = 1;
                }
                else if(last_op == BTN_LEFT)
                {
                    cnt++;
                    cnt &= 0x7f;
                }
                
                step = cnt >> 3;
                if(!step) step = 1;
                camCfg_Menu.wbRed[camCfg_Menu.wbMode-1] -= step;

                Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
            }
        break;

        case CAM_STATUS_WBBLUE:
            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_WBRED;
            else if(op == BTN_DOWN)
            {
                if(cameraID == RUNCAM_MICRO_V2)
                    camMenuStatus = CAM_STATUS_HVFLIP;
                else
                    camMenuStatus = CAM_STATUS_RESET;
            }
            else if(cameraID == RUNCAM_MICRO_V1 && camProfile_Menu == 0)
                break;
            else if(cameraID == RUNCAM_MICRO_V2 && camProfile_Menu <= 2)
                break;
            else if(op == BTN_RIGHT)
            {
                if(last_op == BTN_MID)
                {
                    cnt = 0;
                    step = 1;
                }
                else if(last_op == BTN_RIGHT)
                {
                    cnt++;
                    cnt &= 0x7f;
                }
                
                step = cnt >> 3;
                if(!step) step = 1;
                camCfg_Menu.wbBlue[camCfg_Menu.wbMode-1] += step;

                Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
            }
            else if(op == BTN_LEFT)
            {
                if(last_op == BTN_MID)
                {
                    cnt = 0;
                    step = 1;
                }
                else if(last_op == BTN_LEFT)
                {
                    cnt++;
                    cnt &= 0x7f;
                }
                
                step = cnt >> 3;
                if(!step) step = 1;
                camCfg_Menu.wbBlue[camCfg_Menu.wbMode-1] -= step;

                Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
            }
        break;

        case CAM_STATUS_HVFLIP:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
            {
                if(camCfg_Menu.wbMode)
                    camMenuStatus = CAM_STATUS_WBBLUE;
                else
                    camMenuStatus = CAM_STATUS_WBMODE;
            }
            else if(op == BTN_DOWN)
            {
                if(cameraID == RUNCAM_MICRO_V2)
                    camMenuStatus = CAM_STATUS_NIGHTMODE;
                else
                    camMenuStatus = CAM_STATUS_RESET;
            }
            else if(op == BTN_RIGHT || op == BTN_LEFT)
            {
                camCfg_Menu.hvFlip = 1 - camCfg_Menu.hvFlip;
                Runcam_SetHVFlip(camCfg_Menu.hvFlip);
            }
        break;

        case CAM_STATUS_NIGHTMODE:
            if(last_op != BTN_MID)
                break;
                
            if(op == BTN_UP)
            {
                if(cameraID == RUNCAM_MICRO_V2)
                    camMenuStatus = CAM_STATUS_HVFLIP;
                else if(camCfg_Menu.wbMode)
                    camMenuStatus = CAM_STATUS_WBBLUE;
                else
                    camMenuStatus = CAM_STATUS_WBMODE;
            }
            else if(op == BTN_DOWN)
            {
                if(cameraID == RUNCAM_MICRO_V2)
                    camMenuStatus = CAM_STATUS_VDO_RATIO;
                else
                    camMenuStatus = CAM_STATUS_RESET;
            }
            else if(cameraID == RUNCAM_MICRO_V1 && camProfile_Menu == 0)
                break;
            else if(cameraID == RUNCAM_MICRO_V2 && camProfile_Menu <= 2)
                break;
            else if(op == BTN_RIGHT || op == BTN_LEFT)
            {
                camCfg_Menu.nightMode = 1 - camCfg_Menu.nightMode;
                Runcam_SetNightMode(camCfg_Menu.nightMode);
            }
        break;

        case CAM_STATUS_VDO_RATIO:
            if(last_op != BTN_MID)
                break;
                
            if(op == BTN_UP)
            {
                if(cameraID == RUNCAM_MICRO_V2)
                    camMenuStatus = CAM_STATUS_NIGHTMODE;
                else if(camCfg_Menu.wbMode)
                    camMenuStatus = CAM_STATUS_WBBLUE;
                else
                    camMenuStatus = CAM_STATUS_WBMODE;
            }
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_RESET;
            else if(op == BTN_RIGHT)
            {
                camMenuSetVdoRatioInit();
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_4_3;
            }
        break;

        case CAM_STATUS_SET_VDO_RATIO_4_3:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_RETURN;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_0;
            else if(op == BTN_RIGHT)
            {
                Runcam_SetVdoRatio(0);
                memset(osd_buf,0x20,sizeof(osd_buf));
                strcpy(osd_buf[1]+offset+2, " NEED TO REPOWER VTX");
                camMenuStatus = CAM_STATUS_REPOWER;
            }
        break;

        case CAM_STATUS_SET_VDO_RATIO_16_9_0:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_4_3;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_1;
            else if(op == BTN_RIGHT)
            {
                Runcam_SetVdoRatio(1);
                memset(osd_buf,0x20,sizeof(osd_buf));
                strcpy(osd_buf[1]+offset+2, " NEED TO REPOWER VTX");
                camMenuStatus = CAM_STATUS_REPOWER;
            }
        break;

        case CAM_STATUS_SET_VDO_RATIO_16_9_1:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_0;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_RETURN;
            else if(op == BTN_RIGHT)
            {
                Runcam_SetVdoRatio(2);
                memset(osd_buf,0x20,sizeof(osd_buf));
                strcpy(osd_buf[1]+offset+2, " NEED TO REPOWER VTX");
                camMenuStatus = CAM_STATUS_REPOWER;
            }
        break;

        case CAM_STATUS_SET_VDO_RATIO_RETURN:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_1;
            else if(op == BTN_DOWN)
                    camMenuStatus = CAM_STATUS_SET_VDO_RATIO_4_3;
            else if(op == BTN_RIGHT)
            {
                camMenuInit();
                camMenuStatus = CAM_STATUS_VDO_RATIO;
            }
        break;

        case CAM_STATUS_RESET:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
            {
                if(cameraID == RUNCAM_MICRO_V2)
                    camMenuStatus = CAM_STATUS_VDO_RATIO;
                else if(camCfg_Menu.wbMode)
                    camMenuStatus = CAM_STATUS_WBBLUE;
                else
                    camMenuStatus = CAM_STATUS_WBMODE;
            }
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_EXIT;
            #if(0)
            else if(cameraID == RUNCAM_MICRO_V1 && camProfile_Menu == 0)
                break;
            else if(cameraID == RUNCAM_MICRO_V2 && camProfile_Menu <= 2)
                break;
            #endif
            else if(op == BTN_RIGHT)
            {
                if(cameraID == RUNCAM_MICRO_V1)
                {
                    camCfg_Menu.brightness = camParameterInit[0][0];
                    camCfg_Menu.sharpness = camParameterInit[0][1];
                    camCfg_Menu.saturation = camParameterInit[0][2];
                    camCfg_Menu.contrast = camParameterInit[0][3];
                    camCfg_Menu.hvFlip = camParameterInit[0][4];
                    camCfg_Menu.nightMode = camParameterInit[0][5];
                    camCfg_Menu.wbMode = camParameterInit[0][6];
                    for(i=0;i<WBMODE_MAX;i++)
                    {
                        camCfg_Menu.wbRed[i] = camParameterInit[0][7+i];
                        camCfg_Menu.wbBlue[i] = camParameterInit[0][11+i];
                    }
                }
                else if(cameraID == RUNCAM_MICRO_V2)
                {
                    if(camProfile_Menu == 0 || camProfile_Menu == 3)            //micro V2
                    {
                        camCfg_Menu.brightness = camParameterInit[1][0];
                        camCfg_Menu.sharpness = camParameterInit[1][1];
                        camCfg_Menu.saturation = camParameterInit[1][2];
                        camCfg_Menu.contrast = camParameterInit[1][3];
                        camCfg_Menu.hvFlip = camParameterInit[1][4];
                        camCfg_Menu.nightMode = camParameterInit[1][5];
                        camCfg_Menu.wbMode = camParameterInit[1][6];
                        for(i=0;i<WBMODE_MAX;i++)
                        {
                            camCfg_Menu.wbRed[i] = camParameterInit[1][7+i];
                            camCfg_Menu.wbBlue[i] = camParameterInit[1][11+i];
                        }
                    }
                    else if(camProfile_Menu == 1 || camProfile_Menu == 4)       //nano v2
                    {
                        camCfg_Menu.brightness = camParameterInit[2][0];
                        camCfg_Menu.sharpness = camParameterInit[2][1];
                        camCfg_Menu.saturation = camParameterInit[2][2];
                        camCfg_Menu.contrast = camParameterInit[2][3];
                        camCfg_Menu.hvFlip = camParameterInit[2][4];
                        camCfg_Menu.nightMode = camParameterInit[2][5];
                        camCfg_Menu.wbMode = camParameterInit[2][6];
                        for(i=0;i<WBMODE_MAX;i++)
                        {
                            camCfg_Menu.wbRed[i] = camParameterInit[2][7+i];
                            camCfg_Menu.wbBlue[i] = camParameterInit[2][11+i];
                        }
                    }
                    else if(camProfile_Menu == 2 || camProfile_Menu == 5)       //nano lite
                    {
                        camCfg_Menu.brightness = camParameterInit[3][0];
                        camCfg_Menu.sharpness = camParameterInit[3][1];
                        camCfg_Menu.saturation = camParameterInit[3][2];
                        camCfg_Menu.contrast = camParameterInit[3][3];
                        camCfg_Menu.hvFlip = camParameterInit[3][4];
                        camCfg_Menu.nightMode = camParameterInit[3][5];
                        camCfg_Menu.wbMode = camParameterInit[3][6];
                        for(i=0;i<WBMODE_MAX;i++)
                        {
                            camCfg_Menu.wbRed[i] = camParameterInit[3][7+i];
                            camCfg_Menu.wbBlue[i] = camParameterInit[3][11+i];
                        }
                    }
                }
                SetCamCfg(&camCfg_Menu, 0);
            }
        break;
            
        case CAM_STATUS_EXIT:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_RESET;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_SAVE_EXIT;
            else if(op == BTN_RIGHT)
            {
                camMenuStatus = CAM_STATUS_IDLE;
                msp_tx_cnt = 0;
                ret = 1;
                SetCamCfg(&camCfg, 0);
            }
        break;

        case CAM_STATUS_SAVE_EXIT:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_EXIT;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_PROFILE;
            else if(op == BTN_RIGHT)
            {
                camMenuStatus = CAM_STATUS_IDLE;
                msp_tx_cnt = 0;
                ret = 1;
                SaveCamCfg_Menu();
            }
        break;
            
        case CAM_STATUS_REPOWER:
            break;

        default:break;
    }//switch(camMenuStatus)

    last_op = op;
    camMenuStringUpdate(camMenuStatus);

    return ret;
}
#endif

void camMenuDrawBracket(void)
{
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;
    uint8_t preset = 0;

    if(cameraID == RUNCAM_MICRO_V1)
        preset = 1 - camProfile_Menu;
    else if(cameraID == RUNCAM_MICRO_V2)
    {
        if(camProfile_Menu <= 2)
            preset = 1;
        else
            preset = 0;
    }

    osd_buf[CAM_STATUS_PROFILE][offset+17] = '<';
    osd_buf[CAM_STATUS_PROFILE][offset+29] = '>';

    if(preset == 0)
    {
        osd_buf[CAM_STATUS_BRIGHTNESS][offset+19] = '<';
        osd_buf[CAM_STATUS_BRIGHTNESS][offset+29] = '>';
        osd_buf[CAM_STATUS_SHARPNESS][offset+19] = '<';
        osd_buf[CAM_STATUS_SHARPNESS][offset+29] = '>';
        osd_buf[CAM_STATUS_CONTRAST][offset+19] = '<';
        osd_buf[CAM_STATUS_CONTRAST][offset+29] = '>';
        osd_buf[CAM_STATUS_SATURATION][offset+19] = '<';
        osd_buf[CAM_STATUS_SATURATION][offset+29] = '>';
        osd_buf[CAM_STATUS_WBMODE][offset+19] = '<';
        osd_buf[CAM_STATUS_WBMODE][offset+29] = '>';
        if(camCfg_Menu.wbMode == 0)
        {
            osd_buf[CAM_STATUS_WBRED][offset+19] = ' ';
            osd_buf[CAM_STATUS_WBRED][offset+29] = ' ';
            osd_buf[CAM_STATUS_WBBLUE][offset+19] = ' ';
            osd_buf[CAM_STATUS_WBBLUE][offset+29] = ' ';
        }
        else
        {
            osd_buf[CAM_STATUS_WBRED][offset+19] = '<';
            osd_buf[CAM_STATUS_WBRED][offset+29] = '>';
            osd_buf[CAM_STATUS_WBBLUE][offset+19] = '<';
            osd_buf[CAM_STATUS_WBBLUE][offset+29] = '>';
        }
        if(cameraID == RUNCAM_MICRO_V2)
        {
            osd_buf[CAM_STATUS_HVFLIP][offset+19] = '<';
            osd_buf[CAM_STATUS_HVFLIP][offset+29] = '>';
            osd_buf[CAM_STATUS_NIGHTMODE][offset+19] = '<';
            osd_buf[CAM_STATUS_NIGHTMODE][offset+29] = '>';
        }
    }
    else
    {
        osd_buf[CAM_STATUS_BRIGHTNESS][offset+19] = ' ';
        osd_buf[CAM_STATUS_BRIGHTNESS][offset+29] = ' ';
        osd_buf[CAM_STATUS_SHARPNESS][offset+19] = ' ';
        osd_buf[CAM_STATUS_SHARPNESS][offset+29] = ' ';
        osd_buf[CAM_STATUS_CONTRAST][offset+19] = ' ';
        osd_buf[CAM_STATUS_CONTRAST][offset+29] = ' ';
        osd_buf[CAM_STATUS_SATURATION][offset+19] = ' ';
        osd_buf[CAM_STATUS_SATURATION][offset+29] = ' ';
        osd_buf[CAM_STATUS_WBMODE][offset+19] = ' ';
        osd_buf[CAM_STATUS_WBMODE][offset+29] = ' ';
        osd_buf[CAM_STATUS_WBRED][offset+19] = ' ';
        osd_buf[CAM_STATUS_WBRED][offset+29] = ' ';
        osd_buf[CAM_STATUS_WBBLUE][offset+19] = ' ';
        osd_buf[CAM_STATUS_WBBLUE][offset+29] = ' ';
        if(cameraID == RUNCAM_MICRO_V2)
        {
            osd_buf[CAM_STATUS_HVFLIP][offset+19] = '<';
            osd_buf[CAM_STATUS_HVFLIP][offset+29] = '>';
            osd_buf[CAM_STATUS_NIGHTMODE][offset+19] = ' ';
            osd_buf[CAM_STATUS_NIGHTMODE][offset+29] = ' ';
        }
    }
}


void camMenuInit()
{
    uint8_t i = 0;
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;
    
    memset(osd_buf,0x20,sizeof(osd_buf));
    disp_mode = DISPLAY_CMS;
    if(cameraID == 0)
         Cam_Button_ENTER();
    else
    {
        strcpy(osd_buf[i++]+offset+2, "----CAMERA MENU----");
        strcpy(osd_buf[i++]+offset+2, ">PROFILE");
        strcpy(osd_buf[i++]+offset+2, " BRIGHTNESS");
        strcpy(osd_buf[i++]+offset+2, " SHARPNESS");
        strcpy(osd_buf[i++]+offset+2, " CONTRAST");
        strcpy(osd_buf[i++]+offset+2, " SATURATION");
        strcpy(osd_buf[i++]+offset+2, " WB MODE");
        strcpy(osd_buf[i++]+offset+2, " WB RED");
        strcpy(osd_buf[i++]+offset+2, " WB BLUE");
        strcpy(osd_buf[i++]+offset+2, " HV FLIP");
        strcpy(osd_buf[i++]+offset+2, " MAX GAIN");
        strcpy(osd_buf[i++]+offset+2, " ASPECT RATIO");
        if(cameraID == RUNCAM_MICRO_V1)
            strcpy(osd_buf[i-1]+offset+19, "NOT SUPPORT");
        strcpy(osd_buf[i++]+offset+2,  " RESET");
        strcpy(osd_buf[i++]+offset+2,  " EXIT");
        strcpy(osd_buf[i++]+offset+2,  " SAVE&EXIT");

        camMenuDrawBracket();
        //camMenuStringUpdate(CAM_STATUS_PROFILE);
    }
}

void camMenuSetVdoRatioInit()
{
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;
    
    memset(osd_buf,0x20,sizeof(osd_buf));
    strcpy(osd_buf[0]+offset+2, "--VIDEO FORMAT--");
    strcpy(osd_buf[1]+offset+3, "SET 720P 4:3  DEFAULT");
    strcpy(osd_buf[2]+offset+3, "SET 720P 16:9 CROP");
    strcpy(osd_buf[3]+offset+3, "SET 720P 16:9 FULL");
    strcpy(osd_buf[4]+offset+3, "RETURN");
    osd_buf[1][offset+2] = '>';
}

void camMenuStringUpdate(uint8_t status)
{
    uint8_t i;
    uint8_t Str[3];
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;
    uint8_t dat;

    // Pointer
    for(i = CAM_STATUS_PROFILE; i <= CAM_STATUS_SAVE_EXIT; i++){
        if(i==status){
            osd_buf[i][offset+2] = '>';
        }
        else
            osd_buf[i][offset+2] = ' ';
    }
    if(status > CAM_STATUS_SAVE_EXIT){
        for(i = CAM_STATUS_SET_VDO_RATIO_4_3; i <= CAM_STATUS_SET_VDO_RATIO_RETURN; i++){
            if(i==status){
                osd_buf[i-CAM_STATUS_SAVE_EXIT][offset+2] = '>';
            }
            else
                osd_buf[i-CAM_STATUS_SAVE_EXIT][offset+2] = ' ';
        }
    }

    if(status > CAM_STATUS_SAVE_EXIT)
        return;

    //Profile
    if(cameraID == RUNCAM_MICRO_V1)
    {
        if(camProfile_Menu == 0)
            strcpy(osd_buf[CAM_STATUS_PROFILE]+offset+18, "   MICRO V1");
        else
            strcpy(osd_buf[CAM_STATUS_PROFILE]+offset+18, " MICRO V1 M");
    }
    else
    {
        if(camProfile_Menu == 0)
            strcpy(osd_buf[CAM_STATUS_PROFILE]+offset+18, "   MICRO V2");
        else if(camProfile_Menu == 1)
            strcpy(osd_buf[CAM_STATUS_PROFILE]+offset+18, "    NANO V2");
        else if(camProfile_Menu == 2)
            strcpy(osd_buf[CAM_STATUS_PROFILE]+offset+18, "  NANO LITE");
        else if(camProfile_Menu == 3)
            strcpy(osd_buf[CAM_STATUS_PROFILE]+offset+18, " MICRO V2 M");
        else if(camProfile_Menu == 4)
            strcpy(osd_buf[CAM_STATUS_PROFILE]+offset+18, "  NANO V2 M");
        else if(camProfile_Menu == 5)
            strcpy(osd_buf[CAM_STATUS_PROFILE]+offset+18, "NANO LITE M");
    }

    //Brightness
    if(camCfg_Menu.brightness < CAM_BRIGHTNESS_INITIAL)
    {
        dat = CAM_BRIGHTNESS_INITIAL - camCfg_Menu.brightness;
        osd_buf[CAM_STATUS_BRIGHTNESS][offset+24] = '-';
    }
    else if(camCfg_Menu.brightness > CAM_BRIGHTNESS_INITIAL)
    {
        dat = camCfg_Menu.brightness - CAM_BRIGHTNESS_INITIAL;
        osd_buf[CAM_STATUS_BRIGHTNESS][offset+24] = '+';
    }
    else// if(camCfg_Menu.brightness == CAM_BRIGHTNESS_INITIAL)
    {
        dat = 0;
        osd_buf[CAM_STATUS_BRIGHTNESS][offset+24] = ' ';
    }
    uint8ToString(dat, Str);
    osd_buf[CAM_STATUS_BRIGHTNESS][offset+25] = Str[1];
    osd_buf[CAM_STATUS_BRIGHTNESS][offset+26] = Str[2];

    //sharpness
    if(camCfg_Menu.sharpness == 0)
        strcpy(osd_buf[CAM_STATUS_SHARPNESS]+offset+21, "   LOW");
    else if(camCfg_Menu.sharpness == 1)
        strcpy(osd_buf[CAM_STATUS_SHARPNESS]+offset+21, "MEDIUM");
    else if(camCfg_Menu.sharpness == 2)
        strcpy(osd_buf[CAM_STATUS_SHARPNESS]+offset+21, "  HIGH");
    
    //saturation
    strcpy(osd_buf[CAM_STATUS_SATURATION]+offset+21, "LEVEL");
    osd_buf[CAM_STATUS_SATURATION][offset+26] = '1'+ camCfg_Menu.saturation;
    
    //contrast
    if(camCfg_Menu.contrast == 0)
        strcpy(osd_buf[CAM_STATUS_CONTRAST]+offset+21, "   LOW");
    else if(camCfg_Menu.contrast == 1)
        strcpy(osd_buf[CAM_STATUS_CONTRAST]+offset+21, "MEDIUM");
    else if(camCfg_Menu.contrast == 2)
        strcpy(osd_buf[CAM_STATUS_CONTRAST]+offset+21, "  HIGH");
    
    //hvFlip
    if(cameraID == RUNCAM_MICRO_V2)
    {
        if(camCfg_Menu.hvFlip == 0)
            strcpy(osd_buf[CAM_STATUS_HVFLIP]+offset+21, "  OFF");
        else if(camCfg_Menu.hvFlip == 1)
            strcpy(osd_buf[CAM_STATUS_HVFLIP]+offset+21, "   ON");
    }
    else if(cameraID == RUNCAM_MICRO_V1)
        strcpy(osd_buf[CAM_STATUS_HVFLIP]+offset+19, "NOT SUPPORT");
    
    //nightMode
    if(cameraID == RUNCAM_MICRO_V2)
    {
        if(camCfg_Menu.nightMode == 0)
            strcpy(osd_buf[CAM_STATUS_NIGHTMODE]+offset+21, "  OFF");
        else if(camCfg_Menu.nightMode == 1)
            strcpy(osd_buf[CAM_STATUS_NIGHTMODE]+offset+21, "   ON");
    }
    else if(cameraID == RUNCAM_MICRO_V1)
        strcpy(osd_buf[CAM_STATUS_NIGHTMODE]+offset+19, "NOT SUPPORT");
    
    //wb
    if(camCfg_Menu.wbMode == 0){
        strcpy(osd_buf[CAM_STATUS_WBMODE]+offset+21, "  AUTO ");
        strcpy(osd_buf[CAM_STATUS_WBRED ]+offset+21, "       ");
        strcpy(osd_buf[CAM_STATUS_WBBLUE]+offset+21, "       ");
    }else{
        strcpy(osd_buf[CAM_STATUS_WBMODE]+offset+21, "MANUAL ");
        osd_buf[CAM_STATUS_WBMODE][offset+27] = '0'+ camCfg_Menu.wbMode;
        uint8ToString(camCfg_Menu.wbRed[camCfg_Menu.wbMode-1], Str);
        strcpy(osd_buf[CAM_STATUS_WBRED]+offset+24, Str);
        uint8ToString(camCfg_Menu.wbBlue[camCfg_Menu.wbMode-1], Str);
        strcpy(osd_buf[CAM_STATUS_WBBLUE]+offset+24, Str);
    }

    camMenuDrawBracket();
}
#pragma restore