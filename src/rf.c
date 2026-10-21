#include "rf.h"
#include "common.h"
#include "dm6300.h"
#include "dm6300b.h"
#include "print.h"
#include "spi.h"

const uint16_t frequencies[] = {
    FREQ_R1,
    FREQ_R2,
    FREQ_R3,
    FREQ_R4,
    FREQ_R5,
    FREQ_R6,
    FREQ_R7,
    FREQ_R8,
    FREQ_E1,
    FREQ_F1,
    FREQ_F2,
    FREQ_F4,
    FREQ_L1,
    FREQ_L2,
    FREQ_L3,
    FREQ_L4,
    FREQ_L5,
    FREQ_L6,
    FREQ_L7,
    FREQ_L8,
};

rf_chip_e rf_chip = RF_CHIP_DM6300;
uint8_t rf_chip_lost = 0;
uint8_t rf_init_done = 0;

static uint8_t cur_ch = 0;

void rf_chip_check() {
    WriteReg(0x8F, 0x00);
    WriteReg(0x8F, 0x01);

    if (DM6300_is_alive()) {
        rf_chip_lost = 0;
        rf_chip = RF_CHIP_DM6300;
#ifdef _DEBUG_MODE
        debugf("\r\nRF CHIP: DM6300");
#endif
    } else if (DM6300B_is_alive()) {
        rf_chip_lost = 0;
        rf_chip = RF_CHIP_DM6300B;
#ifdef _DEBUG_MODE
        debugf("\r\nRF CHIP: DM6300B");
#endif
    } else {
        rf_chip_lost = 1;
#ifdef _DEBUG_MODE
        debugf("\r\nRF CHIP: Unknown!!!");
#endif
    }

    WriteReg(0x8F, 0x00);
}

uint8_t rf_get_ch_by_freq(uint16_t const freq) {
    uint8_t iter;
    for (iter = 0; iter < ARRAY_SIZE(frequencies); iter++) {
        if (frequencies[iter] == freq) {
            return iter;
        }
    }
    return INVALID_CHANNEL;
}

uint16_t rf_get_freq_by_ch(uint8_t const ch) {
    if (ch < ARRAY_SIZE(frequencies)) {
        return frequencies[ch];
    }
    return 0;
}

void rf_init(uint8_t ch, uint8_t bw) {
    if (rf_chip == RF_CHIP_DM6300)
        DM6300_Init(ch, bw);
    else
        DM6300B_Init(ch, bw);

    cur_ch = ch;

#ifdef _DEBUG_MODE
    debugf("\r\nRF init ch=%d, bw=%d", ch, bw);
#endif
}

void rf_set_channel(uint8_t ch) {
    if (rf_chip == RF_CHIP_DM6300)
        DM6300_SetChannel(ch);
    else
        DM6300B_SetChannel(ch);

    cur_ch = ch;

#ifdef _DEBUG_MODE
    debugf("\r\nRF set ch %d", ch);
#endif
}

void rf_set_power(uint8_t pwr, uint8_t offset) {
    if (rf_chip == RF_CHIP_DM6300)
        DM6300_SetPower(pwr, cur_ch, offset);
    else
        DM6300B_SetPower(pwr, cur_ch, offset);
#ifdef _DEBUG_MODE
    debugf("\r\nRF set pwr %d ofst%d", pwr, offset);
#endif
}

int16_t rf_get_temp() {
    if (rf_chip == RF_CHIP_DM6300) {
        return DM6300_GetTemp();
    } else {
        return 0; // TODO
    }
}

void rf_auxadc_calib() {
    if (rf_chip == RF_CHIP_DM6300) {
        DM6300_AUXADC_Calib();
#ifdef _DEBUG_MODE
        debugf("\r\nRF auxadc calib");
#endif
    }
    rf_init_done = 1;
}

void rf_monitor_task() {
    if (rf_chip == RF_CHIP_DM6300B) {
        dm6300b_monitor_task();
    }
}