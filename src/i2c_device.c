#include "i2c_device.h"

#include "camera.h"
#include "common.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "print.h"

extern uint8_t g_camera_id;
extern uint8_t g_manual_camera_sel;

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

void set_segment(uint32_t val) {
    uint16_t d;

    // Rev3
    d = val;
    d <<= 8;
    d |= val;
    I2C_Write16_a8(ADDR_KEYBOARD, 0x02, d);
    I2C_Write16_a8(ADDR_KEYBOARD, 0x06, 0);

    // Rev1 & Rev2
    I2C_Write8(ADDR_KEYBOARD, 0x01, val);  // set blink phase 0
    I2C_Write8(ADDR_KEYBOARD, 0x0F, 0x01); // use phase 0
    I2C_Write8(ADDR_KEYBOARD, 0x03, 0x00); // set MAX7315 data pin output
}

/////////////////////////////////////////////////////////////////
// TC3587
#ifdef USE_TC3587_LED
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

    LED_BLUE_ON;
    led_status = ON;

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
        I2C_Write16(ADDR_TC3587, 0x0016, 0x809f); // pll
    I2C_Write16(ADDR_TC3587, 0x0020, 0x0000);     // clk config
    I2C_Write16(ADDR_TC3587, 0x000c, 0x0101);     // mclk
    I2C_Write16(ADDR_TC3587, 0x0018, 0x0111);     // pll
    I2C_Write16(ADDR_TC3587, 0x0018, 0x0113);     // pll
    I2C_Write16(ADDR_TC3587, 0x0002, 0x0001);
    I2C_Write16(ADDR_TC3587, 0x0002, 0x0000);
}

/////////////////////////////////////////////////////////////////
// Camera Expander Switch

// Read from camera switch
uint8_t pi4io_get(uint8_t reg) {
    return I2C_Read8(ADDR_PI4IO, reg);
}

// Write to camera switch
void pi4io_set(uint8_t reg, uint8_t val) {
    I2C_Write8(ADDR_PI4IO, reg, val);
}

// Select camera 1 or camera 2 and whether the i2c is shared
void select_camera(uint8_t camera_id, uint8_t sync_config) {
    uint8_t command;
    switch (camera_id) {
    case 1:
    default:
        command = 0x40;
        if (!sync_config) {
            command |= 0x04;
        }
        break;    
    case 2:
        command = 0x60;
        if (!sync_config) {
            command |= 0x01;
        }
        break;
    }
    pi4io_set(0x05, command);
}

void init_camera_switch() {
    pi4io_set(0x03, 0x77); // P7 and P3 are inputs
    g_manual_camera_sel = 0;
    select_camera(1, 1); // camera 1 is default, synchronised config
}

// Check if manual camera selection is enabled and switch if required
// (called from loop)
void manual_select_camera(void) {
    uint8_t command = pi4io_get(0x0F);
    g_manual_camera_sel = (command & 0x80);
    if (g_manual_camera_sel) {
        uint8_t camera_id = ((command & 0x08) >> 3) + 1;
        if (camera_id != g_camera_id) {
            g_camera_id = camera_id;
            select_camera(camera_id, 1);
        }
    }
}
