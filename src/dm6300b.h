#ifndef __DM6300B_H__
#define __DM6300B_H__

#include "common.h"
#include "hardware.h"
#include "stdint.h"

void dm6300b_monitor_task(void);

void DM6300B_Init(uint8_t ch, BWType_e bw);
void DM6300B_SetChannel(uint8_t ch);
void DM6300B_SetPower(uint8_t pwr, uint8_t freq, uint8_t offset);

void DM6300B_spi_write(uint16_t addr, uint8_t val);
uint8_t DM6300B_spi_read(uint16_t addr);
void gc_tx_gain_set(uint8_t gain_index);

uint8_t DM6300B_is_alive();
#ifdef _RF_CALIB
void DM6300B_reset_table_power();
#endif
#endif