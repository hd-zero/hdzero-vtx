#include "common.h"
#include "isr.h"
#include "global.h"
#include "uart.h"
#include "print.h"
#include "monitor.h"
#include "sfr_ext.h"
#include "hardware.h"
#include "i2c_device.h"
#include "i2c.h"
#include "rom.h"
#include "msp_displayport.h"
#include "smartaudio_protocol.h"
#include "dm6300.h"
#include "camera.h"
#include "lifetime.h"

uint8_t UNUSED = 0;

void timer_task();
#ifdef USE_SMARTAUDIO
void SA_Delay_init();
#endif

void main(void)
{
    // init
    CPU_init();
    WriteReg(0, 0xB0, 0x3E);
    WriteReg(0, 0xB2, 0x03);
    WriteReg(0, 0x80, 0xC8);
    //WAIT(100);

    #ifdef _DEBUG_MODE
    Printf("\r\n========================================================");
    Printf("\r\n     >>>             Divimath DM568X            <<<     ");
    Printf("\r\n========================================================");
    Printf("\r\n");
    #endif

    
    Init_HW(); // init
    fc_init(); // init displayport
    
    #ifdef USE_SMARTAUDIO
    SA_Init();
    #endif
    
    #ifdef _DEBUG_MODE
    Prompt();
    #endif

    // main loop
    while(1) {

        timer_task();

        #ifdef USE_SMARTAUDIO
        while(SA_task());
        #endif

        #ifdef _RF_CALIB
        CalibProc();
        #elif defined _DEBUG_MODE
        Monitor();
        #endif

        Video_Detect();
        if(!SA_lock) OnButton1();

        if((last_SA_lock && (seconds > WAIT_SA_CONFIG)) || (last_SA_lock == 0)){
            TempDetect(); // temperature dectect
            PwrLMT(); // RF power ctrl
            msp_task(); // msp displayport process
            Update_EEP_LifeTime();
        }
        #ifdef USE_SMARTAUDIO
        SA_Delay_init();
        #endif

    }
}

void timer_task()
{
    static uint16_t cur_ms10x_1sd16 = 0, last_ms10x_1sd16 = 0;
    static uint8_t  timer_cnt = 0;
    cur_ms10x_1sd16 = timer_ms10x;
    if(((cur_ms10x_1sd16 - last_ms10x_1sd16) >= TIMER0_1SD16) || (cur_ms10x_1sd16 < last_ms10x_1sd16)) {
        last_ms10x_1sd16 = cur_ms10x_1sd16;
        timer_cnt++;
        timer_cnt &= 15;
        if(timer_cnt == 15) {   //every second, 1Hz
            btn1_tflg = 1;
            pwr_tflg = 1;
            cfg_tflg = 1;
            seconds++;
            pwr_sflg = 1;
        }

        if((timer_cnt&7) == 7)  //every half second, 2Hz
            temp_tflg = 1;
        
        if((timer_cnt&3)== 3)   //every quater second, 4Hz
            timer_4hz = 1;
        
        if((timer_cnt&1)== 1)   //every octual second, 8Hz
            timer_8hz = 1;
        timer_16hz = 1;
    }
}

#ifdef USE_SMARTAUDIO
void SA_Delay_init()
{
    static uint8_t SA_saved = 0;
    static uint8_t SA_init_done = 0;
    
    if(SA_saved == 0){
        if(seconds >= WAIT_SA_CONFIG){
            I2C_Write(ADDR_EEPROM, EEP_ADDR_SA_LOCK, SA_lock, 0, 0);
            #ifdef _DEBUG_MODE
            Printf("\r\nSave SA_lock(%bx) to EEPROM",SA_lock);
            #endif
            SA_saved = 1;
        }
    }
    
    //init_rf
    if(SA_init_done==0){
        if(last_SA_lock && seconds >= WAIT_SA_CONFIG){
            #ifdef _DEBUG_MODE
            Printf("\r\nInit RF");
            #endif
            pwr_lmt_sec = PWR_LMT_SEC;
            SA_init_done = 1;
            if(SA_lock){
                if(pwr_init == POWER_MAX+2){ //0mW
                    #if(0)
                    SA_dbm = pwr_to_dbm(pwr_init);
                    RF_POWER = POWER_MAX + 1;
                    Init_6300RF(ch_init, RF_POWER);
                    PIT_MODE = PIT_0MW;
                    #ifdef _DEBUG_MODE
                    Printf("\r\n SA_dbm:%bx",SA_dbm);
                    Printf("\r\n ch%bx, pwr%bx", ch_init, pwr_init);
                    #endif
                    #else
                    RF_POWER = POWER_MAX + 2;
                    cur_pwr = POWER_MAX + 2;
                    #endif
                }else if(PIT_MODE){
                    Init_6300RF(ch_init, POWER_MAX + 1);
                    #ifdef _DEBUG_MODE
                    Printf("\r\n ch%bx, pwr%bx", ch_init, cur_pwr);
                    #endif
                }else{
                    Init_6300RF(ch_init, pwr_init);
                    #ifdef _DEBUG_MODE
                    Printf("\r\n ch%bx, pwr%bx", ch_init, cur_pwr);
                    #endif
                }
            }else if(PIT_MODE){
                Init_6300RF(RF_FREQ, POWER_MAX+1);
                #ifdef _DEBUG_MODE
                Printf("\r\n ch%bx, pwr%bx", RF_FREQ, cur_pwr);
                #endif
            }else{
                Init_6300RF(RF_FREQ, 0);
                #ifdef _DEBUG_MODE
                Printf("\r\n ch%bx, pwr%bx", RF_FREQ, 0);
                #endif
            }
            
            DM6300_AUXADC_Calib();
        }
    }
}
#endif