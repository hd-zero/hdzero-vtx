#ifndef __I2C_DEVICE_H_
#define __I2C_DEVICE_H_

#include "common.h"

#define ADDR_MAX7315 0x20 // Rev1 Rev3
#define ADDR_PCA9554 0x38 // Rev2

#define ADDR_TP2825  0x44
#define ADDR_TC3587  0x0E
#define ADDR_EEPROM  0x50
#define ADDR_TEMPADC 0x48
#define ADDR_RUNCAM  0x21

void set_segment(uint32_t val);
#ifdef USE_TC3587_LED
void LED_TC3587_Init();
#endif
void Init_TC3587(uint8_t fmt);

extern uint8_t USE_MAX7315;
extern uint8_t USE_PCA9554;

#define ADDR_KEYBOARD (USE_MAX7315 ? ADDR_MAX7315 : ADDR_PCA9554)

#endif /* __I2C_DEVICE_H_ */
