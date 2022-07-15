#include "common.h"
#include "sfr_ext.h"
#include "global.h"
#include "uart.h"
#include "print.h"
#include "monitor.h"
#include "isr.h"
#include "hardware.h"
#include "i2c.h"
#include "i2c_device.h"
#include "spi.h"
#include "dm6300.h"
#include "smartaudio_protocol.h"
#include "msp_displayport.h"
#include "camera.h"
#include "lifetime.h"

uint8_t KEYBOARD_ON = 0; //avoid conflict between keyboard and cam_control
uint8_t EE_VALID = 0;

#ifdef VTX_L
uint8_t powerLock = 1;
#endif
/**********************************
//
//  POWER MODE
//
//    VTX_L
//    0------------25mW   (14dBm)
//    1------------200mW  (23dBm)
//    2------------500mW  (27dBm)
//    3------------1000mW (30dBm)

//    VTX_R, VTX_S
//    0------------25mW (14dBm)
//    1------------200mW(23dBm)
//    2------------500mW(27dBm)
**********************************/
uint8_t RF_POWER = 0;
uint8_t RF_FREQ = 0;
uint8_t LP_MODE = 0;
uint8_t PIT_MODE = 0;
uint8_t OFFSET_25MW = 0; // 0~10 -> 0~10    11~20 -> -1~-10

uint8_t RF_BW = BW_27M;

uint8_t g_IS_ARMED = 0;
uint8_t g_IS_PARALYZE = 0;

uint8_t cfg_step = 0;   // 0:idle, 1:freq, 2:power, 3:LP_MODE

uint8_t cfg_to_cnt = 0;
uint8_t pwr_lmt_done = 0;
uint8_t pwr_lmt_sec = 0;
int16_t temperature = 0;
uint8_t pwr_offset = 0;
uint8_t heat_protect = 0;

uint8_t last_SA_lock = 0; 
uint8_t cur_pwr = 0;

uint8_t led_status = 0;

uint8_t temp_err = 0;
#ifdef VTX_L
int16_t temp0 = 0;
uint8_t p;
#else
#endif

uint8_t i = 0;

uint8_t BPLED[] = {
    0xC0, // 0
    0xF9, // 1
    0xA4, // 2
    0xB0, // 3
    0x99, // 4
    0x92, // 5
    0x82, // 6
    0xF8, // 7
    0x80, // 8
    0x90, // 9
    0x88, // A
    0x83, // b
    0xC6, // C
    0x8E, // F
    0x0E  // F.
    };

uint8_t dispF_cnt = 0xff;
    
void Set_720P50(uint8_t page)
{
    WriteReg(page, 0x21, 0x25);
    
    WriteReg(page, 0x40, 0x00);
    WriteReg(page, 0x41, 0x25);
    WriteReg(page, 0x42, 0xD0);
    WriteReg(page, 0x43, 0xBC);
    WriteReg(page, 0x44, 0x47);
    WriteReg(page, 0x45, 0xEE);
    WriteReg(page, 0x49, 0x04);
    WriteReg(page, 0x4c, 0x19);
    WriteReg(page, 0x4f, 0x00);
    WriteReg(page, 0x52, 0x04);
    WriteReg(page, 0x53, 0x00);
    WriteReg(page, 0x54, 0x3C);
                   
    WriteReg(page, 0x06, 0x01);
}

void Set_720P60(uint8_t page)
{
    WriteReg(page, 0x21, 0x1F);
    
    WriteReg(page, 0x40, 0x00);
    WriteReg(page, 0x41, 0x25);
    WriteReg(page, 0x42, 0xD0);
    WriteReg(page, 0x43, 0x72);
    WriteReg(page, 0x44, 0x46);
    WriteReg(page, 0x45, 0xEE);
    WriteReg(page, 0x49, 0x04);
    WriteReg(page, 0x4c, 0x19);
    WriteReg(page, 0x4f, 0x00);
    WriteReg(page, 0x52, 0x04);
    WriteReg(page, 0x53, 0x00);
    WriteReg(page, 0x54, 0x3C);
    
    WriteReg(page, 0x06, 0x01);
}

void Set_720P30(uint8_t page, uint8_t is_43)
{
    if(is_43)
        WriteReg(page, 0x21, 0x28);
    else
        WriteReg(page, 0x21, 0x1F);
    
    WriteReg(page, 0x40, 0x00);
    WriteReg(page, 0x41, 0x25);
    WriteReg(page, 0x42, 0xB0);
    WriteReg(page, 0x43, 0xD4);
    WriteReg(page, 0x44, 0x88);
    WriteReg(page, 0x45, 0x47);
    WriteReg(page, 0x49, 0x04);
    WriteReg(page, 0x4c, 0x19);
    WriteReg(page, 0x4f, 0x00);
    WriteReg(page, 0x52, 0x04);
    WriteReg(page, 0x53, 0x00);
    WriteReg(page, 0x54, 0x3C);
    
    WriteReg(page, 0x06, 0x01);
}

