#ifndef __DM6300_H_
#define __DM6300_H_

#include "common.h"
#include "hardware.h"

void DM6300_Init(uint8_t ch, uint8_t bw);
void DM6300_SetChannel(uint8_t ch);
void DM6300_SetPower(uint8_t pwr, uint8_t freq, uint8_t offset);
int16_t DM6300_GetTemp();
void DM6300_AUXADC_Calib();
uint8_t DM6300_is_alive(void);
void DM6300_Set_Page18();

extern uint32_t dcoc_ih, dcoc_qh;
#endif /* __DM6300_H_ */
