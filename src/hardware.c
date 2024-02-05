#include "hardware.h"
#include "camera.h"
#include "common.h"
#include "dm6300.h"
#include "global.h"
#include "i2c.h"
#include "i2c_device.h"
#include "isr.h"
#include "lifetime.h"
#include "monitor.h"
#include "msp_displayport.h"
#include "print.h"
#include "sfr_ext.h"
#include "smartaudio_protocol.h"
#include "spi.h"
#include "tramp_protocol.h"
#include "uart.h"

uint8_t KEYBOARD_ON = 0; // avoid conflict between keyboard and cam_control
uint8_t EE_VALID = 0;
uint8_t lowband_lock = 1;

#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
uint8_t powerLock = 1;
#endif

/**********************************
//
//  POWER MODE
//
//    HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
//    0------------25mW   (14dBm)
//    1------------200mW  (23dBm)
//    2------------500mW  (27dBm)
//    3------------1000mW (30dBm)

//    HDZERO_RACE, HDZERO_WHOOP
//    0------------25mW (14dBm)
//    1------------200mW(23dBm)
**********************************/
uint8_t RF_POWER = 0;
uint8_t RF_FREQ = 0;
uint8_t LP_MODE = 0;
uint8_t PIT_MODE = 0;
uint8_t OFFSET_25MW = 0; // 0~10 -> 0~10    11~20 -> -1~-10
uint8_t TEAM_RACE = 0;
uint8_t BAUDRATE = 0;
uint8_t SHORTCUT = 0;

uint8_t RF_BW = BW_27M;
uint8_t RF_BW_last = BW_27M;

uint8_t cfg_step = 0; // 0:idle, 1:freq, 2:power, 3:LP_MODE

uint8_t cfg_to_cnt = 0;
uint8_t pwr_lmt_done = 0;
uint8_t pwr_lmt_sec = 0;
int16_t temperature = 0;
uint8_t pwr_offset = 0;
uint8_t heat_protect = 0;

uint8_t last_SA_lock = 0;
/*
cur_pwr:
    0: 25mW
    1:200mW
    .....
    POWER_MAX+1: 0.1mw
    POWER_MAX+2: 0mw
*/
uint8_t cur_pwr = 0;

uint8_t led_status = 0;

uint8_t temp_err = 0;
#ifdef USE_TEMPERATURE_SENSOR
int16_t temp0 = 0;
uint8_t p;
#endif

uint8_t i = 0;

CODE_SEG const uint8_t BPLED[] = {
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
    0x0E, // F.
    0x47, // L.
    0x06, // E.
};

uint8_t dispE_cnt = 0xff;
uint8_t dispF_cnt = 0xff;
uint8_t dispL_cnt = 0xff;

uint8_t cameraLost = 0;

uint8_t timer_cnt = 0;

void LED_Init();

void Set_720P50(uint8_t page) {
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

void Set_720P60(uint8_t page) {
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

void Set_720P60_8bit(uint8_t page) {
    WriteReg(page, 0x21, 0x1F);

    WriteReg(page, 0x40, 0x00);
    WriteReg(page, 0x41, 0x2A);
    WriteReg(page, 0x42, 0xD0);
    WriteReg(page, 0x43, 0xE4);
    WriteReg(page, 0x44, 0x4C);
    WriteReg(page, 0x45, 0xEE);
    WriteReg(page, 0x49, 0x04);
    WriteReg(page, 0x4c, 0x19);
    WriteReg(page, 0x4f, 0x86);
    WriteReg(page, 0x52, 0x04);
    WriteReg(page, 0x53, 0x00);
    WriteReg(page, 0x54, 0x3C);
    WriteReg(0, 0x8e, 0x04);

    WriteReg(page, 0x06, 0x01);
}

void Set_960x720P60(uint8_t page) {
    WriteReg(page, 0x21, 0x1C);

    WriteReg(page, 0x40, 0xC0);
    WriteReg(page, 0x41, 0x23);
    WriteReg(page, 0x42, 0xD0);
    WriteReg(page, 0x43, 0x49);
    WriteReg(page, 0x44, 0x45);
    WriteReg(page, 0x45, 0xE3);
    WriteReg(page, 0x49, 0x04);
    WriteReg(page, 0x4c, 0x08);
    WriteReg(page, 0x4f, 0x00);
    WriteReg(page, 0x52, 0x04);
    WriteReg(page, 0x53, 0x02);
    WriteReg(page, 0x54, 0x3C);

    WriteReg(page, 0x06, 0x01);
}

void Set_720P30(uint8_t page, uint8_t is_43) {
    if (is_43)
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

void Set_540P90(uint8_t page) {
    WriteReg(page, 0x21, 0x21);

    WriteReg(page, 0x40, 0xD0);
    WriteReg(page, 0x41, 0x22);
    WriteReg(page, 0x42, 0x18);

    // WriteReg(page, 0x43, 0xD4); //pat
    // WriteReg(page, 0x44, 0x45);

    WriteReg(page, 0x43, 0x1F); // cam
    WriteReg(page, 0x44, 0x44);

    WriteReg(page, 0x45, 0x29);
    WriteReg(page, 0x49, 0x04);
    WriteReg(page, 0x4c, 0x08);
    WriteReg(page, 0x4f, 0x00);
    WriteReg(page, 0x52, 0x04);
    WriteReg(page, 0x53, 0x02);
    WriteReg(page, 0x54, 0x3C);

    WriteReg(page, 0x06, 0x01);
}

void Set_540P90_crop(uint8_t page) {
    WriteReg(page, 0x21, 0x1f);

    WriteReg(page, 0x40, 0xD0);
    WriteReg(page, 0x41, 0x22);
    WriteReg(page, 0x42, 0x18);

    // WriteReg(page, 0x43, 0xD4); //pat
    // WriteReg(page, 0x44, 0x45);

    WriteReg(page, 0x43, 0x1F); // cam
    WriteReg(page, 0x44, 0x44);

    WriteReg(page, 0x45, 0x29);
    WriteReg(page, 0x49, 0x04);
    WriteReg(page, 0x4c, 0x08);
    WriteReg(page, 0x4f, 0x00);
    WriteReg(page, 0x52, 0x04);
    WriteReg(page, 0x53, 0x02);
    WriteReg(page, 0x54, 0x3C);

    WriteReg(page, 0x06, 0x01);
}

void Set_540P60(uint8_t page) {
    WriteReg(page, 0x21, 0x1F);

    WriteReg(page, 0x40, 0xD0);
    WriteReg(page, 0x41, 0x22);
    WriteReg(page, 0x42, 0x18);

    // WriteReg(page, 0x43, 0xD0); //pat
    // WriteReg(page, 0x44, 0x47);

    WriteReg(page, 0x43, 0x50); // cam
    WriteReg(page, 0x44, 0x46);

    WriteReg(page, 0x45, 0x6B);
    WriteReg(page, 0x49, 0x04);
    WriteReg(page, 0x4c, 0x08);
    WriteReg(page, 0x4f, 0x00);
    WriteReg(page, 0x52, 0x04);
    WriteReg(page, 0x53, 0x02);
    WriteReg(page, 0x54, 0x3C);

    WriteReg(page, 0x06, 0x01);
}

void Set_1080P30(uint8_t page) {
    WriteReg(page, 0x21, 0x1B);

    WriteReg(page, 0x40, 0x80);
    WriteReg(page, 0x41, 0x47);
    WriteReg(page, 0x42, 0x38);
    WriteReg(page, 0x43, 0x98);
    WriteReg(page, 0x44, 0x88);
    WriteReg(page, 0x45, 0x65);
    WriteReg(page, 0x49, 0x04);
    WriteReg(page, 0x4c, 0x29);
    WriteReg(page, 0x4f, 0x00);
    WriteReg(page, 0x52, 0x04);
    WriteReg(page, 0x53, 0x00);
    WriteReg(page, 0x54, 0x5A);

    WriteReg(page, 0x06, 0x01);
}

void Setting_Save() {
    uint8_t err = 0;

    if (EE_VALID) {
        err |= I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_RF_FREQ, RF_FREQ);
        err |= I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_RF_POWER, RF_POWER);
        err |= I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LPMODE, LP_MODE);
        err |= I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_PITMODE, PIT_MODE);
        err |= I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_25MW, OFFSET_25MW);
        err |= I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_BAUDRATE, BAUDRATE);
        err |= I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_TEAM_RACE, TEAM_RACE);
        err |= I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_SHORTCUT, SHORTCUT);