void Setting_Save()
{
    uint8_t rcv = 0;

    #if(0)
    if(cameraID == 0){
        CAM_WriteCFG(0,RF_FREQ,RF_POWER,MODE);
        CAM_WriteCFG(0,RF_FREQ,RF_POWER,MODE);
        CAM_WriteCFG(0,RF_FREQ,RF_POWER,MODE);
    }
    #endif
    
    if(EE_VALID){
        WAIT(10);
        rcv |= I2C_Write(ADDR_EEPROM, EEP_ADDR_RF_FREQ, RF_FREQ, 0, 0);
        WAIT(10);
        rcv |= I2C_Write(ADDR_EEPROM, EEP_ADDR_RF_POWER, RF_POWER, 0, 0);
        WAIT(10);
        rcv |= I2C_Write(ADDR_EEPROM, EEP_ADDR_LPMODE, LP_MODE, 0, 0);
        WAIT(10);
        rcv |= I2C_Write(ADDR_EEPROM, EEP_ADDR_PITMODE, PIT_MODE, 0, 0);
        WAIT(10);
        rcv |= I2C_Write(ADDR_EEPROM, EEP_ADDR_25MW, OFFSET_25MW, 0, 0);
        #ifdef _DEBUG_MODE
        if(!rcv)
            Printf("\r\nEEPROM write success");
        #endif
    }

    #ifdef _DEBUG_MODE
    Printf("\r\nSetting Save:  RF_FREQ=%d, RF_POWER=%d, LP_MODE=%d, PIT_MODE=%d",
        (uint16_t)RF_FREQ, (uint16_t)RF_POWER, (uint16_t)LP_MODE, (uint16_t)PIT_MODE);
    #endif
}

void CFG_Back()
{
    RF_FREQ = (RF_FREQ > FREQ_MAX_EXT) ? 0 : RF_FREQ;
    RF_POWER = (RF_POWER > POWER_MAX) ? 0 : RF_POWER;
    LP_MODE  = (LP_MODE > 2) ? 0 : LP_MODE;
    PIT_MODE = (PIT_MODE > PIT_0MW) ? PIT_OFF : PIT_MODE;
    OFFSET_25MW = (OFFSET_25MW > 20) ? 0 : OFFSET_25MW;
}

