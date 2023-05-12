#include "camera.h"
#include "common.h"
#include "dm6300.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "i2c_device.h"
#include "isr.h"
#include "lifetime.h"
#include "monitor.h"
#include "msp_displayport.h"
#include "print.h"
#include "rom.h"
#include "sfr_ext.h"
#include "smartaudio_protocol.h"
#include "tramp_protocol.h"
#include "uart.h"
#include "version.h"

uint8_t UNUSED = 0;
uint8_t rf_delay_init_done = 0;

void timer_task();
void RF_Delay_Init();

BIT_TYPE int0_req = 0;
BIT_TYPE int1_req = 0;

void Timer0_isr(void) INTERRUPT(1) {
    TH0 = 138;

#ifdef USE_SMARTAUDIO_SW
    if (SA_config) {
        if (suart_tx_en) {
            // TH0 = 139;
            suart_txint();
        } else
            suart_rxint();
    }
#endif

    timer_ms10x++;
}

void Timer1_isr(void) INTERRUPT(3) {
}

void Ext0_isr(void) INTERRUPT(0) {
    int0_req = 1;
}

void Ext1_isr(void) INTERRUPT(2) {
    int1_req = 1;
}

void UART0_isr() INTERRUPT(4) {
    if (RI && (tr_tx_busy == 0)) { // RX int
        RI = 0;
        RS_buf[RS_in++] = SBUF0;
        if (RS_in >= BUF_MAX)
            RS_in = 0;
        if (RS_in == RS_out)
            RS0_ERR = 1;
    }

    if (TI) { // TX int
        TI = 0;
        RS_Xbusy = 0;
    }
}
#ifdef USE_SMARTAUDIO_HW
void UART1_isr() INTERRUPT(6) {

    if (RI1) { // RX int
        RI1 = 0;
        if (sa_status == SA_ST_IDLE) {
            uart_set_baudrate(3);
            sa_status = SA_ST_RX;
            sa_start_ms = timer_ms10x;
        }
        if (sa_status == SA_ST_TX)
            return;

        RS_buf1[RS_in1++] = SBUF1;
        if (RS_in1 >= BUF1_MAX)
            RS_in1 = 0;
    }

    if (TI1) { // TX int
        TI1 = 0;
        RS_Xbusy1 = 0;
    }
}
#else
void UART1_isr() INTERRUPT(6) {

    if (RI1 && (tr_tx_busy == 0)) { // RX int
        RI1 = 0;
        RS_buf1[RS_in1++] = SBUF1;
        if (RS_in1 >= BUF1_MAX)
            RS_in1 = 0;
    }

    if (TI1) { // TX int
        TI1 = 0;
        RS_Xbusy1 = 0;
    }
}
#endif

void version_info(void) {
#ifdef _DEBUG_MODE
    debugf("\r\n");
    debugf("\r\nVtx        : %s", VTX_NAME);
    debugf("\r\nVersion    : %s", VTX_VERSION_STRING);
    debugf("\r\nBuild time : " __DATE__ " " __TIME__);
#endif
}

void main(void) {
    // init
    CPU_init();
    WriteReg(0, 0xB0, 0x3E);
    WriteReg(0, 0xB2, 0x03);
    WriteReg(0, 0x80, 0xC8);
    // WAIT(100);
    version_info();
    Init_HW(); // init
    fc_init(); // init displayport

#ifdef USE_SMARTAUDIO_SW
    SA_Init();
#elif defined USE_TRAMP
    tramp_init();
#endif

#ifdef _DEBUG_MODE
    Prompt();
#endif

    // main loop
    while (1) {
        timer_task();
#if defined USE_SMARTAUDIO_SW
        while (SA_task())
            ;
#elif defined USE_SMARTAUDIO_HW
        while (SA_task()) {
            if (SA_timeout())
                break;
        }
#elif defined USE_TRAMP
        tramp_receive();
#endif

#ifdef _RF_CALIB
        CalibProc();
#elif defined _DEBUG_MODE
        Monitor();
#endif
        video_detect();
        OnButton1();

        // if ((last_SA_lock && (seconds > WAIT_SA_CONFIG)) || (last_SA_lock == 0)) {
        LED_Task();
        TempDetect(); // temperature dectect
        PwrLMT();     // RF power ctrl
        msp_task();
        Update_EEP_LifeTime();
        // uart_baudrate_detect();
        //}

        // RF_Delay_Init();
    }
}