#ifdef _DEBUG_MODE
        if (!err)
            debugf("\r\nEEPROM write success");
#endif
    }

#ifdef _DEBUG_MODE
    debugf("\r\nSetting Save\r\n");
    debugf("    RF_FREQ=%d\r\n", (uint16_t)RF_FREQ);
    debugf("    RF_POWER=%d\r\n", (uint16_t)RF_POWER);
    debugf("    LP_MODE=%d\r\n", (uint16_t)LP_MODE);
    debugf("    PIT_MODE=%d\r\n", (uint16_t)PIT_MODE);
    debugf("    OFFSET_25MW=%d\r\n", (uint16_t)OFFSET_25MW);
    debugf("    BAUDRATE=%d\r\n", (uint16_t)BAUDRATE);
    debugf("    TEAM_RACE=%d\r\n", (uint16_t)TEAM_RACE);
    debugf("    SHORTCUT=%d\r\n", (uint16_t)SHORTCUT);
#endif
}

void CFG_Back() {
    RF_FREQ = (RF_FREQ >= FREQ_NUM) ? 0 : RF_FREQ;
    RF_POWER = (RF_POWER > POWER_MAX) ? 0 : RF_POWER;
    LP_MODE = (LP_MODE > 2) ? 0 : LP_MODE;
    PIT_MODE = (PIT_MODE > PIT_0MW) ? PIT_OFF : PIT_MODE;
    OFFSET_25MW = (OFFSET_25MW > 20) ? 0 : OFFSET_25MW;
    BAUDRATE = (BAUDRATE > 1) ? 0 : BAUDRATE;
    TEAM_RACE = (TEAM_RACE > 2) ? 0 : TEAM_RACE;
    SHORTCUT = (SHORTCUT > 1) ? 0 : SHORTCUT;
}

void GetVtxParameter() {
    unsigned char CODE_SEG *ptr = (unsigned char CODE_SEG *)0xFFE8;
    uint8_t i, j;
    XDATA_SEG uint8_t tab[FREQ_NUM_EXTERNAL][POWER_MAX + 1];
    uint8_t flash_vld = 1;
    uint8_t ee_vld = 1;
    uint8_t tab_min[4] = {255, 255, 255, 255};

    EE_VALID = !I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_EEP_VLD, 0xFF);

#ifdef _DEBUG_MODE
    debugf("\r\nEE_VALID:%x", (uint16_t)EE_VALID);