void GetVtxParameter()
{
    unsigned char CODE_SEG *ptr = 0xFFE8;
    uint8_t i, j;
    uint8_t tab[FREQ_MAX+1][POWER_MAX+1];
    uint8_t flash_vld = 1;
    uint8_t ee_vld = 1;

    WAIT(10);
    EE_VALID = !I2C_Write(ADDR_EEPROM, 0x40, 0xFF, 0, 0);
    
    #ifdef _DEBUG_MODE
    Printf("\r\nEE_VALID:%bx",EE_VALID);
    #endif
    
    if(EE_VALID){ // eeprom valid
        
        #ifdef FIX_EEP
        for(i=0;i<=FREQ_MAX;i++) {
            for(j=0;j<=POWER_MAX;j++){
                WAIT(10);I2C_Write(ADDR_EEPROM, i*(POWER_MAX+1) + j, table_power[i][j], 0, 0);
            }
        }
        #endif
        // RF Tab
        for(i=0;i<=FREQ_MAX;i++) {
            for(j=0;j<=POWER_MAX;j++){
                WAIT(10);
                tab[i][j] = I2C_Read(ADDR_EEPROM, i*(POWER_MAX+1) + j, 0, 0);
                if(tab[i][j] == 0xFF)
                    ee_vld = 0;
            }
        }
        
        if(ee_vld){
            #ifdef _DEBUG_MODE
            Printf("\r\nUSE EEPROM for rf_pwr_tab.");
            #endif
            for(i=0;i<=FREQ_MAX;i++) {
                for(j=0;j<=POWER_MAX;j++){
                    table_power[i][j] = tab[i][j];
                    #ifndef _RF_CALIB
                    if(j==0)     //25mw +3dbm
                        table_power[i][j] +=  0xC;
                    #endif
                }
            }
            
            //ch9 5760M use ch4 5769M;ch10 5800 use ch5 5806
            for(j=0;j<=POWER_MAX;j++){
                table_power[8][j] = tab[3][j];
                table_power[9][j] = tab[4][j];
                if(j==0){     //25mw +3dbm
                    table_power[8][j] += 0x0C;
                    table_power[9][j] += 0x0C;
                }
            }
        }else{
            #ifdef _DEBUG_MODE
            Printf("\r\nEEPROM is NOT initialized. Use default rf_pwr_tab.");
            #endif
            
            #ifdef _RF_CALIB
            for(i=0;i<=FREQ_MAX;i++) {
                for(j=0;j<=POWER_MAX;j++){
                    WAIT(10);I2C_Write(ADDR_EEPROM, i*(POWER_MAX+1) + j, table_power[i][j], 0, 0);
                }
            }
            #endif
        }
        
        // VTX Setting
        RF_FREQ = I2C_Read(ADDR_EEPROM, EEP_ADDR_RF_FREQ, 0, 0);
        RF_POWER = I2C_Read(ADDR_EEPROM, EEP_ADDR_RF_POWER, 0, 0);
        LP_MODE = I2C_Read(ADDR_EEPROM, EEP_ADDR_LPMODE, 0, 0);
        PIT_MODE = I2C_Read(ADDR_EEPROM, EEP_ADDR_PITMODE, 0, 0);
        OFFSET_25MW = I2C_Read(ADDR_EEPROM, EEP_ADDR_25MW, 0, 0);
        
        if(RF_FREQ == 0xff || RF_POWER == 0xff || LP_MODE == 0xff || PIT_MODE == 0xff || OFFSET_25MW == 0xff){
            CFG_Back();
            WAIT(10);I2C_Write(ADDR_EEPROM, EEP_ADDR_RF_FREQ, RF_FREQ, 0, 0);
            WAIT(10);I2C_Write(ADDR_EEPROM, EEP_ADDR_RF_POWER, RF_POWER, 0, 0);
            WAIT(10);I2C_Write(ADDR_EEPROM, EEP_ADDR_LPMODE, LP_MODE, 0, 0);
            WAIT(10);I2C_Write(ADDR_EEPROM, EEP_ADDR_PITMODE, PIT_MODE, 0, 0);
            WAIT(10);I2C_Write(ADDR_EEPROM, EEP_ADDR_25MW, OFFSET_25MW, 0, 0);
            #ifdef _DEBUG_MODE
            Printf("\r\nEEPROM is NOT initialized. USE CAM for VTX setting.");
            #endif
        }else{
            CFG_Back();
            #ifdef _DEBUG_MODE
            Printf("\r\nUSE EEPROM for VTX setting:RF_FREQ=%d, RF_POWER=%d, LPMODE=%d PIT_MODE=%d", (uint16_t)RF_FREQ, (uint16_t)RF_POWER, (uint16_t)LP_MODE, (uint16_t)PIT_MODE);
            #endif
        }
        
        //last_SA_lock
        #ifdef USE_SMARTAUDIO
        WAIT(10);
        last_SA_lock = I2C_Read(ADDR_EEPROM, EEP_ADDR_SA_LOCK, 0, 0);
        WAIT(10);
        if(last_SA_lock == 0xff){
            last_SA_lock = 0;
            I2C_Write(ADDR_EEPROM, EEP_ADDR_SA_LOCK, last_SA_lock, 0, 0);
        }
        #ifdef _DEBUG_MODE
        Printf("\r\nlast_SA_lock %bx",last_SA_lock);
        #endif
        #endif

        #ifdef VTX_L
        //powerLock
        WAIT(10); powerLock = 0x01 & I2C_Read(ADDR_EEPROM, EEP_ADDR_POWER_LOCK, 0, 0);
        #endif
    }
    else{
        for(i=0;i<=FREQ_MAX;i++) {
            for(j=0;j<=POWER_MAX;j++){
                tab[i][j] = *ptr;
                ptr++;
                if(tab[i][j] == 0xFF)
                    flash_vld = 0;
            }
        }

        if(flash_vld){ // flash valid
            #ifdef _DEBUG_MODE
            Printf("\r\nUse Flash rf_pwr_tab space.");
            #endif
            for(i=0;i<=FREQ_MAX;i++) {
                for(j=0;j<=POWER_MAX;j++) {
                    table_power[i][j] = tab[i][j];
                    if(j==0)
                        table_power[i][j] += 0x0c;
                }
            }
            
            //ch9 5760M use ch4 5769M;ch10 5800 use ch5 5806
            for(j=0;j<=POWER_MAX;j++){
                table_power[8][j] = tab[3][j];
                table_power[9][j] = tab[4][j];
                if(j==0){     //25mw +3dbm
                    table_power[8][j] += 0x0C;
                    table_power[9][j] += 0x0C;
                }
            }
        }
        #ifdef _DEBUG_MODE
        else
            Printf("\r\nUse default rf_pwr_tab.");
        
        Printf("\r\nUSE CAM for VTX setting.");
        #endif
    }
    
    #ifdef _DEBUG_MODE
    for(i=0; i<=FREQ_MAX_EXT; i++){
        Printf("\r\nrf_pwr_tab[%bd]=",i);
        for(j=0;j<=POWER_MAX;j++)
            Printf(" %bx", table_power[i][j]);
    }
    Printf("\r\nUSE EEPROM for VTX setting:RF_FREQ=%d, RF_POWER=%d, LPMODE=%d PIT_MODE=%d", (uint16_t)RF_FREQ, (uint16_t)RF_POWER, (uint16_t)LP_MODE, (uint16_t)PIT_MODE);
    #endif
}

void Init_6300RF(uint8_t freq, uint8_t pwr)
{
    WriteReg(0, 0x8F, 0x00);
    WriteReg(0, 0x8F, 0x01);
    DM6300_Init(freq, RF_BW);
    DM6300_SetChannel(freq);
#ifndef VIDEO_PAT
#ifdef VTX_L
    if((pwr == 3) && (!g_IS_ARMED))
        pwr_lmt_done = 0;
    else 
#endif
#endif
	{
	    DM6300_SetPower(pwr, freq, pwr_offset);
	    cur_pwr = pwr;
	}
	WriteReg(0, 0x8F, 0x11);
}

