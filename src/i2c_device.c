#include "i2c_device.h"

#include "camera.h"
#include "common.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "print.h"

/////////////////////////////////////////////////////////////////
// MAX7315
/*
(1) 0 -> light up;  1-> turn off
(2) blink phase 0 reg: 0x01
    blink phase 1 reg: 0x09
     _______
    |  [0]  |
 [5]|       |[1]
    |_______|
    |  [6]  |
 [4]|       |[2]
    |_______| o[7]
       [3]

    0 -> 0xC0
    1 -> 0xF9
    2 -> 0xA4
    3 -> 0xB0
    4 -> 0x99
    5 -> 0x92
    6 -> 0x82
    7 -> 0xF8
    8 -> 0x80
    9 -> 0x90
    A -> 0x88
    b -> 0x83
    E -> 0x86
    r -> 0xAF
    F -> 0x8E

    (3) reg 0x0F: blink flip
        [0]: 1 -> flip enable
        [1]: 0 -> phase0; 1 -> phase1
*/

uint8_t USE_MAX7315 = 0;
uint8_t USE_PCA9554 = 0;

void Set_MAX7315(uint32_t val) {
    uint16_t d;
    if (USE_MAX7315) {
        d = val;
        d <<= 8;
        d |= val;
        I2C_Write16(ADDR_KEYBOARD, 0x02, d);
        I2C_Write16(ADDR_KEYBOARD, 0x06, 0);
    } else {
        I2C_Write8(ADDR_KEYBOARD, 0x01, val);  // set blink phase 0
        I2C_Write8(ADDR_KEYBOARD, 0x0F, 0x01); // use phase 0
        I2C_Write8(ADDR_KEYBOARD, 0x03, 0x00); // set MAX7315 data pin output
    }
}

/////////////////////////////////////////////////////////////////
// TC3587
#ifdef HDZERO_FREESTYLE
void LED_TC3587_Init() {
    I2C_Write16(ADDR_TC3587, 0x0002, 0x0001);
    I2C_Write16(ADDR_TC3587, 0x0002, 0x0000); // srst

    I2C_Write16(ADDR_TC3587, 0x000E, 0x8000); // GPIO PD[23]--GPIO[15]
    I2C_Write16(ADDR_TC3587, 0x0010, 0x7FFF); // output
    I2C_Write16(ADDR_TC3587, 0x0014, 0x8000); // output
}
#endif

void Init_TC3587(uint8_t fmt) {
    uint16_t val = 0;

#ifdef TC3587_RSTB
    TC3587_RSTB = 0;
    WAIT(80);
    TC3587_RSTB = 1;
    WAIT(20);
#endif

#ifdef _DEBUG_TC3587
    debugf("\r\nDetecte TC3587...");
#endif
    while (1) {
        val = I2C_Read16(ADDR_TC3587, 0x0000);
        if (val == 0x4401) {
            LED_BLUE_ON;
            led_status = ON;
            break;
        } else {
            if (led_status == ON) {
                LED_BLUE_OFF;
                led_status = OFF;
                WAIT(50);
            } else {
                LED_BLUE_ON;
                led_status = ON;
                WAIT(20);
            }
        }
    }
    I2C_Write16(ADDR_TC3587, 0x0002, 0x0001);
    I2C_Write16(ADDR_TC3587, 0x0002, 0x0000); // srst

    I2C_Write16(ADDR_TC3587, 0x0004, 0x026f); // config

    I2C_Write16(ADDR_TC3587, 0x0006, 0x0062); // fifo
    I2C_Write16(ADDR_TC3587, 0x0008, 0x0061); // data format

    I2C_Write16(ADDR_TC3587, 0x0060, 0x800a); // mipi phy timing delay ; 0x800a

    I2C_Write16(ADDR_TC3587, 0x0018, 0x0111);     // pll
    I2C_Write16(ADDR_TC3587, 0x0018, 0x0113);     // pll
    if (fmt == 0)                                 // 50/60fps
        I2C_Write16(ADDR_TC3587, 0x0016, 0x3057); // pll
    else if (fmt == 1)                            // 90fps
        I2C_Write16(ADDR_TC3587, 0x0016, 0x808b); // pll
    I2C_Write16(ADDR_TC3587, 0x0020, 0x0000);     // clk config
    I2C_Write16(ADDR_TC3587, 0x000c, 0x0101);     // mclk
    I2C_Write16(ADDR_TC3587, 0x0018, 0x0111);     // pll
    I2C_Write16(ADDR_TC3587, 0x0018, 0x0113);     // pll
    I2C_Write16(ADDR_TC3587, 0x0002, 0x0001);
    I2C_Write16(ADDR_TC3587, 0x0002, 0x0000);
    // WAIT(100);
#ifdef _DEBUG_TC3587
    debugf("\r\nInit TC3587 done");
#endif
}