#endif

    if (EE_VALID) { // eeprom valid

#ifdef FIX_EEP
        for (i = 0; i < FREQ_NUM_INTERNAL; i++) {
            for (j = 0; j <= POWER_MAX; j++) {
                I2C_Write8_Wait(10, ADDR_EEPROM, i * (POWER_MAX + 1) + j, table_power[0][i][j]);
            }
        }
#endif
        // race band
        for (i = 0; i < FREQ_NUM_INTERNAL; i++) {
            for (j = 0; j <= POWER_MAX; j++) {
                tab[i][j] = I2C_Read8_Wait(10, ADDR_EEPROM, i * (POWER_MAX + 1) + j);
                if (tab[i][j] < tab_min[j])
                    tab_min[j] = tab[i][j];
                if (tab[i][j] == 0xFF)
                    ee_vld = 0;
            }
        }

        // E/F band
        for (j = 0; j <= POWER_MAX; j++) {
            tab[8][j] = tab[0][j];  // E1 <- R1
            tab[9][j] = tab[2][j];  // F1 <- R3
            tab[10][j] = tab[3][j]; // F2 <- R4
            tab[11][j] = tab[4][j]; // F4 <- R5
        }

        // low band
        for (i = 10; i < FREQ_NUM_EXTERNAL; i++) {
            for (j = 0; j <= POWER_MAX; j++) {
                tab[i][j] = tab_min[j] - 4;
            }
        }

        if (ee_vld) {
#ifdef _DEBUG_MODE
            debugf("\r\nUSE EEPROM for rf_pwr_tab.");
#endif
            for (i = 0; i < FREQ_NUM_EXTERNAL; i++) {
                for (j = 0; j <= POWER_MAX; j++) {
                    table_power[i][j] = tab[i][j];
#ifndef _RF_CALIB
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
#else
                    if (j == 0) // 25mw +3dbm
                        table_power[i][j] += 0xC;
#endif
#endif
                }
            }
        } else {
#ifdef _DEBUG_MODE
            debugf("\r\nEEPROM is NOT initialized. Use default rf_pwr_tab.");
#endif

#ifdef _RF_CALIB
            for (i = 0; i < FREQ_NUM_INTERNAL; i++) {
                for (j = 0; j <= POWER_MAX; j++) {
                    I2C_Write8_Wait(10, ADDR_EEPROM, i * (POWER_MAX + 1) + j, table_power[i][j]);
                }
            }
#endif
        }

        // VTX Setting
        lowband_lock = I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_LOWBAND_LOCK);
        RF_FREQ = I2C_Read8(ADDR_EEPROM, EEP_ADDR_RF_FREQ);
        RF_POWER = I2C_Read8(ADDR_EEPROM, EEP_ADDR_RF_POWER);
        LP_MODE = I2C_Read8(ADDR_EEPROM, EEP_ADDR_LPMODE);
        PIT_MODE = I2C_Read8(ADDR_EEPROM, EEP_ADDR_PITMODE);
        OFFSET_25MW = I2C_Read8(ADDR_EEPROM, EEP_ADDR_25MW);
        TEAM_RACE = I2C_Read8(ADDR_EEPROM, EEP_ADDR_TEAM_RACE);
        SHORTCUT = I2C_Read8(ADDR_EEPROM, EEP_ADDR_SHORTCUT);
#ifdef USE_TRAMP
        // tramp protocol need 115200 bps.
        BAUDRATE = 0;
#else
        BAUDRATE = I2C_Read8(ADDR_EEPROM, EEP_ADDR_BAUDRATE);
#endif
        CFG_Back();
#ifdef RESET_CONFIG
        RF_FREQ = 0;
        RF_POWER = 0;
        LP_MODE = 0;
        PIT_MODE = 0;
        OFFSET_25MW = 0;
        TEAM_RACE = 0;
        BAUDRATE = 0;
        SHORTCUT = 0;
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_RF_FREQ, RF_FREQ);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_RF_POWER, RF_POWER);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LPMODE, LP_MODE);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_PITMODE, PIT_MODE);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_25MW, OFFSET_25MW);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_TEAM_RACE, TEAM_RACE);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_BAUDRATE, BAUDRATE);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_SHORTCUT, SHORTCUT);
#endif
#ifdef _DEBUG_MODE
        debugf("\r\nUSE EEPROM for VTX setting:RF_FREQ=%d, RF_POWER=%d, LPMODE=%d PIT_MODE=%d", (uint16_t)RF_FREQ, (uint16_t)RF_POWER, (uint16_t)LP_MODE, (uint16_t)PIT_MODE);
#endif

// last_SA_lock
#if defined USE_SMARTAUDIO_SW || defined USE_SMARTAUDIO_HW
        last_SA_lock = I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_SA_LOCK);
        WAIT(10);
        if (last_SA_lock == 0xff) {
            last_SA_lock = 0;
            I2C_Write8(ADDR_EEPROM, EEP_ADDR_SA_LOCK, last_SA_lock);
        }
#ifdef _DEBUG_MODE
        debugf("\r\nlast_SA_lock %x", (uint16_t)last_SA_lock);
#endif
#endif

#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
        // powerLock
        powerLock = 0x01 & I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_POWER_LOCK);
#endif
    } else {
        CFG_Back();
    }

#ifdef _DEBUG_DM6300
    for (i = 0; i < FREQ_NUM_EXTERNAL; i++) {
        debugf("\r\nrf_pwr_tab[%d]=", (uint16_t)i);
        for (j = 0; j <= POWER_MAX; j++)
            debugf(" %x", (uint16_t)table_power[i][j]);
    }
    debugf("\r\nUSE EEPROM for VTX setting:RF_FREQ=%d, RF_POWER=%d, LPMODE=%d PIT_MODE=%d", (uint16_t)RF_FREQ, (uint16_t)RF_POWER, (uint16_t)LP_MODE, (uint16_t)PIT_MODE);
#endif
}

void Init_6300RF(uint8_t freq, uint8_t pwr) {
    WriteReg(0, 0x8F, 0x00);
    WriteReg(0, 0x8F, 0x01);
    DM6300_Init(freq, RF_BW);
    DM6300_SetChannel(freq);
#ifndef VIDEO_PAT
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
    if ((pwr == 3) && (!g_IS_ARMED))
        pwr_lmt_done = 0;
    else
#endif
#endif
    {
        DM6300_SetPower(pwr, freq, pwr_offset);
        cur_pwr = pwr;
    }
    WriteReg(0, 0x8F, 0x11);
    rf_delay_init_done = 1;
#ifdef _DEBUG_MODE
    debugf("\r\nInit_6300RF(%x, %x)", (uint16_t)freq, (uint16_t)pwr);
#endif
}