void Init_HW()
{
//--------- gpio init -----------------
    SPI_Init();

#ifdef VIDEO_PAT
    Set_720P60(0);
    WriteReg(0, 0x50, 0x01);
    RF_FREQ = 0;
//--------- eeprom --------------------
    GetVtxParameter();

#ifdef _RF_CALIB
    RF_POWER = 0;   //max power
    RF_FREQ  = 0;   //ch1
#else
    RF_POWER = 0;
#endif
    
    Init_6300RF(RF_FREQ, RF_POWER);
    DM6300_AUXADC_Calib();
#else
    #ifdef VTX_L
        LED_TC3587_Init();
    #endif
//--------- eeprom --------------------
    GetVtxParameter();
    Get_EEP_LifeTime();
//---------- Camera ---------------
    CameraInit();
//--------- dm6300 --------------------
    if(last_SA_lock == 0){
        if(PIT_MODE == PIT_0MW){
        /*
            pwr_lmt_done = 1;
            RF_POWER = POWER_MAX + 2;
            cur_pwr = POWER_MAX + 2;
            vtx_pit = PIT_0MW;
        }else if(PIT_MODE == PIT_P1MW)
        */
            Init_6300RF(RF_FREQ, POWER_MAX+1);
        }
        else{
            WriteReg(0, 0x8F, 0x00);
            WriteReg(0, 0x8F, 0x01);
            DM6300_Init(RF_FREQ, RF_BW);
            DM6300_SetChannel(RF_FREQ);
            DM6300_SetPower(0, RF_FREQ, 0);
            cur_pwr = RF_POWER;
            WriteReg(0, 0x8F, 0x11);
        }
        
        DM6300_AUXADC_Calib();
    }else{
        ch_init = RF_FREQ;
        if(PIT_MODE)
            pwr_init = POWER_MAX+1;
        else
            pwr_init = 0;
    }
#endif

    
//---------- DC IQ --------------------
    //WriteReg(0, 0x25, 0xf6);
    //WriteReg(0, 0x26, 0x00);
}
#ifdef VTX_L
void TempDetect()
{
    static uint8_t init = 1;
    int16_t temp_new,temp_new0; 
    
    if(temp_tflg){
        temp_tflg = 0;
        
        if(init){
            init = 0;
            temp0 = DM6300_GetTemp();
            temp0 <<= 2;
            
            temperature = I2C_Read(ADDR_TEMPADC, 0, 0, 0); //NCT75 MSB 8bit
            //temperature >>= 5; //LM75AD

            if(temperature >= 0x7D) //MAX +125 
                temperature = 0x7D;
            temperature <<= 2; //filter * 4           
        }
        else{
            temp_new0 = DM6300_GetTemp();
            
            temp_new = I2C_Read(ADDR_TEMPADC, 0, 0, 0);
            if(temp_new >= 0x7D) //MAX +125 
                temp_new = 0x7D;            
            //temp_new >>= 5; //LM75AD
            
            temperature = temperature - (temperature>>2) + temp_new;
            
            temp0 = temp0 - (temp0>>2) + temp_new0;
            
            #ifdef _DEBUG_MODE
            if(verbose){
                //Printf("\r\ntempADC  detect: temp = %d, temp_new = %d", (uint16_t)(temperature>>2), (uint16_t)temp_new);
                //Printf("\r\ntemp6300 detect: temp = 0x%x, temp_new = 0x%x", (uint16_t)(temp0>>2), (uint16_t)temp_new0);
            }
            #endif
              
        }
    }
}
#else
void TempDetect()
{
    static uint8_t init = 1;
    int16_t temp_new;
    
    if(temp_tflg){
        temp_tflg = 0;
        
        if(init){
            init = 0;
            temperature = DM6300_GetTemp();
            temperature <<= 2;
        }
        else{
            temp_new = DM6300_GetTemp();
            temperature = temperature - (temperature>>2) + temp_new;
            
            #ifdef _DEBUG_MODE
            //if(verbose)
                //Printf("\r\ntemp detect: temp = %x, temp_new = %x", (uint16_t)(temperature>>2), (uint16_t)temp_new);
            #endif
        }
    }
}
#endif


