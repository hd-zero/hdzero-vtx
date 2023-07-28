#ifndef __DM6300_H_
#define __DM6300_H_

#include "common.h"
#include "hardware.h"

#ifdef HDZERO_FREESTYLE
#define PIT_POWER 0x18 // 2dbm
#else
#define PIT_POWER 0x26
#endif

void DM6300_Init(uint8_t ch, uint8_t bw);
void DM6300_EFUSE1();
void DM6300_EFUSE2();
// void DM6300_CalibRF();
void DM6300_SetChannel(uint8_t ch);
uint8_t DM6300_GetChannelByFreq(uint16_t const freq);
uint16_t DM6300_GetFreqByChannel(uint8_t const ch);
void DM6300_SetPower(uint8_t pwr, uint8_t freq, uint8_t offset);
int16_t DM6300_GetTemp();
void DM6300_AUXADC_Calib();

void DM6300_init1();
void DM6300_init2(uint8_t sel);
void DM6300_init3(uint8_t ch);
void DM6300_init4();
void DM6300_init5(uint8_t sel);
void DM6300_init6(uint8_t sel);
void DM6300_init7(uint8_t sel);
void DM6300_RFTest();
// void DM6300_M0();

extern int16_t auxadc_offset;
extern uint8_t table_power[FREQ_NUM][POWER_MAX + 1];

extern uint32_t dcoc_ih, dcoc_qh;

extern uint8_t dm6300_init_done;
#endif /* __DM6300_H_ */