void Init_HW() {
    WAIT(100);
    SPI_Init();
    LED_Init();
#ifdef VIDEO_PAT
#ifdef USE_TP9950
    Set_720P60_8bit(0);
#else
    Set_720P60(0);
#endif
    WriteReg(0, 0x50, 0x01);
    RF_FREQ = 0;
    GetVtxParameter();
#ifdef _RF_CALIB
    RF_POWER = 0; // max power
    RF_FREQ = 0;  // ch1
#else
    RF_POWER = 0;
#endif
    Init_6300RF(RF_FREQ, RF_POWER);
    DM6300_AUXADC_Calib();
#else
#ifdef USE_TC3587_LED
    LED_TC3587_Init();
#endif
    GetVtxParameter();
    Get_EEP_LifeTime();
    camera_init();

    uart_set_baudrate(BAUDRATE);
//--------- dm6300 --------------------
// move to RF_Delay_Init()
#endif

    //---------- DC IQ --------------------
    // WriteReg(0, 0x25, 0xf6);
    // WriteReg(0, 0x26, 0x00);
}
#ifdef USE_TEMPERATURE_SENSOR
void TempDetect() {
    static uint8_t init = 1;
    int16_t temp_new, temp_new0;

    if (timer_2hz) {
        if (init) {
            init = 0;
            temp0 = DM6300_GetTemp();
            temp0 <<= 2;

            temperature = I2C_Read8(ADDR_TEMPADC, 0); // NCT75 MSB 8bit
            // temperature >>= 5; //LM75AD

            if (temperature >= 0x7D) // MAX +125
                temperature = 0x7D;
            temperature <<= 2; // filter * 4
        } else {
            temp_new0 = DM6300_GetTemp();

            temp_new = I2C_Read8(ADDR_TEMPADC, 0);
            if (temp_new >= 0x7D) // MAX +125
                temp_new = 0x7D;
            // temp_new >>= 5; //LM75AD

            temperature = temperature - (temperature >> 2) + temp_new;

            temp0 = temp0 - (temp0 >> 2) + temp_new0;

#ifdef _DEBUG_MODE
// verbosef("\r\ntempADC  detect: temp = %d, temp_new = %d", (uint16_t)(temperature>>2), (uint16_t)temp_new);
// verbosef("\r\ntemp6300 detect: temp = 0x%x, temp_new = 0x%x", (uint16_t)(temp0>>2), (uint16_t)temp_new0);
#endif
        }
    }
}
#else
void TempDetect() {
    static uint8_t init = 1;
    int16_t temp_new;

    if (!dm6300_init_done)
        return;

    if (timer_2hz) {
        if (init) {
            init = 0;
            temperature = DM6300_GetTemp();
            temperature <<= 2;
        } else {
            temp_new = DM6300_GetTemp();
            temperature = temperature - (temperature >> 2) + temp_new;

#ifdef _DEBUG_MODE
// verbosef("\r\ntemp detect: temp = %x, temp_new = %x", (uint16_t)(temperature>>2), (uint16_t)temp_new);
#endif
        }
    }
}
#endif