#ifdef VTX_L
void PowerAutoSwitch()
{
    int16_t temp;
    static uint8_t last_ofs = 0;
	
    if(pwr_sflg)
        pwr_sflg = 0;
    else
        return;
	
    temp = temperature >> 2;  //ADC temp
    last_ofs = pwr_offset;
    
    if(temp < 25)      pwr_offset = 0;  
    else if(temp < 30) pwr_offset = 1;
    else if(temp < 35) pwr_offset = 2;
    else if(temp < 38) pwr_offset = 3;
    else if(temp < 40) pwr_offset = 4;
    else if(temp < 43) pwr_offset = 5;  
    else if(temp < 45) pwr_offset = 6;
    else if(temp < 48) pwr_offset = 7;  
    else if(temp < 50) pwr_offset = 8;
    else if(temp < 53) pwr_offset = 9;
    else if(temp < 55) pwr_offset = 10; 
    else if(temp < 58) pwr_offset = 11;
    else if(temp < 60) pwr_offset = 12;
    else if(temp < 63) pwr_offset = 13;
    else if(temp < 65) pwr_offset = 14; 
    else if(temp < 68) pwr_offset = 15;
    else if(temp < 70) pwr_offset = 16; 
    else if(temp < 73) pwr_offset = 17;
    else if(temp < 75) pwr_offset = 18;
    else if(temp < 78) pwr_offset = 19;
    else if(temp < 80) pwr_offset = 20;
    else               pwr_offset = 20;

    if((!g_IS_ARMED) && (last_ofs == pwr_offset));
    else {
    	DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
        #ifdef _DEBUG_MODE
        Printf("\r\nPowerAutoSwitch: temp = %bx%bx, ",(uint8_t)(temp>>8),(uint8_t)(temp&0xff));
        Printf("pwr_offset = %d", (uint16_t)pwr_offset);
        #endif
        cur_pwr = RF_POWER;
    }
}
#else
void PowerAutoSwitch()
{
    int16_t temp;
    static uint8_t last_ofs = 0;
	
    if(pwr_sflg)
        pwr_sflg = 0;
    else
        return;
	
    temp = temperature >> 2;
    
    if(RF_POWER == 0){
        if(temp < 0x479)      pwr_offset = 0;
        else if(temp < 0x488) pwr_offset = 1;
        else if(temp < 0x4A2) pwr_offset = 2;
        else if(temp < 0x4B5) pwr_offset = 3;
        else if(temp < 0x4C7) pwr_offset = 4;
        else if(temp < 0x4DD) pwr_offset = 5;
        else if(temp < 0x4F0) pwr_offset = 6;
        else if(temp < 0x500) pwr_offset = 7;
        else if(temp < 0x512) pwr_offset = 8;
        else if(temp < 0x524) pwr_offset = 9;
        else if(temp < 0x534) pwr_offset = 10;
        else if(temp < 0x544) pwr_offset = 11;
        else if(temp < 0x554) pwr_offset = 12;
        else if(temp < 0x564) pwr_offset = 13;
        else if(temp < 0x574) pwr_offset = 14;
        else if(temp < 0x582) pwr_offset = 15;
        else if(temp < 0x592) pwr_offset = 16;
        else if(temp < 0x59D) pwr_offset = 17;
        else if(temp < 0x5AC) pwr_offset = 18;
        else                  pwr_offset = 19;
    }
    else {
        if(temp < 0x479)      pwr_offset = 0;
        else if(temp < 0x488) pwr_offset = 1;
        else if(temp < 0x4A2) pwr_offset = 2;
        else if(temp < 0x4B5) pwr_offset = 3;
        else if(temp < 0x4C7) pwr_offset = 4;
        else if(temp < 0x4DD) pwr_offset = 5;
        else if(temp < 0x4F0) pwr_offset = 6;
        else if(temp < 0x500) pwr_offset = 7;
        else if(temp < 0x512) pwr_offset = 8;
        else if(temp < 0x523) pwr_offset = 9;
        else if(temp < 0x533) pwr_offset = 10;
        else if(temp < 0x543) pwr_offset = 11;
        else if(temp < 0x552) pwr_offset = 12;
        else if(temp < 0x562) pwr_offset = 13;
        else if(temp < 0x572) pwr_offset = 14;
        else if(temp < 0x580) pwr_offset = 15;
        else if(temp < 0x590) pwr_offset = 16;
        else if(temp < 0x59B) pwr_offset = 17;
        else if(temp < 0x5AA) pwr_offset = 18;
        else                  pwr_offset = 19;
    }

    if(temp_err)
        pwr_offset = 10;
    
    if(last_ofs != pwr_offset){
        #ifdef _DEBUG_MODE
        Printf("\r\nPowerAutoSwitch:Yes %x %bx",temp, pwr_offset);
        #endif
        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
        cur_pwr = RF_POWER;
    }else{
        #ifdef _DEBUG_MODE
        Printf("\r\nPowerAutoSwitch: No %x %bx",temp, pwr_offset);
        #endif
    }
    
    last_ofs = pwr_offset;
}
#endif
void HeatProtect()
{
    static uint16_t cur_sec = 0;
    static uint8_t sec = 0;
    static uint8_t cnt = 0;
    int16_t temp;
    
    #ifdef VTX_L
    int16_t temp_max = 0x5A;
    #else
    int16_t temp_max = 0x5C0;
    int16_t temp_err_data = 0x700;
    #endif

    if(!g_IS_ARMED){
        if(heat_protect == 0){
            if(cur_sec != seconds){
                cur_sec = seconds;
                sec++;
                if(sec >= 4){
                    sec = 0;
                    temp = temperature >> 2;
                    //temp = temperature >> 5;  //LM75AD
                    #ifdef _DEBUG_MODE
                    if(verbose)
                    	#ifdef VTX_L
                        Printf("\r\nHeat detect: temp = %d, pwr_offset=%d", (uint16_t)temp, (uint16_t)pwr_offset);
                        #else
                    	Printf("\r\nHeat Protect detect: %x",temp);
                    	#endif
                        
                    #endif
                    
                    #ifdef VTX_L
                    ;
                    #else
                    if(temp > temp_err_data){
                        temp_err = 1;
                        return;
                    }
                    #endif
                    
                    #ifdef VTX_L
                    if(temp >= temp_max){
                    #else
                    if((temp_err== 0) && temp >= temp_max){
                    #endif
                        cnt++;
                        if(cnt == 3){
                            #ifdef _DEBUG_MODE
                            Printf("\r\nHeat Protect.");
                            #endif
                            heat_protect = 1;
                            #ifdef VTX_L
                            WriteReg(0, 0x8F, 0x00);
                            msp_set_vtx_config(POWER_MAX+1, 0);
                            #else
                            DM6300_SetPower(0, RF_FREQ, 0);
                            msp_set_vtx_config(0, 0);
                            #endif
                            cur_pwr = 0;
                            pwr_offset = 0;
                            pwr_lmt_done = pwr_lmt_sec = 0;
                            cnt = 0;
                        }
                    }
                    else cnt = 0;
                }
            }
        }
        else {
            if(cur_sec != seconds){
                cur_sec = seconds;
                led_status = 1-led_status;
                if(led_status == ON)
                    LED_BLUE_ON;
                else
                    LED_BLUE_OFF;
            }
        }
    }
    else{
        if(heat_protect){
            LED_BLUE_OFF;
            led_status = OFF;
        }
        heat_protect = cnt = cur_sec = sec = 0;
    }
}

void PwrLMT()
{
    static uint8_t p_init = 1;
    
    if(cur_pwr > POWER_MAX)
        return;
    
#ifndef _RF_CALIB
    if(SA_lock){          //Smart Audio
        HeatProtect();
        if(!heat_protect){
            if(pwr_lmt_done == 0){
                if(pwr_tflg){
                    pwr_tflg = 0;
                    pwr_lmt_sec++;
                    
                    #ifdef VTX_L
                    //test: power plus every sec 
                    if(pwr_lmt_sec >= 3){
                        if(RF_POWER == 3){
                            if(p_init){
                                p = table_power[RF_FREQ][3] - 0x1C;
                                p_init = 0;
                            }
                                
                            SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
                            
                            if(p >= table_power[RF_FREQ][3]){
                                p = table_power[RF_FREQ][3];
                            }else{
                                p += 0x4;
#ifdef _DEBUG_MODE
                                Printf("\r\npwr_plus 2dbm,p=%bx",(uint8_t)p);
#endif
                            }
                            
                            SPI_Write(0x3, 0xD1C, 0x00000000, (uint32_t)p);
                            SPI_Write(0x3, 0x330, 0x00000000, 0x31F); //1W
                        }
                    }
                    #endif
                        
                    #ifdef _DEBUG_MODE
                    Printf("\r\npwr_lmt_sec %bx",pwr_lmt_sec);
                    #endif
                    if(pwr_lmt_sec >= PWR_LMT_SEC){
                        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                        cur_pwr = RF_POWER;
                        pwr_lmt_done = 1;
                        pwr_lmt_sec = 0;
                        //test: power init reset   
                        p_init = 1;
                        
                        #ifdef _DEBUG_MODE
                        Printf("\r\nPower limit done.");
                        Prompt();
                        #endif
                    }
                }
            }
            else{
                PowerAutoSwitch();
            }
        }
    }
    /*
    else if(!fc_lock){     //No FC connected
        HeatProtect();
        if(!heat_protect && seconds > 5)
            PowerAutoSwitch();
    }
    */
    else if(g_IS_ARMED){    //Armed
        PowerAutoSwitch();
    }
    else{                   //Disarmed
        if(PIT_MODE){
            /*if(cur_pwr == 0mW)
                set 0mW;
            else
                set 0.1mW;*/
        }
        else if(LP_MODE){
            //DM6300_SetPower(0, RF_FREQ, 0);
            //cur_pwr = 0;
        }
        else{
            HeatProtect();
            if(!heat_protect){
                if(pwr_lmt_done == 0){
                    if(pwr_tflg){
                        pwr_tflg = 0;
                        pwr_lmt_sec++;
                        
                        #ifdef VTX_L
                        //test: power plus every sec 
                        if(pwr_lmt_sec >= 3){
                            if(RF_POWER == 3){
                                if(p_init){
                                    p = table_power[RF_FREQ][3] - 0x1C;
                                    p_init = 0;
                                }
                                SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018); //set page
                                
                                if(p >= table_power[RF_FREQ][3])
                                    p = table_power[RF_FREQ][3];
                                else{ 
                                    p += 0x4;
#ifdef _DEBUG_MODE
                                    Printf("\r\npwr_plus 2dbm,p=%bx",(uint8_t)p + pwr_offset);
#endif
                                }
                                SPI_Write(0x3, 0xD1C, 0x00000000, (uint32_t)(p+pwr_offset)); //digital offset
                                SPI_Write(0x3, 0x330, 0x00000000, 0x31F); // analog offset 1W
                            }
                        }
                        #endif//VTX_L
                            
                        #ifdef _DEBUG_MODE
                        Printf("\r\npwr_lmt_sec %bx",pwr_lmt_sec);
                        #endif
                        if(pwr_lmt_sec >= PWR_LMT_SEC){
                            DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                            cur_pwr = RF_POWER;
                            pwr_lmt_done = 1;
                            pwr_lmt_sec = 0;
                            //test: power init reset   
                            p_init = 1;
                            
                            #ifdef _DEBUG_MODE
                            Printf("\r\nPower limit done.");
                            Prompt();
                            #endif
                        }
                    }
                }
                else{
                    PowerAutoSwitch();
                }
            }
        }
    }
#else
    PowerAutoSwitch();
#endif
}

