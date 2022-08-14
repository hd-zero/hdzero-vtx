

#include "camera.h"
#include "i2c_device.h"
#include "global.h"
#include "i2c.h"
#include "print.h"
#include "msp_displayport.h"
#include "isr.h"
#include "hardware.h"

uint8_t cameraID = 0;
RuncamV2Profile_e camProfile_EEP;
RuncamV2Profile_e camProfile;
RuncamV2Profile_e camProfile_Menu;
cameraConfig_t camCfg;
cameraConfig_t camCfg_Menu;
cameraConfig_t camCfg_Cur;
cameraConfig_t camCfg_EEP[CAM_PROFILE_NUM];

// {brightness, sharpness, saturation, contrast, hvFlip, nightMode, wbMode,
//  wbRed[0], wbRed[1], wbRed[2], wbRed[3],
//  wbBlue[0], wbBlue[1], wbBlue[2], wbBlue[3],}
const uint8_t camParameterInit[CAM_PROFILE_NUM][15] = 
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
uint8_t cam_4_3 = 0;
uint8_t camMenuStatus = CAM_STATUS_IDLE;
uint8_t bw_changed = 0;

void camMenuStringUpdate(uint8_t status);
void camMenuSetVdoRatioInit();

uint8_t CamDetect()
{
    int fps = CAM_720P60;
    uint8_t cycles = 4;
    uint8_t loss = 0;
    uint8_t detect_tries = 0;
    uint8_t status_reg = 0;

    WriteReg(0, 0x8F, 0x91);

    while (cycles) {
        if (fps == CAM_720P50) {
            Init_TC3587();
            Set_720P50(IS_RX);
            debugf("\r\nCamDetect: Set 50fps.");
        } else if (fps == CAM_720P60) {
            Init_TC3587();
            Set_720P60(IS_RX);
            debugf("\r\nCamDetect: Set 60fps.");
        }
        WAIT(100);

        for (detect_tries = 0; detect_tries < 5; detect_tries++) {
            status_reg = ReadReg(0, 0x02);
            debugf("\r\nCamDetect status_reg: %x", status_reg);

            if ((status_reg >> 4) != 0) {
                loss = 1;
            }
            WAIT(5);
        }

        if (loss == 0)
            break;

        fps = (fps == CAM_720P60) ? CAM_720P50 : CAM_720P60;

        loss = 0;
        cycles--;
    }

    if(cycles == 0){
        fps = CAM_720P60_NEW;
        Set_720P60(IS_RX);
        I2C_Write16(ADDR_TC3587, 0x0058, 0x00e0);
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
    debugf("\r\nRUNCAM brightness:0x%02x", (uint16_t)val);
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
    debugf("\r\nRUNCAM sharpness:0x%02x", (uint16_t)val);
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
    debugf("\r\nRUNCAM saturation:%02x", (uint16_t)val);
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
    debugf("\r\nRUNCAM contrast:%02x", (uint16_t)val);
    #endif
}

void Runcam_SetVdoRatio(uint8_t ratio)
{
/*
    0: 720p60 4:3 default
    1: 720p60 16:9 crop
    2: 720p30 16:9 full
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
    debugf("\r\nRUNCAM VdoRatio:%02x", (uint16_t)ratio);
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
    debugf("\r\nVdoFormat:%02x, 4_3:%02x", (uint16_t)ret, (uint16_t)cam_4_3);
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
    debugf("\r\nRUNCAM NightMode:%02x", (uint16_t)val);
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

void camera_write_eep_parameter(uint16_t addr, uint8_t val) {
    I2C_Write8_Wait(10, ADDR_EEPROM, addr, val);
}

uint8_t camera_read_eep_parameter(uint16_t addr) {
    return I2C_Read8_Wait(10, ADDR_EEPROM, addr);
}

void camera_check_and_save_parameters() {
    uint8_t i = 0;
    uint8_t j = 0;

    if (cameraID == 0)
        return;

    if (camProfile_EEP >= Profile_Max) 
        camProfile_EEP = Profile_MicroV2;

    for (i = 0; i < CAM_PROFILE_NUM; i++) {
        if (camCfg_EEP[i].brightness < BRIGHTNESS_MIN || camCfg_EEP[i].brightness > BRIGHTNESS_MAX) {
            camCfg_EEP[i].brightness = camParameterInit[i][0];
        }

        if (camCfg_EEP[i].sharpness > SHARPNESS_MAX) {
            camCfg_EEP[i].sharpness = camParameterInit[i][1];
        }

        if (camCfg_EEP[i].saturation > SATURATION_MAX) {
            camCfg_EEP[i].saturation = camParameterInit[i][2];
        }

        if (camCfg_EEP[i].contrast > CONTRAST_MAX) {
            camCfg_EEP[i].contrast = camParameterInit[i][3];
        }

        if (camCfg_EEP[i].hvFlip > HVFLIP_MAX) {
            camCfg_EEP[i].hvFlip = camParameterInit[i][4];
        }

        if (camCfg_EEP[i].nightMode > NIGHTMODE_MAX) {
            camCfg_EEP[i].nightMode = camParameterInit[i][5];
        }

        if (camCfg_EEP[i].wbMode > WBMODE_MAX) {
            camCfg_EEP[i].wbMode = camParameterInit[i][6];

            for (j = 0; j < WBMODE_MAX; j++) {
                camCfg_EEP[i].wbRed[j] = camParameterInit[i][7 + j];
                camCfg_EEP[i].wbBlue[j] = camParameterInit[i][11 + j];
            }
        }

        camera_write_eep_parameter(EEP_ADDR_CAM_PROFILE, camProfile_EEP);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_BRIGHTNESS, camCfg_EEP[i].brightness);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_SHARPNESS, camCfg_EEP[i].sharpness);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_SATURATION, camCfg_EEP[i].saturation);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_CONTRAST, camCfg_EEP[i].contrast);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_HVFLIP, camCfg_EEP[i].hvFlip);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_NIGHTMODE, camCfg_EEP[i].nightMode);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_WBMODE, camCfg_EEP[i].wbMode);
        for (j = 0; j < WBMODE_MAX; j++) {
            camera_write_eep_parameter((i << 4) + j + EEP_ADDR_CAM_WBRED, camCfg_EEP[i].wbRed[j]);
            camera_write_eep_parameter((i << 4) + j + EEP_ADDR_CAM_WBBLUE, camCfg_EEP[i].wbBlue[j]);
        }
    }
}

void GetCamCfg_EEP(void) {
    uint8_t i, j;

    if (cameraID == 0)
        return;

    if (cameraID == RUNCAM_MICRO_V1 || cameraID == RUNCAM_MICRO_V2) {
        camProfile_EEP = camera_read_eep_parameter(EEP_ADDR_CAM_PROFILE);
        for (i = 0; i < CAM_PROFILE_NUM; i++) {
            camCfg_EEP[i].brightness = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_BRIGHTNESS);
            camCfg_EEP[i].sharpness = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_SHARPNESS);
            camCfg_EEP[i].saturation = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_SATURATION);
            camCfg_EEP[i].contrast = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_CONTRAST);
            camCfg_EEP[i].hvFlip = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_HVFLIP);
            camCfg_EEP[i].nightMode = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_NIGHTMODE);
            camCfg_EEP[i].wbMode = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_WBMODE);
            for (j = 0; j < WBMODE_MAX; j++) {
                camCfg_EEP[i].wbRed[j] = camera_read_eep_parameter((i << 4) + j + EEP_ADDR_CAM_WBRED);
                camCfg_EEP[i].wbBlue[j] = camera_read_eep_parameter((i << 4) + j + EEP_ADDR_CAM_WBBLUE);
            }
        }
    }

    camera_check_and_save_parameters();
}
void GetCamCfg(uint8_t USE_EEP_PROFILE)
{
    uint8_t i;
    uint8_t index = 0;

    if(cameraID == 0)
        return;

    if(USE_EEP_PROFILE)
        camProfile = camProfile_EEP;

    if(cameraID == RUNCAM_MICRO_V1)
        index = 0;
    else if(cameraID == RUNCAM_MICRO_V2)
        index = camProfile + 1;

    camCfg.brightness = camCfg_EEP[index].brightness;
    camCfg.sharpness = camCfg_EEP[index].sharpness;
    camCfg.saturation = camCfg_EEP[index].saturation;
    camCfg.contrast = camCfg_EEP[index].contrast;
    camCfg.hvFlip = camCfg_EEP[index].hvFlip;
    camCfg.nightMode = camCfg_EEP[index].nightMode;
    camCfg.wbMode = camCfg_EEP[index].wbMode;
    for(i=0;i<WBMODE_MAX;i++)
    {
        camCfg.wbRed[i] = camCfg_EEP[index].wbRed[i];
        camCfg.wbBlue[i] = camCfg_EEP[index].wbBlue[i];
    }

    #ifdef _DEBUG_CAMERA
    debugf("\r\nGetCamCfg->camProfile: 0x%02x", (uint16_t)camProfile);
    debugf("\r\nGetCamCfg->index:      0x%02x", (uint16_t)index);
    debugf("\r\nGetCamCfg->brightness: 0x%02x", (uint16_t)camCfg.brightness);
    debugf("\r\nGetCamCfg->sharpness:  0x%02x", (uint16_t)camCfg.sharpness);
    debugf("\r\nGetCamCfg->saturation: 0x%02x", (uint16_t)camCfg.saturation);
    debugf("\r\nGetCamCfg->contrast:   0x%02x", (uint16_t)camCfg.contrast);
    debugf("\r\nGetCamCfg->hvFlip:     0x%02x", (uint16_t)camCfg.hvFlip);
    debugf("\r\nGetCamCfg->nightMode:  0x%02x", (uint16_t)camCfg.nightMode);
    debugf("\r\nGetCamCfg->wbMode:     0x%02x", (uint16_t)camCfg.wbMode);
    #endif
}

void GetCamCfg_Menu(uint8_t INIT_PROFILE)
{
    uint8_t i;

    if(cameraID == 0)
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
    uint8_t index = 0;

    if(cameraID == 0)
        return;

    if(cameraID == RUNCAM_MICRO_V2)
    {
        camProfile = camProfile_Menu;
        index = 1 + camProfile;
    }
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

    camProfile_EEP = camProfile;
    camCfg_EEP[index].brightness = camCfg.brightness;
    camCfg_EEP[index].sharpness = camCfg.sharpness;
    camCfg_EEP[index].saturation = camCfg.saturation;
    camCfg_EEP[index].contrast = camCfg.contrast;
    camCfg_EEP[index].hvFlip = camCfg.hvFlip;
    camCfg_EEP[index].nightMode = camCfg.nightMode;
    camCfg_EEP[index].wbMode = camCfg.wbMode;
    for(i=0;i<WBMODE_MAX;i++)
    {
        camCfg_EEP[index].wbRed[i] = camCfg.wbRed[i];
        camCfg_EEP[index].wbBlue[i] = camCfg.wbBlue[i];
    }

    camera_check_and_save_parameters();

    #ifdef _DEBUG_CAMERA
    debugf("\r\nSaveCamCfg->camProfile: 0x%02x", (uint16_t)camProfile_EEP);
    debugf("\r\nSaveCamCfg->index:      0x%02x", (uint16_t)index);
    debugf("\r\nSaveCamCfg->brightness: 0x%02x", (uint16_t)camCfg_EEP[index].brightness);
    debugf("\r\nSaveCamCfg->sharpness:  0x%02x", (uint16_t)camCfg_EEP[index].sharpness);
    debugf("\r\nSaveCamCfg->saturation: 0x%02x", (uint16_t)camCfg_EEP[index].saturation);
    debugf("\r\nSaveCamCfg->contrast:   0x%02x", (uint16_t)camCfg_EEP[index].contrast);
    debugf("\r\nSaveCamCfg->hvFlip:     0x%02x", (uint16_t)camCfg_EEP[index].hvFlip);
    debugf("\r\nSaveCamCfg->nightMode:  0x%02x", (uint16_t)camCfg_EEP[index].nightMode);
    debugf("\r\nSaveCamCfg->wbMode:     0x%02x", (uint16_t)camCfg_EEP[index].wbMode);
    #endif
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
            if (op == BTN_MID)
            {
                GetCamCfg_Menu(1);
                cnt = 0;
                if(cameraID == RUNCAM_MICRO_V1)
                    camMenuStatus = CAM_STATUS_BRIGHTNESS;
                else
                    camMenuStatus = CAM_STATUS_PROFILE;
            }
        break;

        case CAM_STATUS_PROFILE:
            if(last_op != BTN_MID)
                break;

            if (op == BTN_UP)
                camMenuStatus = CAM_STATUS_SAVE_EXIT;
            else if (op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_BRIGHTNESS;
            else if (op == BTN_LEFT)
            {
                camProfile_Menu --;
                if(camProfile_Menu  >= Profile_Max)
                    camProfile_Menu = Profile_Max - 1;
        
                camProfile = camProfile_Menu;
                GetCamCfg(0);
                GetCamCfg_Menu(0);
                SetCamCfg(&camCfg_Menu, 0);
            }
            else if (op == BTN_RIGHT)
            {
                camProfile_Menu ++;
                if(camProfile_Menu >= Profile_Max)
                    camProfile_Menu = 0;
        
                camProfile = camProfile_Menu;
                GetCamCfg(0);
                GetCamCfg_Menu(0);
                SetCamCfg(&camCfg_Menu, 0);
            }
        break;

        case CAM_STATUS_BRIGHTNESS:
            if (op == BTN_UP)
            {
                if(cameraID == RUNCAM_MICRO_V1)
                    camMenuStatus = CAM_STATUS_SAVE_EXIT;
                else
                    camMenuStatus = CAM_STATUS_PROFILE;
            }
            else if (op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_SHARPNESS;
            else if (op == BTN_RIGHT)
            {
                if (last_op == BTN_MID)
                {
                    cnt = 0;
                    step = 1;
                }
                else if (last_op == BTN_RIGHT)
                {
                    cnt++;
                    cnt &= 0x7f;
                }

                step = cnt >> 3;
                if (!step)
                    step = 1;

                camCfg_Menu.brightness += step;
                if (camCfg_Menu.brightness > BRIGHTNESS_MAX)
                    camCfg_Menu.brightness = BRIGHTNESS_MAX;

                Runcam_SetBrightness(camCfg_Menu.brightness);
            }
            else if (op == BTN_LEFT)
            {
                if (last_op == BTN_MID)
                {
                    cnt = 0;
                    step = 1;
                } 
                else if (last_op == BTN_LEFT)
                {
                    cnt++;
                    cnt &= 0x7f;
                }

                step = cnt >> 3;
                if (!step)
                    step = 1;

                camCfg_Menu.brightness -= step;
                if (camCfg_Menu.brightness < BRIGHTNESS_MIN)
                    camCfg_Menu.brightness = BRIGHTNESS_MIN;

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
                if(camCfg_Menu.sharpness > SHARPNESS_MAX)
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
                if(camCfg_Menu.contrast > CONTRAST_MAX)
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
                if(camCfg_Menu.saturation > SATURATION_MAX)
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
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_CROP;
            else if(op == BTN_RIGHT)
            {
                Runcam_SetVdoRatio(0);
                memset(osd_buf,0x20,sizeof(osd_buf));
                strcpy(osd_buf[1]+osd_menu_offset+2, " NEED TO REPOWER VTX");
                camMenuStatus = CAM_STATUS_REPOWER;
            }
        break;

        case CAM_STATUS_SET_VDO_RATIO_16_9_CROP:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_4_3;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_FULL;
            else if(op == BTN_RIGHT)
            {
                Runcam_SetVdoRatio(1);
                memset(osd_buf,0x20,sizeof(osd_buf));
                strcpy(osd_buf[1]+osd_menu_offset+2, " NEED TO REPOWER VTX");
                camMenuStatus = CAM_STATUS_REPOWER;
            }
        break;

        case CAM_STATUS_SET_VDO_RATIO_16_9_FULL:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_CROP;
            else if(op == BTN_DOWN)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_RETURN;
            else if(op == BTN_RIGHT)
            {
                Runcam_SetVdoRatio(2);
                memset(osd_buf,0x20,sizeof(osd_buf));
                strcpy(osd_buf[1]+osd_menu_offset+2, " NEED TO REPOWER VTX");
                camMenuStatus = CAM_STATUS_REPOWER;
            }
        break;

        case CAM_STATUS_SET_VDO_RATIO_RETURN:
            if(last_op != BTN_MID)
                break;

            if(op == BTN_UP)
                camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_FULL;
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
                    camCfg_Menu.brightness = camParameterInit[camProfile_Menu+1][0];
                    camCfg_Menu.sharpness = camParameterInit[camProfile_Menu+1][1];
                    camCfg_Menu.saturation = camParameterInit[camProfile_Menu+1][2];
                    camCfg_Menu.contrast = camParameterInit[camProfile_Menu+1][3];
                    camCfg_Menu.hvFlip = camParameterInit[camProfile_Menu+1][4];
                    camCfg_Menu.nightMode = camParameterInit[camProfile_Menu+1][5];
                    camCfg_Menu.wbMode = camParameterInit[camProfile_Menu+1][6];
                    for(i=0;i<WBMODE_MAX;i++)
                    {
                        camCfg_Menu.wbRed[i] = camParameterInit[camProfile_Menu+1][7+i];
                        camCfg_Menu.wbBlue[i] = camParameterInit[camProfile_Menu+1][11+i];
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
            else if(op == BTN_DOWN){
                if(cameraID == RUNCAM_MICRO_V1)
                    camMenuStatus = CAM_STATUS_BRIGHTNESS;
                else
                    camMenuStatus = CAM_STATUS_PROFILE;
            }
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
    if(cameraID == RUNCAM_MICRO_V1)
    {
        osd_buf[CAM_STATUS_PROFILE][osd_menu_offset+17] = ' ';
        osd_buf[CAM_STATUS_PROFILE][osd_menu_offset+29] = ' ';
    }
    else
    {
        osd_buf[CAM_STATUS_PROFILE][osd_menu_offset+17] = '<';
        osd_buf[CAM_STATUS_PROFILE][osd_menu_offset+29] = '>';
    }
    osd_buf[CAM_STATUS_BRIGHTNESS][osd_menu_offset+19] = '<';
    osd_buf[CAM_STATUS_BRIGHTNESS][osd_menu_offset+29] = '>';
    osd_buf[CAM_STATUS_SHARPNESS][osd_menu_offset+19] = '<';
    osd_buf[CAM_STATUS_SHARPNESS][osd_menu_offset+29] = '>';
    osd_buf[CAM_STATUS_CONTRAST][osd_menu_offset+19] = '<';
    osd_buf[CAM_STATUS_CONTRAST][osd_menu_offset+29] = '>';
    osd_buf[CAM_STATUS_SATURATION][osd_menu_offset+19] = '<';
    osd_buf[CAM_STATUS_SATURATION][osd_menu_offset+29] = '>';
    osd_buf[CAM_STATUS_WBMODE][osd_menu_offset+19] = '<';
    osd_buf[CAM_STATUS_WBMODE][osd_menu_offset+29] = '>';
    if(camCfg_Menu.wbMode == 0)
    {
        osd_buf[CAM_STATUS_WBRED][osd_menu_offset+19] = ' ';
        osd_buf[CAM_STATUS_WBRED][osd_menu_offset+29] = ' ';
        osd_buf[CAM_STATUS_WBBLUE][osd_menu_offset+19] = ' ';
        osd_buf[CAM_STATUS_WBBLUE][osd_menu_offset+29] = ' ';
    }
    else
    {
        osd_buf[CAM_STATUS_WBRED][osd_menu_offset+19] = '<';
        osd_buf[CAM_STATUS_WBRED][osd_menu_offset+29] = '>';
        osd_buf[CAM_STATUS_WBBLUE][osd_menu_offset+19] = '<';
        osd_buf[CAM_STATUS_WBBLUE][osd_menu_offset+29] = '>';
    }
    if(cameraID == RUNCAM_MICRO_V2)
    {
        osd_buf[CAM_STATUS_HVFLIP][osd_menu_offset+19] = '<';
        osd_buf[CAM_STATUS_HVFLIP][osd_menu_offset+29] = '>';
        osd_buf[CAM_STATUS_NIGHTMODE][osd_menu_offset+19] = '<';
        osd_buf[CAM_STATUS_NIGHTMODE][osd_menu_offset+29] = '>';
    }
}


void camMenuInit()
{
    uint8_t i = 0;
    
    memset(osd_buf,0x20,sizeof(osd_buf));
    disp_mode = DISPLAY_CMS;
    if(cameraID == 0)
         Cam_Button_ENTER();
    else
    {
        strcpy(osd_buf[i++]+osd_menu_offset+2, "----CAMERA MENU----");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " PROFILE");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " BRIGHTNESS");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " SHARPNESS");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " CONTRAST");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " SATURATION");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " WB MODE");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " WB RED");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " WB BLUE");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " HV FLIP");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " MAX GAIN");
        strcpy(osd_buf[i++]+osd_menu_offset+2, " ASPECT RATIO");
        if(cameraID == RUNCAM_MICRO_V1)
            strcpy(osd_buf[i-1]+osd_menu_offset+19, "NOT SUPPORT");
        strcpy(osd_buf[i++]+osd_menu_offset+2,  " RESET");
        strcpy(osd_buf[i++]+osd_menu_offset+2,  " EXIT");
        strcpy(osd_buf[i++]+osd_menu_offset+2,  " SAVE&EXIT");

        camMenuDrawBracket();
        if(cameraID == RUNCAM_MICRO_V1)
            camMenuStringUpdate(CAM_STATUS_BRIGHTNESS);
        else
            camMenuStringUpdate(CAM_STATUS_PROFILE);
        
    }
}

void camMenuSetVdoRatioInit()
{
    
    memset(osd_buf,0x20,sizeof(osd_buf));
    strcpy(osd_buf[0]+osd_menu_offset+2, "--VIDEO FORMAT--");
    strcpy(osd_buf[1]+osd_menu_offset+3, "SET 720P 4:3  DEFAULT");
    strcpy(osd_buf[2]+osd_menu_offset+3, "SET 720P 16:9 CROP");
    strcpy(osd_buf[3]+osd_menu_offset+3, "SET 720P 16:9 FULL");
    strcpy(osd_buf[4]+osd_menu_offset+3, "RETURN");
    osd_buf[1][osd_menu_offset+2] = '>';
}

const char *cam_names[] = {"   MICRO V2", "    NANO V2", "  NANO LITE"};
const char *low_med_high_strs[] = {"   LOW", "MEDIUM", "  HIGH"};
const char *off_on_strs[] = {"  OFF", "   ON"};

void camMenuStringUpdate(uint8_t status) {
    uint8_t i;
    uint8_t Str[3];
    int8_t dat;

    // Pointer
    for (i = CAM_STATUS_PROFILE; i <= CAM_STATUS_SAVE_EXIT; i++) {
        if (i == status) {
            osd_buf[i][osd_menu_offset + 2] = '>';
        } else
            osd_buf[i][osd_menu_offset + 2] = ' ';
    }
    if (status > CAM_STATUS_SAVE_EXIT) {
        for (i = CAM_STATUS_SET_VDO_RATIO_4_3; i <= CAM_STATUS_SET_VDO_RATIO_RETURN; i++) {
            if (i == status) {
                osd_buf[i - CAM_STATUS_SAVE_EXIT][osd_menu_offset + 2] = '>';
            } else
                osd_buf[i - CAM_STATUS_SAVE_EXIT][osd_menu_offset + 2] = ' ';
        }
    }

    if (status > CAM_STATUS_SAVE_EXIT)
        return;

    // Profile
    if (cameraID == RUNCAM_MICRO_V1)
        strcpy(osd_buf[CAM_STATUS_PROFILE] + osd_menu_offset + 18, "   MICRO V1");
    else
        strcpy(osd_buf[CAM_STATUS_PROFILE] + osd_menu_offset + 18, cam_names[camProfile_Menu % 3]);

    // Brightness
    dat = (int8_t)CAM_BRIGHTNESS_INITIAL - (int8_t)camCfg_Menu.brightness;
    if (dat > 0) {
        osd_buf[CAM_STATUS_BRIGHTNESS][osd_menu_offset + 24] = '-';
    } else if (dat < 0) {
        osd_buf[CAM_STATUS_BRIGHTNESS][osd_menu_offset + 24] = '+';
        dat = 0 - dat;
    } else {
        osd_buf[CAM_STATUS_BRIGHTNESS][osd_menu_offset + 24] = ' ';
    }
    uint8ToString(dat, Str);
    osd_buf[CAM_STATUS_BRIGHTNESS][osd_menu_offset + 25] = Str[1];
    osd_buf[CAM_STATUS_BRIGHTNESS][osd_menu_offset + 26] = Str[2];

    // sharpness

    // TODO: If these values are always guaranteed to be within boundaries, eliminating the % and == operations will net us another 100 bytes or so
    strcpy(osd_buf[CAM_STATUS_SHARPNESS] + osd_menu_offset + 21, low_med_high_strs[camCfg_Menu.sharpness % 3]);

    // saturation
    strcpy(osd_buf[CAM_STATUS_SATURATION] + osd_menu_offset + 21, "LEVEL");
    osd_buf[CAM_STATUS_SATURATION][osd_menu_offset + 26] = '1' + camCfg_Menu.saturation;

    // contrast
    strcpy(osd_buf[CAM_STATUS_CONTRAST] + osd_menu_offset + 21, low_med_high_strs[camCfg_Menu.contrast % 3]);

    // hvFlip
    if (cameraID == RUNCAM_MICRO_V1) {
        strcpy(osd_buf[CAM_STATUS_HVFLIP] + osd_menu_offset + 19, "NOT SUPPORT");
    } else { // if(cameraID == RUNCAM_MICRO_V2)
        strcpy(osd_buf[CAM_STATUS_HVFLIP] + osd_menu_offset + 21, off_on_strs[(uint8_t)(camCfg_Menu.hvFlip != 0)]);
    }

    // nightMode
    if (cameraID == RUNCAM_MICRO_V1) {
        strcpy(osd_buf[CAM_STATUS_NIGHTMODE] + osd_menu_offset + 19, "NOT SUPPORT");
    } else { // cameraID == RUNCAM_MICRO_V2
        strcpy(osd_buf[CAM_STATUS_NIGHTMODE] + osd_menu_offset + 21, off_on_strs[(uint8_t)(camCfg_Menu.nightMode != 0)]);
    }

    // wb
    if (camCfg_Menu.wbMode == 0) {
        strcpy(osd_buf[CAM_STATUS_WBMODE] + osd_menu_offset + 21, "  AUTO  ");
        strcpy(osd_buf[CAM_STATUS_WBRED] + osd_menu_offset + 21, "        ");
        strcpy(osd_buf[CAM_STATUS_WBBLUE] + osd_menu_offset + 21, "        ");
    } else {
        strcpy(osd_buf[CAM_STATUS_WBMODE] + osd_menu_offset + 21, "MANUAL ");
        osd_buf[CAM_STATUS_WBMODE][osd_menu_offset + 27] = '0' + camCfg_Menu.wbMode;
        uint8ToString(camCfg_Menu.wbRed[camCfg_Menu.wbMode - 1], Str);
        strcpy(osd_buf[CAM_STATUS_WBRED] + osd_menu_offset + 24, Str);
        strcpy(osd_buf[CAM_STATUS_WBRED] + osd_menu_offset + 27, "  ");
        uint8ToString(camCfg_Menu.wbBlue[camCfg_Menu.wbMode - 1], Str);
        strcpy(osd_buf[CAM_STATUS_WBBLUE] + osd_menu_offset + 24, Str);
        strcpy(osd_buf[CAM_STATUS_WBBLUE] + osd_menu_offset + 27, "  ");
    }

    camMenuDrawBracket();
}