#ifdef USE_TEMPERATURE_SENSOR
uint8_t temperature_level(void) {
    int16_t ret;
    ret = temperature >> 2;

    if (ret < 25)
        ret = 0;
    else if (ret < 30)
        ret = 1;
    else if (ret < 35)
        ret = 2;
    else if (ret < 38)
        ret = 3;
    else if (ret < 40)
        ret = 4;
    else if (ret < 43)
        ret = 5;
    else if (ret < 45)
        ret = 6;
    else if (ret < 48)
        ret = 7;
    else if (ret < 50)
        ret = 8;
    else if (ret < 53)
        ret = 9;
    else if (ret < 55)
        ret = 10;
    else if (ret < 58)
        ret = 11;
    else if (ret < 60)
        ret = 12;
    else if (ret < 63)
        ret = 13;
    else if (ret < 65)
        ret = 14;
    else if (ret < 68)
        ret = 15;
    else if (ret < 70)
        ret = 16;
    else if (ret < 73)
        ret = 17;
    else if (ret < 75)
        ret = 18;
    else if (ret < 78)
        ret = 19;
    else if (ret < 80)
        ret = 20;
    else
        ret = 20;

    return (uint8_t)ret;
}
void PowerAutoSwitch() {
    static uint8_t last_ofs = 0;

    if (pwr_sflg)
        pwr_sflg = 0;
    else
        return;

    last_ofs = pwr_offset;
    pwr_offset = temperature_level();

#ifdef HDZERO_WHOOP_LITE
    pwr_offset >>= 1;
#elif defined HDZERO_FREESTYLE_V2
    if (pwr_offset > 16)
        pwr_offset = 16;
#endif

    if ((!g_IS_ARMED) && (last_ofs == pwr_offset))
        ;
    else {
        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
#ifdef _DEBUG_MODE
        verbosef("\r\nPowerAutoSwitch: temp = %x%x, ", (uint16_t)(temperature >> 10), (uint16_t)((temperature >> 2) & 0xff));
        verbosef("pwr_offset = %d", (uint16_t)pwr_offset);
#endif
        cur_pwr = RF_POWER;
    }
}
#else
void PowerAutoSwitch() {
    int16_t temp;
    static uint8_t last_ofs = 0;

    if (pwr_sflg)
        pwr_sflg = 0;
    else
        return;

    temp = temperature >> 2;

    if (RF_POWER == 0) {
        if (temp < 0x479)
            pwr_offset = 0;
        else if (temp < 0x488)
            pwr_offset = 1;
        else if (temp < 0x4A2)
            pwr_offset = 2;
        else if (temp < 0x4B5)
            pwr_offset = 3;
        else if (temp < 0x4C7)
            pwr_offset = 4;
        else if (temp < 0x4DD)
            pwr_offset = 5;
        else if (temp < 0x4F0)
            pwr_offset = 6;
        else if (temp < 0x500)
            pwr_offset = 7;
        else if (temp < 0x512)
            pwr_offset = 8;
        else if (temp < 0x524)
            pwr_offset = 9;
        else if (temp < 0x534)
            pwr_offset = 10;
        else if (temp < 0x544)
            pwr_offset = 11;
        else if (temp < 0x554)
            pwr_offset = 12;
        else if (temp < 0x564)
            pwr_offset = 13;
        else if (temp < 0x574)
            pwr_offset = 14;
        else if (temp < 0x582)
            pwr_offset = 15;
        else if (temp < 0x592)
            pwr_offset = 16;
        else if (temp < 0x59D)
            pwr_offset = 17;
        else if (temp < 0x5AC)
            pwr_offset = 18;
        else
            pwr_offset = 19;
    } else {
        if (temp < 0x479)
            pwr_offset = 0;
        else if (temp < 0x488)
            pwr_offset = 1;
        else if (temp < 0x4A2)
            pwr_offset = 2;
        else if (temp < 0x4B5)
            pwr_offset = 3;
        else if (temp < 0x4C7)
            pwr_offset = 4;
        else if (temp < 0x4DD)
            pwr_offset = 5;
        else if (temp < 0x4F0)
            pwr_offset = 6;
        else if (temp < 0x500)
            pwr_offset = 7;
        else if (temp < 0x512)
            pwr_offset = 8;
        else if (temp < 0x523)
            pwr_offset = 9;
        else if (temp < 0x533)
            pwr_offset = 10;
        else if (temp < 0x543)
            pwr_offset = 11;
        else if (temp < 0x552)
            pwr_offset = 12;
        else if (temp < 0x562)
            pwr_offset = 13;
        else if (temp < 0x572)
            pwr_offset = 14;
        else if (temp < 0x580)
            pwr_offset = 15;
        else if (temp < 0x590)
            pwr_offset = 16;
        else if (temp < 0x59B)
            pwr_offset = 17;
        else if (temp < 0x5AA)
            pwr_offset = 18;
        else
            pwr_offset = 19;
    }

    if (temp_err)
        pwr_offset = 10;

    if (OFFSET_25MW <= 10) {
        if (temp_err)
            pwr_offset = 10 - OFFSET_25MW;
        else if (pwr_offset + OFFSET_25MW > 20)
            pwr_offset = 20 - OFFSET_25MW;
    }

    if (last_ofs != pwr_offset) {
#ifdef _DEBUG_MODE
        verbosef("\r\nPowerAutoSwitch:Yes 0x%x 0x%x", temp, (uint16_t)pwr_offset);
#endif
        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
        cur_pwr = RF_POWER;
    } else {
#ifdef _DEBUG_MODE
        verbosef("\r\nPowerAutoSwitch: No 0x%x 0x%x", temp, (uint16_t)pwr_offset);
#endif
    }

    last_ofs = pwr_offset;
}
#endif
void HeatProtect() {
    static uint16_t cur_sec = 0;
    static uint8_t sec = 0;
    static uint8_t cnt = 0;
    int16_t temp;

#ifdef USE_TEMPERATURE_SENSOR
#ifdef HDZERO_FREESTYLE_V2
    int16_t temp_max = 95;
#else
    int16_t temp_max = 90;
#endif
#else
    int16_t temp_max = 0x5C0;
    int16_t temp_err_data = 0x700;
#endif

    if (!g_IS_ARMED) {
        if (heat_protect == 0) {
            if (cur_sec != seconds) {
                cur_sec = seconds;
                sec++;
                if (sec >= 4) {
                    sec = 0;
                    temp = temperature >> 2;
// temp = temperature >> 5;  //LM75AD
#ifdef _DEBUG_MODE
#ifdef USE_TEMPERATURE_SENSOR
                    verbosef("\r\nHeat detect: temp = %d, pwr_offset=%d", (uint16_t)temp, (uint16_t)pwr_offset);
#else
                    // verbosef("\r\nHeat Protect detect: 0x%x", temp);
#endif
#endif

#ifdef USE_TEMPERATURE_SENSOR
                    ;
#else
                    if (temp > temp_err_data) {
                        temp_err = 1;
                        return;
                    }
#endif

#ifdef USE_TEMPERATURE_SENSOR
                    if (temp >= temp_max)
#else
                    if ((temp_err == 0) && temp >= temp_max)
#endif
                    {
                        cnt++;
                        if (cnt == 3) {
#ifdef _DEBUG_MODE
                            debugf("\r\nHeat Protect.");
#endif
                            heat_protect = 1;
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
                            WriteReg(0, 0x8F, 0x00);
                            msp_set_vtx_config(POWER_MAX + 1, 0);
#else
                            DM6300_SetPower(0, RF_FREQ, 0);
                            msp_set_vtx_config(0, 0);
#endif
                            cur_pwr = 0;
                            pwr_offset = 0;
                            pwr_lmt_done = pwr_lmt_sec = 0;
                            cnt = 0;
                        }
                    } else
                        cnt = 0;
                }
            }
        }
    } else {
        heat_protect = cnt = cur_sec = sec = 0;
    }
}