void Flicker_LED(uint8_t n)
{
    uint8_t i;
    for(i=0; i<n; i++){
    	LED_BLUE_OFF;
        WAIT(120);
        LED_BLUE_ON;
        WAIT(120);
    }
    led_status = ON;
}

void Video_Detect()
{
    static uint16_t last_sec = 0;
    static uint8_t sec = 0;
    uint8_t vdet;
    
    if(last_sec != seconds){
        last_sec = seconds;
        sec++;

        if(heat_protect)
            return;

        if(cameraID){
        	LED_BLUE_ON;
        	led_status = ON;
            return;
        }
        
        vdet = ReadReg(0, 0x02) >> 4;
            
        led_status = vdet;
        if(led_status == ON)
            LED_BLUE_ON;
        else
            LED_BLUE_OFF;
            
        if(sec == 3){
            sec = 0;
            if(vdet){ // video loss
                if(CAM_MODE == CAM_720P50){
                    Set_720P60(IS_RX);
                    CAM_MODE = CAM_720P60;
                }
                else if(CAM_MODE == CAM_720P60){
                    Set_720P50(IS_RX);
                    CAM_MODE = CAM_720P50;
                }
            }
        }
    }
}

void Imp_RF_Param()
{
    DM6300_SetChannel(RF_FREQ);
    if(LP_MODE && !g_IS_ARMED)
        return;
#ifndef VIDEO_PAT
#ifdef VTX_L
    if(RF_POWER == 3 && !g_IS_ARMED)
        pwr_lmt_done = 0;
    else
#endif
#endif
    DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
    cur_pwr = RF_POWER;
}