void timer_task() {
    static uint16_t cur_ms10x_1sd16 = 0, last_ms10x_1sd16 = 0;
    static uint8_t timer_cnt = 0;
    cur_ms10x_1sd16 = timer_ms10x;

    if (((cur_ms10x_1sd16 - last_ms10x_1sd16) >= TIMER0_1SD16) || (cur_ms10x_1sd16 < last_ms10x_1sd16)) {
        last_ms10x_1sd16 = cur_ms10x_1sd16;
        timer_cnt++;
        timer_cnt &= 15;
        if (timer_cnt == 15) { // every second, 1Hz
            btn1_tflg = 1;
            pwr_tflg = 1;
            cfg_tflg = 1;
            seconds++;
            pwr_sflg = 1;
        }

        timer_2hz = ((timer_cnt & 7) == 7);
        timer_4hz = ((timer_cnt & 3) == 3);
        timer_8hz = ((timer_cnt & 1) == 1);
        timer_16hz = 1;
    } else {
        timer_2hz = 0;
        timer_4hz = 0;
        timer_8hz = 0;
        timer_16hz = 0;
    }
}

void RF_Delay_Init() {
    static uint8_t SA_saved = 0;

    if (SA_saved == 0) {
        if (seconds >= WAIT_SA_CONFIG) {
            I2C_Write8(ADDR_EEPROM, EEP_ADDR_SA_LOCK, SA_lock);
            SA_saved = 1;
#ifdef _DEBUG_MODE
            debugf("\r\nSave SA_lock(%x) to EEPROM", (uint16_t)SA_lock);
#endif
        }
    }

    // init_rf
    if (seconds < WAIT_SA_CONFIG) { // wait for SA config vtx
        if (seconds < WAIT_SA_LOCK)
            return;
        else if (SA_lock)
            return;
        else
            seconds = WAIT_SA_CONFIG;
    } else if (rf_delay_init_done)
        return;
    else if (dm6300_init_done)
        return;
    else
        rf_delay_init_done = 1;

    if (last_SA_lock) {
#ifdef _DEBUG_MODE
        debugf("\r\nRF_Delay_Init: SA");
#endif
        pwr_lmt_sec = PWR_LMT_SEC;
        if (SA_lock) {
            if (pwr_init == POWER_MAX + 2) { // 0mW
                RF_POWER = POWER_MAX + 2;
                cur_pwr = POWER_MAX + 2;
            } else if (PIT_MODE) {
                Init_6300RF(ch_init, POWER_MAX + 1);
#ifdef _DEBUG_MODE
                debugf("\r\n ch%x, pwr%x", (uint16_t)ch_init, (uint16_t)cur_pwr);
#endif
            } else {
                Init_6300RF(ch_init, pwr_init);
#ifdef _DEBUG_MODE
                debugf("\r\n ch%x, pwr%x", (uint16_t)ch_init, (uint16_t)cur_pwr);
#endif
            }
            DM6300_AUXADC_Calib();
        }
    } else if (!mspVtxLock) {
#ifdef _DEBUG_MODE
        debugf("\r\nRF_Delay_Init: None");
#endif
        if (TEAM_RACE == 0x01)
            vtx_paralized();
#if (0)
        if (PIT_MODE == PIT_0MW) {
            pwr_lmt_done = 1;
            RF_POWER = POWER_MAX + 2;
            cur_pwr = POWER_MAX + 2;
            vtx_pit = PIT_0MW;
        } else if (PIT_MODE == PIT_P1MW) {
#else
        if (PIT_MODE != PIT_OFF) {
#endif
            Init_6300RF(RF_FREQ, POWER_MAX + 1);
            vtx_pit = PIT_P1MW;
        } else {
            WriteReg(0, 0x8F, 0x00);
            WriteReg(0, 0x8F, 0x01);
            DM6300_Init(RF_FREQ, RF_BW);
            DM6300_SetChannel(RF_FREQ);
            DM6300_SetPower(0, RF_FREQ, 0);
            cur_pwr = RF_POWER;
            WriteReg(0, 0x8F, 0x11);
        }
        DM6300_AUXADC_Calib();
    }
}