void PwrLMT() {
    static uint8_t p_init = 1;

    if (cur_pwr > POWER_MAX)
        return;

#ifndef _RF_CALIB
    if (SA_lock) { // Smart Audio
        HeatProtect();
        if (!heat_protect) {
            if (pwr_lmt_done == 0) {
                if (pwr_tflg) {
                    pwr_tflg = 0;
                    pwr_lmt_sec++;

#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
                    // test: power plus every sec
                    if (pwr_lmt_sec >= 3) {
                        if (RF_POWER == 3) {
                            if (p_init) {
                                p = table_power[RF_FREQ][3] - 0x1C;
                                p_init = 0;
                            }

                            SPI_Write(0x6, 0xFF0, 0x00000018);

                            if (p >= table_power[RF_FREQ][3]) {
                                p = table_power[RF_FREQ][3];
                            } else {
                                p += 0x4;
                                debugf("\r\npwr_plus 2dbm,p=%x", (uint16_t)p);
                            }

                            SPI_Write(0x3, 0xD1C, (uint32_t)p);
                            SPI_Write(0x3, 0x330, 0x31F); // 1W
                        }
                    }
#endif

#ifdef _DEBUG_MODE
                    debugf("\r\npwr_lmt_sec %x", (uint16_t)pwr_lmt_sec);
#endif
                    if (pwr_lmt_sec >= PWR_LMT_SEC) {
                        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                        cur_pwr = RF_POWER;
                        pwr_lmt_done = 1;
                        pwr_lmt_sec = 0;
                        // test: power init reset
                        p_init = 1;

#ifdef _DEBUG_MODE
                        debugf("\r\nPower limit done.");
#endif
                        Prompt();
                    }
                }
            } else {
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
    else if (g_IS_ARMED) { // Armed
        PowerAutoSwitch();
        HeatProtect();
    } else { // Disarmed
        if (PIT_MODE) {
            /*if(cur_pwr == 0mW)
                set 0mW;
            else
                set 0.1mW;*/
        } else if (LP_MODE) {
            // DM6300_SetPower(0, RF_FREQ, 0);
            // cur_pwr = 0;
        } else {
            HeatProtect();
            if (!heat_protect) {
                if (pwr_lmt_done == 0) {
                    if (pwr_tflg) {
                        pwr_tflg = 0;
                        pwr_lmt_sec++;

#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
                        // test: power plus every sec
                        if (pwr_lmt_sec >= 3) {
                            if (RF_POWER == 3) {
                                if (p_init) {
                                    p = table_power[RF_FREQ][3] - 0x1C;
                                    p_init = 0;
                                }
                                SPI_Write(0x6, 0xFF0, 0x00000018); // set page

                                if (p >= table_power[RF_FREQ][3])
                                    p = table_power[RF_FREQ][3];
                                else {
                                    p += 0x4;
                                    debugf("\r\npwr_plus 2dbm,p=%x", (uint16_t)p + pwr_offset);
                                }
                                SPI_Write(0x3, 0xD1C, (uint32_t)(p + pwr_offset)); // digital offset
                                SPI_Write(0x3, 0x330, 0x31F);                      // analog offset 1W
                            }
                        }
#endif // HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2

#ifdef _DEBUG_MODE
                        debugf("\r\npwr_lmt_sec %x", (uint16_t)pwr_lmt_sec);
#endif
                        if (pwr_lmt_sec >= PWR_LMT_SEC) {
                            DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                            cur_pwr = RF_POWER;
                            pwr_lmt_done = 1;
                            pwr_lmt_sec = 0;
                            // test: power init reset
                            p_init = 1;

#ifdef _DEBUG_MODE
                            debugf("\r\nPower limit done.");
#endif
                            Prompt();
                        }
                    }
                } else {
                    PowerAutoSwitch();
                }
            }
        }
    }
#else
    PowerAutoSwitch();
#endif
}

void Flicker_LED(uint8_t n) {
    uint8_t i;
    for (i = 0; i < n; i++) {
        LED_BLUE_OFF;
        WAIT(50);
        LED_BLUE_ON;
        WAIT(50);
    }
    led_status = ON;
}

void video_detect(void) {
    static uint16_t last_sec = 0;
    static uint8_t sec = 0;
    uint16_t video_type_id = 0;
    uint8_t i;

    if (last_sec != seconds) {
        last_sec = seconds;
        sec++;

#ifdef _DEBUG_TC3587
        debugf("\r\nTC3587 reg:");
        val = I2C_Read16(ADDR_TC3587, 0x0062);
        debugf("\r\n0x0062 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0064);
        debugf("\r\n0x0064 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0066);
        debugf("\r\n0x0066 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0068);
        debugf("\r\n0x0068 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x006A);
        debugf("\r\n0x006A = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x006C);
        debugf("\r\n0x006C = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x006E);
        debugf("\r\n0x006E = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0070);
        debugf("\r\n0x0070 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0080);
        debugf("\r\n0x0080 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0082);
        debugf("\r\n0x0082 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0084);
        debugf("\r\n0x0084 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0086);
        debugf("\r\n0x0086 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0088);
        debugf("\r\n0x0088 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x008A);
        debugf("\r\n0x008A = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x008C);
        debugf("\r\n0x008C = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x008E);
        debugf("\r\n0x008E = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x0090);
        debugf("\r\n0x0090 = 0x%x", val);
        val = I2C_Read16(ADDR_TC3587, 0x00F8);
        debugf("\r\n0x00F8 = 0x%x", val);
        debugf("\r\n");
        I2C_Write16(ADDR_TC3587, 0x0064, 0x01ff);
        I2C_Write16(ADDR_TC3587, 0x0068, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x006C, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x0080, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x0082, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x0084, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x0086, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x0088, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x008A, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x008C, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x008E, 0x0000);
        I2C_Write16(ADDR_TC3587, 0x0090, 0x0000);
#endif

        if (heat_protect)
            return;
#if (0)
        if (camera_type == CAMERA_TYPE_RESERVED) {
            cameraLost = 1;
            return;
        }
#endif
        if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V1 ||
            camera_type == CAMERA_TYPE_RUNCAM_MICRO_V2 ||
            camera_type == CAMERA_TYPE_RUNCAM_NANO_90 ||
            camera_type == CAMERA_TYPE_RUNCAM_MICRO_V3 ||
            camera_type == CAMERA_TYPE_RESERVED) {
            i = 0;
            video_type_id = 0;
            while (video_type_id == 0) {
                video_type_id = I2C_Read16(ADDR_TC3587, 0x006A);
                i++;
                if (i == 10)
                    break;
                WAIT(1);
            }
            cameraLost = (video_type_id != 0x1E); // YUV422
#ifdef _DEBUG_CAMERA
            debugf("\r\nvideo_TypeID:%d, cameraLost:%d", video_type_id, uint16_t(val));
#endif
            return;
        }

        cameraLost = (ReadReg(0, 0x02) >> 4) & 1;
        if (camera_type == CAMERA_TYPE_OUTDATED) {
            cameraLost |= (I2C_Read8(ADDR_TP9950, 0x01) != 0x7E);
            return;
        }

        if (sec == 3) {
            sec = 0;
            if (cameraLost) { // video loss
#ifdef _DEBUG_CAMERA
                debugf("r\ncamera lost");
#endif
                if (video_format == VDO_FMT_720P50) {
                    Set_720P60(IS_RX);
                    video_format = VDO_FMT_720P60;
                } else if (video_format == VDO_FMT_720P60) {
                    Set_720P50(IS_RX);
                    video_format = VDO_FMT_720P50;
                }
            }
        }
    }
}

void Imp_RF_Param() {
    DM6300_SetChannel(RF_FREQ);
    if (LP_MODE && !g_IS_ARMED)
        return;
#ifndef VIDEO_PAT
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
    if (RF_POWER == 3 && !g_IS_ARMED)
        pwr_lmt_done = 0;
    else
#endif
#endif
        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
    cur_pwr = RF_POWER;
}

void Button1_SP() {
#ifdef _DEBUG_MODE
    debugf("\r\nButton1_SP.");
#endif

    cfg_to_cnt = 0;

    // exit 0mW
    if (vtx_pit_save == PIT_0MW) {
#ifdef _DEBUG_MODE
        debugf("\n\rDM6300 init");
#endif
        Init_6300RF(RF_FREQ, RF_POWER);
        DM6300_AUXADC_Calib();
        cur_pwr = RF_POWER;
        // reset pitmode
        vtx_pit_save = PIT_OFF;
        PIT_MODE = PIT_OFF;
    }

    switch (cfg_step) {
    case 0:
        cfg_step = 1;
        if (RF_FREQ == 8)
            dispE_cnt = 0;
        else if ((RF_FREQ == 9) || (RF_FREQ == 10) || (RF_FREQ == 11))
            dispF_cnt = 0;
        else if (RF_FREQ > 11)
            dispL_cnt = 0;
        CFG_Back();

        break;
    case 1:
        RF_FREQ++;
        if (RF_FREQ == FREQ_NUM)
            RF_FREQ = 0;

        if (RF_FREQ == 8)
            dispE_cnt = 0;
        else if ((RF_FREQ == 9) || (RF_FREQ == 10) || (RF_FREQ == 11))
            dispF_cnt = 0;
        else if (RF_FREQ > 11)
            dispL_cnt = 0;
        Imp_RF_Param();
        Setting_Save();
        msp_set_vtx_config(RF_POWER, 1);

        if (LP_MODE) {
            cur_pwr = 0;
            DM6300_SetPower(0, RF_FREQ, 0);
        } else {
            DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
            cur_pwr = RF_POWER;
        }
        break;
    case 2:
        if (RF_POWER >= POWER_MAX)
            RF_POWER = 0;
        else
            RF_POWER++;
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
        if (powerLock)
            RF_POWER &= 0x01;
#endif
        Imp_RF_Param();
        Setting_Save();

        msp_set_vtx_config(RF_POWER, 1);
        if (LP_MODE) { // limit power to 25mW
            DM6300_SetPower(0, RF_FREQ, 0);
            cur_pwr = 0;
        } else {
#ifndef VIDEO_PAT
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
            if (RF_POWER == 3 && !g_IS_ARMED)
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
        LP_MODE++;
        if (LP_MODE > 2)
            LP_MODE = 0;

        if (LP_MODE) {
            DM6300_SetPower(0, RF_FREQ, 0); // limit power to 25mW
            cur_pwr = 0;
#ifdef _DEBUG_MODE
            debugf("\n\rEnter LP_MODE");
#endif
        } else {
            DM6300_SetPower(RF_POWER, RF_FREQ, 0);
        }

        msp_set_vtx_config(RF_POWER, 1);

        Setting_Save();
        break;
    }
    // debugf("\r\nShort Press: cfg_step=%d, RF_FREQ=%d, RF_POWER=%d", (uint16_t)cfg_step, (uint16_t)RF_FREQ, (uint16_t)RF_POWER);
}

void Button1_LP() {
#ifdef _DEBUG_MODE
    debugf("\r\nButton1_LP.");
#endif
    cfg_to_cnt = 0;
    switch (cfg_step) {
    case 0:
        cfg_step = 2;
        CFG_Back();
        set_segment(0xFF);
        break;
    case 1:
        cfg_step = 2;
        set_segment(0xFF);
        break;
    case 2:
    case 3:
        cfg_step = 0;
        set_segment(0x00);
        WAIT(100);
        set_segment(0xFF);
        break;
    }
    // debugf("\r\nShort Press: cfg_step=%d, FREQ_CFG=%d, POWER_CFG=%d", (uint16_t)cfg_step, (uint16_t)FREQ_CFG, (uint16_t)POWER_CFG);
}

void Button1_LLP() {
#ifdef _DEBUG_MODE
    debugf("\r\nButton1_LLP.");
#endif
    cfg_to_cnt = 0;
    if (cfg_step == 0) {
        cfg_step = 3;
        CFG_Back();
        set_segment(0xFF);
    }
}

void Flicker_MAX(uint8_t ch, uint8_t cnt) {
    uint8_t i;
    for (i = 0; i < cnt; i++) {
        set_segment(0xFF);
        WAIT(90);
        set_segment(ch);
        WAIT(120);
    }
    set_segment(0xFF);
}

void BlinkPhase() {
    uint8_t bp = 0;

    if (cfg_step == 1 && dispE_cnt < DISP_TIME) {
        // display 'E' band
        bp = BPLED[16];
        set_segment(bp);
    } else if (cfg_step == 1 && dispF_cnt < DISP_TIME) {
        // display 'F' band
        bp = BPLED[14];
        set_segment(bp);
    } else if (cfg_step == 1 && dispL_cnt < DISP_TIME) {
        // display 'L' band
        bp = BPLED[15];
        set_segment(bp);
    } else {
        switch (cfg_step) {
#if 0 // Clear the compiler warning
        case 0:
            // set_segment(0xFF);
            break;
#endif

        case 1:
            if (RF_FREQ < 8)
                bp = BPLED[RF_FREQ + 1];
            else if (RF_FREQ == 8) // E1
                bp = BPLED[1];
            else if (RF_FREQ == 9) // F1
                bp = BPLED[1];
            else if (RF_FREQ == 10) // F2
                bp = BPLED[2];
            else if (RF_FREQ == 11) // F4
                bp = BPLED[4];
            else
                bp = BPLED[RF_FREQ - 11];
            set_segment(bp);
            break;

        case 2:
            bp = BPLED[RF_POWER + 1] & 0x7F;
            set_segment(bp);
            break;

        case 3:
            bp = BPLED[LP_MODE + 1];
            set_segment(bp);
            break;
        }
    }
}

void CFGTimeout() {
    if (cfg_step != 0) { // timeout
        if (cfg_tflg) {
            cfg_tflg = 0;
            cfg_to_cnt++;
            if (cfg_to_cnt >= CFG_TO_SEC) {
                CFG_Back();
                cfg_step = 0;

                set_segment(0xFF);
#ifdef _DEBUG_MODE
                debugf("\r\nCFG Timeout.");
#endif
                Prompt();
            }
        }
    }
}

void OnButton1() {
    static uint8_t btn_down = 0;
    static uint8_t sec = 0;
    static uint8_t leave_respond = 0;
    static uint16_t cur_sec = 0;
    static uint8_t last_keyon = 0;

    if (SA_lock)
        return;

    if (cur_sec != seconds) {
        cur_sec = seconds;

        USE_MAX7315 = !I2C_Write8(ADDR_MAX7315, 0x09, 0);
        USE_PCA9554 = !I2C_Write8(ADDR_PCA9554, 0x09, 0);

        KEYBOARD_ON = USE_MAX7315 | USE_PCA9554;
        if (KEYBOARD_ON && (last_keyon == 0))
            set_segment(0xFF);
        last_keyon = KEYBOARD_ON;
    }

    if (KEYBOARD_ON == 0) {
        WriteReg(0, 0xB0, 0x3E);

        btn_down = sec = leave_respond = 0;
        cfg_step = 0;
        cfg_to_cnt = 0;
    } else {
        WriteReg(0, 0xB0, 0x3F);

        if (btn_down == 0) {
            if (BTN_1 == 0) {
                WAIT(16);
                if (BTN_1 == 0) {
                    btn_down = 1;
                    sec = 0;
                }
            }
        } else {
            if (BTN_1) {
                if (leave_respond == 2) // long long press
                    Button1_LLP();
                else if (leave_respond == 1) // long press
                    Button1_LP();
                else if (leave_respond == 0) // short press
                    Button1_SP();

                leave_respond = 0;
                btn_down = 0;
                sec = 0;
            } else if (btn1_tflg) {
                btn1_tflg = 0;
                sec++;
                if (sec == PRESS_L) {
                    // enter long
                    if (cfg_step <= 1) {
                        Flicker_MAX(0x0C, 3);
                        leave_respond = 1;
                    } else {
                        Button1_LP();
                        leave_respond = 3;
                    }
                } else if (sec == PRESS_LL) {
                    // enter long long
                    if (cfg_step == 0 && leave_respond != 3) {
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

void LED_Init() {
    LED_BLUE_ON;
    led_status = ON;
}
void LED_Flip() {
    if (led_status == ON) {
        LED_BLUE_OFF;
        led_status = OFF;
    } else {
        LED_BLUE_ON;
        led_status = ON;
    }
}
void LED_Task() {
    if (dm6300_lost) {
        if (timer_cnt == 0 || timer_cnt == 4) {
            LED_BLUE_ON;
            led_status = ON;
        } else if (timer_cnt == 2 || timer_cnt == 6) {
            LED_BLUE_OFF;
            led_status = OFF;
        }
    } else if (cameraLost) {
        if (led_status == ON) {
            LED_BLUE_OFF;
            led_status = OFF;
        }
    } else if (heat_protect) {
        if (timer_2hz)
            LED_Flip();
    } else if (cur_pwr == POWER_MAX + 2) {
        if (timer_8hz) {
            LED_Flip();
        }
    } else if (PIT_MODE != PIT_OFF) {
        if (timer_4hz) {
            LED_Flip();
        }
    } else if (led_status == OFF) {
        LED_BLUE_ON;
        led_status = ON;
    }
}

uint8_t RF_BW_to_be_changed(void) {
    uint8_t ret = 0;
    if (camera_type == CAMERA_TYPE_RUNCAM_NANO_90 && camera_setting_reg_menu[11] == 2)
        RF_BW = BW_17M;
    else
        RF_BW = BW_27M;

    if (RF_BW != RF_BW_last) {
        RF_BW_last = RF_BW;
        ret = 1;
    }
#ifdef _DEBUG_MODE
    debugf("\r\ncamera_type:%x, video_mode:%x, RF_BW:%x, RF_BW_last:%x ret=%x", (uint16_t)camera_type, (uint16_t)camera_setting_reg_set[11], (uint16_t)RF_BW, (uint16_t)RF_BW_last, (uint16_t)ret);
#endif
    return ret;
}

void uart_baudrate_detect(void) {
#ifdef USE_TRAMP
    // tramp protocol need 115200 bps.
    return;
#else
    if (seconds - msp_lst_rcv_sec >= 20) {
        msp_lst_rcv_sec = seconds;
        BAUDRATE++;
        CFG_Back();
        uart_set_baudrate(BAUDRATE);
        Setting_Save();
    }
#endif
}

void vtx_paralized(void) {
    // Sleep until repower
    WriteReg(0, 0x8F, 0x00);
#ifdef _DEBUG_MODE
    debugf("\r\nvtx paralized");
#endif
    while (1) {
        LED_Flip();
        WAIT(50);
    }
}

void timer_task() {
    static uint16_t cur_ms10x_1sd16 = 0, last_ms10x_1sd16 = 0;
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

#ifdef _RF_CALIB
    return;
#endif

    if (tramp_lock)
        return;

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
            rf_delay_init_done = 1;
        }
        DM6300_AUXADC_Calib();
    }
}