void Button1_SP()
{
#ifdef _DEBUG_MODE
    Printf("\r\nButton1_SP.");
#endif
    cfg_to_cnt = 0;
    switch(cfg_step){
        case 0:
            cfg_step = 1;
            if((RF_FREQ == 8) || (RF_FREQ==9))
                dispF_cnt = 0;
            CFG_Back();
        
            //exit 0mW
            if(vtx_pit_save == PIT_0MW){
                #ifdef _DEBUG_MODE
                Printf("\n\rcfg_step(0),DM6300 init");
                #endif
                Init_6300RF(RF_FREQ, RF_POWER);
                DM6300_AUXADC_Calib();
                cur_pwr = RF_POWER;
            }
            //reset pitmode
            vtx_pit_save = PIT_OFF;
            PIT_MODE = PIT_OFF;
            break;
        case 1:
            //exit 0mW
            if(vtx_pit_save == PIT_0MW){
                #ifdef _DEBUG_MODE
                Printf("\n\rcfg_step(1),DM6300 init");
                #endif
                Init_6300RF(RF_FREQ, RF_POWER);
                DM6300_AUXADC_Calib();
                cur_pwr = RF_POWER;
            }
            //reset pitmode
            vtx_pit_save = PIT_OFF;
            PIT_MODE = PIT_OFF;
            
            if(RF_FREQ >= FREQ_MAX_EXT) RF_FREQ = 0;
            else RF_FREQ++;
        
            if((RF_FREQ == 8) || (RF_FREQ == 9))
                dispF_cnt = 0;
            Imp_RF_Param();
            Setting_Save();
            msp_set_vtx_config(RF_POWER, 1);
            
            if(LP_MODE){
                cur_pwr = 0;
                DM6300_SetPower(0, RF_FREQ, 0);
            }else{
                DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                cur_pwr = RF_POWER;
            }
            break;
        case 2:
            if(vtx_pit_save == PIT_0MW){
                //exit 0mW
                #ifdef _DEBUG_MODE
                Printf("\n\rcfg_step(2),DM6300 init");
                #endif
                Init_6300RF(RF_FREQ, RF_POWER);
                DM6300_AUXADC_Calib();
                cur_pwr = RF_POWER;
            }
            //reset pitmode
            vtx_pit_save = PIT_OFF;
            PIT_MODE = PIT_OFF;
            
            if(RF_POWER >= POWER_MAX) RF_POWER = 0;
            else RF_POWER++;
            #ifdef VTX_L
            if(powerLock)
                RF_POWER &= 0x01;
            #endif
            Imp_RF_Param();
            Setting_Save();
            
            msp_set_vtx_config(RF_POWER, 1);
            if(LP_MODE){ // limit power to 25mW
                DM6300_SetPower(0, RF_FREQ, 0);
                cur_pwr = 0;
            }else{
            #ifndef VIDEO_PAT
            #ifdef VTX_L
                if(RF_POWER == 3 && !g_IS_ARMED)
                    pwr_lmt_done = 0;
                else
            #endif
            #endif
                {
                DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                cur_pwr = RF_POWER;
            	}
            }
            break;
        case 3:
            if(vtx_pit_save == PIT_0MW){
                //exit 0mW
                #ifdef _DEBUG_MODE
                Printf("\n\rcfg_step(3),DM6300 init");
                #endif
                Init_6300RF(RF_FREQ, RF_POWER);
                DM6300_AUXADC_Calib();
                cur_pwr = RF_POWER;
            }
            //reset pitmode
            vtx_pit_save = PIT_OFF;
            PIT_MODE = PIT_OFF;

            LP_MODE ++;
            if(LP_MODE > 2)
                LP_MODE = 0;

            if(LP_MODE)
            {
                DM6300_SetPower(0, RF_FREQ, 0); // limit power to 25mW
                cur_pwr = 0;
                #ifdef _DEBUG_MODE
                Printf("\n\rEnter LP_MODE");
                #endif
            }
            else
            {
                DM6300_SetPower(RF_POWER, RF_FREQ, 0);
            }

            msp_set_vtx_config(RF_POWER, 1);
        
            Setting_Save();
            break;
    }
    //Printf("\r\nShort Press: cfg_step=%d, RF_FREQ=%d, RF_POWER=%d", (uint16_t)cfg_step, (uint16_t)RF_FREQ, (uint16_t)RF_POWER);
}

