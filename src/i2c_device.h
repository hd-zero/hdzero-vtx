#ifndef __I2C_DEVICE_H_
#define __I2C_DEVICE_H_

#include "common.h"

#define ADDR_MAX7315 0x20 // Rev1 Rev3
#define ADDR_PCA9554 0x38 // Rev2

#define ADDR_TP9950  0x44
#define ADDR_TC3587  0x0E
#define ADDR_EEPROM  0x50
#define ADDR_TEMPADC 0x48
#define ADDR_RUNCAM  0x21
#define ADDR_PI4IO   0x43 // 2 camera switch
#define ADDR_PCA9557 0x18 // 3 camera switch

typedef enum {
    SWITCH_TYPE_NONE,
    SWITCH_TYPE_PI4IO,
    SWITCH_TYPE_PCA9557
} switch_type_e;

typedef enum {
    PI4IO_CAMS = 2,
    PCA9557_CAMS = 3
} switch_cams_e;

void set_segment(uint32_t val);
#ifdef USE_TC3587_LED
void LED_TC3587_Init();
#endif
void Init_TC3587(uint8_t fmt);

uint8_t pi4io_get(uint8_t reg);
void pi4io_set(uint8_t reg, uint8_t val);
void camera_switch_init(void);
void select_camera(uint8_t camera_id);
void manual_select_camera(void);

extern uint8_t USE_MAX7315;
extern uint8_t USE_PCA9554;

#define ADDR_KEYBOARD (USE_MAX7315 ? ADDR_MAX7315 : ADDR_PCA9554)

#endif /* __I2C_DEVICE_H_ */
