#include "i2c_device.h"

#include "camera.h"
#include "common.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "print.h"

uint8_t g_camera_switch;
uint8_t g_camera_id;
uint8_t g_manual_camera_sel;

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

// Read from 2 camera switch
uint8_t pi4io_get(uint8_t reg) {
    return I2C_Read8(ADDR_PI4IO, reg);
}

// Write to 2 camera switch
void pi4io_set(uint8_t reg, uint8_t val) {
    I2C_Write8(ADDR_PI4IO, reg, val);
}

// Read from 3 camera switch
uint8_t pca9557_get(uint8_t reg) {
    return I2C_Read8(ADDR_PCA9557, reg);
}

// Write to 3 camera switch
void pca9557_set(uint8_t reg, uint8_t val) {
    I2C_Write8(ADDR_PCA9557, reg, val);
}

uint8_t get_camera_switch_type(void) {
    uint8_t camera_switch = SWITCH_TYPE_NONE;

    if ((pi4io_get(0x01) & 0xE0) == 0xA0) { // device ID register
        camera_switch = SWITCH_TYPE_PI4IO;
    } else if (pca9557_get(0x02) == 0xF0) { // polarity inversion register defaults to 0xF0
        camera_switch = SWITCH_TYPE_PCA9557;
    }

    return camera_switch;
}

void select_camera(uint8_t camera_id) {
    if (g_camera_switch)
    {
        uint8_t command = 0;
        switch (camera_id) {
            case 1:
                command = (g_camera_switch == SWITCH_TYPE_PI4IO) ? 0x11 : 0x0D;
                break;
            case 2:
                command = (g_camera_switch == SWITCH_TYPE_PI4IO) ? 0x64 : 0x16;
                break;
            case 3:
                command = (g_camera_switch == SWITCH_TYPE_PI4IO) ? 0x00 : 0x1B;
                break;
            default:
                break; // do nothing
        }

        if (command) {
            if (g_camera_switch == SWITCH_TYPE_PI4IO) {
                pi4io_set(0x05, command);
            }
            else {
                pca9557_set(0x01, command);
                WAIT(200); // wait for camera power up 
            }

            camera_switch_profile();
        }
    }
}

void camera_switch_init() {
    g_manual_camera_sel = 0;
    g_camera_id = 1;
    g_camera_switch = get_camera_switch_type();
    if (g_camera_switch == SWITCH_TYPE_PI4IO) {
        //pi4io_set(0x01, 0xFF); // reset
        pi4io_set(0x0B, 0xFF); // Disable pullup/pulldown resistors
        pi4io_set(0x11, 0xFF); // Disable interrupts on inputs
        pi4io_get(0x13);       // De-assert the interrrupt 
        pi4io_set(0x03, 0x77); // Set P3 and P7 as inputs
        pi4io_set(0x07, 0x00); // Set outputs to follow the output port register
    } else if (g_camera_switch == SWITCH_TYPE_PCA9557) {
        pca9557_set(0x03, 0x00); // all outputs
    }
    select_camera(g_camera_id);
}

// Check if manual camera selection is enabled and switch if required
// (called from loop)
void manual_select_camera(void) {
    if (g_camera_switch == SWITCH_TYPE_PI4IO) {
        uint8_t command = pi4io_get(0x0F);
        g_manual_camera_sel = (command & 0x08);
        if (g_manual_camera_sel) {
            uint8_t camera_id = ((command & 0x80) >> 7) + 1;
            if (camera_id != g_camera_id) {
                g_camera_id = camera_id;
                select_camera(camera_id);
            }
        }
    }
}