void Button1_LP()
{
    #ifdef _DEBUG_MODE
    Printf("\r\nButton1_LP.");
    #endif
    cfg_to_cnt = 0;
    switch(cfg_step){
        case 0:
            cfg_step = 2;
            CFG_Back();
            Init_MAX7315(0xFF);
            break;
        case 1:
            cfg_step = 2;
            Init_MAX7315(0xFF);
            break;
        case 2:
        case 3:
            cfg_step = 0;
            Init_MAX7315(0x00);
            WAIT(100);
            Init_MAX7315(0xFF);
            break;
    }
    //Printf("\r\nShort Press: cfg_step=%d, FREQ_CFG=%d, POWER_CFG=%d", (uint16_t)cfg_step, (uint16_t)FREQ_CFG, (uint16_t)POWER_CFG);
}

void Button1_LLP()
{
    #ifdef _DEBUG_MODE
    Printf("\r\nButton1_LLP.");
    #endif
    cfg_to_cnt = 0;
    if(cfg_step == 0){
        cfg_step = 3;
        CFG_Back();
        Init_MAX7315(0xFF);
    }
}

void Flicker_MAX(uint8_t ch, uint8_t cnt)
{
    uint8_t i;
    for(i=0; i<cnt; i++){
        Init_MAX7315(0xFF);
        WAIT(90);
        Init_MAX7315(ch);
        WAIT(120);
    }
    Init_MAX7315(0xFF);
}

void BlinkPhase()
{
    uint8_t bp;
    
    if(cfg_step == 1 && (dispF_cnt < DISPF_TIME)){    //display 'F' band
        bp = BPLED[14];
        I2C_Write(ADDR_KEYBOARD, 0x01, bp, 0, 0);
        I2C_Write(ADDR_KEYBOARD, 0x03, 0x00, 0, 0);
    }else{
        switch(cfg_step){
            case 0:
                //Init_MAX7315(0xFF);
                break;

            case 1:
                if(RF_FREQ <= 7) 
                    bp = BPLED[RF_FREQ+1];
                else if(RF_FREQ == 8)   //F2
                    bp = BPLED[2];
                else if(RF_FREQ == 9)   //F4
                    bp = BPLED[4];
                I2C_Write(ADDR_KEYBOARD, 0x01, bp, 0, 0);
                I2C_Write(ADDR_KEYBOARD, 0x03, 0x00, 0, 0);
                break;

            case 2:
                bp = BPLED[RF_POWER+1] & 0x7F;
                I2C_Write(ADDR_KEYBOARD, 0x01, bp, 0, 0);
                I2C_Write(ADDR_KEYBOARD, 0x03, 0x00, 0, 0);
                break;

            case 3:
                bp = BPLED[LP_MODE+1];
                I2C_Write(ADDR_KEYBOARD, 0x01, bp, 0, 0);
                I2C_Write(ADDR_KEYBOARD, 0x03, 0x00, 0, 0);
                break;
        }
    }
}

void CFGTimeout()
{
    if(cfg_step != 0){ // timeout
        if(cfg_tflg){
            cfg_tflg = 0;
            cfg_to_cnt++;
            if(cfg_to_cnt >= CFG_TO_SEC){
                CFG_Back();
                cfg_step = 0;
                
                Init_MAX7315(0xFF);
                #ifdef _DEBUG_MODE
                Printf("\r\nCFG Timeout.");
                Prompt();
                #endif
            }
        }
    }
}

void OnButton1()
{
    static uint8_t btn_down = 0;
    static uint8_t sec = 0;
    static uint8_t leave_respond = 0;
    static uint16_t cur_sec = 0;
    static uint8_t last_keyon = 0;
    
    if(cur_sec != seconds){
        cur_sec = seconds;
        
        USE_MAX7315 = !I2C_Write(ADDR_MAX7315, 0x09, 0, 0, 0);
        USE_PCA9554 = !I2C_Write(ADDR_PCA9554, 0x09, 0, 0, 0);
        
        KEYBOARD_ON = USE_MAX7315 | USE_PCA9554;
        if(KEYBOARD_ON && (last_keyon == 0))
            Init_MAX7315(0xFF);
        last_keyon = KEYBOARD_ON;
    }
     
    if(KEYBOARD_ON == 0){
        WriteReg(0, 0xB0, 0x3E);
        
        btn_down = sec = leave_respond = 0;
        cfg_step = 0;
        cfg_to_cnt = 0;
    }else{
        WriteReg(0, 0xB0, 0x3F);
        
        if(btn_down == 0){
            if(BTN_1 == 0){
                WAIT(16);
                if(BTN_1 == 0){
                    btn_down = 1;
                    sec = 0;
                }
            }
        }
        else{
            if(BTN_1){
                if(leave_respond == 2) // long long press
                    Button1_LLP();
                else if(leave_respond == 1) // long press
                    Button1_LP();
                else if(leave_respond == 0) // short press
                    Button1_SP();

                leave_respond = 0;
                btn_down = 0;
                sec = 0;
            }
            else if(btn1_tflg){
                btn1_tflg = 0;
                sec++;
                if(sec == PRESS_L){
                    // enter long
                    if(cfg_step <= 1){
                        Flicker_MAX(0x0C, 3);
                        leave_respond = 1;
                    }
                    else{
                        Button1_LP();
                        leave_respond = 3;
                    }
                }
                else if(sec == PRESS_LL){
                    // enter long long
                    if(cfg_step == 0 && leave_respond != 3){
                        Flicker_MAX(0x47, 3);
                        leave_respond = 2;
                    }
                }
            }
        }
    }
    
    BlinkPhase();
    CFGTimeout();
}
