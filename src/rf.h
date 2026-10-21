#ifndef __RF_H__
#define __RF_H__

#include "common.h"
#include "hardware.h"
#include "stdint.h"

typedef enum {
    RF_CHIP_DM6300,
    RF_CHIP_DM6300B,
} rf_chip_e;

uint8_t rf_get_ch_by_freq(uint16_t const freq);
uint16_t rf_get_freq_by_ch(uint8_t const ch);

void rf_chip_check();
void rf_init(uint8_t ch, uint8_t bw);
void rf_set_channel(uint8_t ch);
void rf_set_power(uint8_t pwr, uint8_t offset);
int16_t rf_get_temp();
void rf_auxadc_calib();
void rf_monitor_task();

extern rf_chip_e rf_chip;
extern uint8_t rf_init_done;
extern uint8_t rf_chip_lost;
#endif