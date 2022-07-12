#include "common.h"
#include "i2c.h"
#include "i2c_device.h"
#include "print.h"
#include "global.h"
#include "camera.h"
#include "hardware.h"

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

void Init_MAX7315(uint32_t val)
{
    I2C_Write(ADDR_KEYBOARD, 0x01, val, 0, 0);  // set blink phase 0
    I2C_Write(ADDR_KEYBOARD, 0x0F, 0x01, 0, 0); // use phase 0
    I2C_Write(ADDR_KEYBOARD, 0x03, 0x00, 0, 0); // set MAX7315 data pin output
}

/////////////////////////////////////////////////////////////////
// TC3587
#ifdef VTX_L
void LED_TC3587_Init()
{
    I2C_Write(ADDR_TC3587, 0x0002, 0x0001, 1, 1);
    I2C_Write(ADDR_TC3587, 0x0002, 0x0000, 1, 1); //srst
    
    I2C_Write(ADDR_TC3587, 0x000E, 0x8000, 1, 1); //GPIO PD[23]--GPIO[15]
    I2C_Write(ADDR_TC3587, 0x0010, 0x7FFF, 1, 1); //output
    I2C_Write(ADDR_TC3587, 0x0014, 0x8000, 1, 1); //output 
}
#endif

void Init_TC3587()
{
    uint16_t val = 0;
    
    #ifdef TC3587_RSTB
	TC3587_RSTB = 0;
	WAIT(80);
	TC3587_RSTB = 1;
	WAIT(20);
	#endif
    
    I2C_Write(ADDR_TC3587, 0x0002, 0x0001, 1, 1);
    I2C_Write(ADDR_TC3587, 0x0002, 0x0000, 1, 1); //srst
    
    I2C_Write(ADDR_TC3587, 0x0004, 0x026f, 1, 1); //config
    
    I2C_Write(ADDR_TC3587, 0x0006, 0x0062, 1, 1); //fifo
    I2C_Write(ADDR_TC3587, 0x0008, 0x0061, 1, 1); //data format
    
    I2C_Write(ADDR_TC3587, 0x0060, 0x800a, 1, 1); //mipi phy timing delay ; 0x800a
    
    I2C_Write(ADDR_TC3587, 0x0018, 0x0111, 1, 1); //pll
    I2C_Write(ADDR_TC3587, 0x0018, 0x0113, 1, 1); //pll
    I2C_Write(ADDR_TC3587, 0x0016, 0x3057, 1, 1); //pll
    I2C_Write(ADDR_TC3587, 0x0020, 0x0000, 1, 1); //clk config
    I2C_Write(ADDR_TC3587, 0x000c, 0x0101, 1, 1); //mclk
    I2C_Write(ADDR_TC3587, 0x0018, 0x0111, 1, 1); //pll //0111
    I2C_Write(ADDR_TC3587, 0x0018, 0x0113, 1, 1); //pll //0113
    I2C_Write(ADDR_TC3587, 0x0002, 0x0001, 1, 1);
    I2C_Write(ADDR_TC3587, 0x0002, 0x0000, 1, 1);
    //WAIT(100);

#ifdef _DEBUG_TC3587
    val = I2C_Read(ADDR_TC3587, 0x0002, 1, 1);
    Printf("\r\n0x0002 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0004, 1, 1);
    Printf("\r\n0x0004 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0006, 1, 1);
    Printf("\r\n0x0006 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0008, 1, 1);
    Printf("\r\n0x0008 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x000C, 1, 1);
    Printf("\r\n0x000C = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0016, 1, 1);
    Printf("\r\n0x0016 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0018, 1, 1);
    Printf("\r\n0x0018 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0020, 1, 1);
    Printf("\r\n0x0020 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0060, 1, 1);
    Printf("\r\n0x0060 = 0x%x", val);

    val = I2C_Read(ADDR_TC3587, 0x0062, 1, 1);
    Printf("\r\n0x0062 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0064, 1, 1);
    Printf("\r\n0x0064 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0066, 1, 1);
    Printf("\r\n0x0066 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0068, 1, 1);
    Printf("\r\n0x0068 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x006A, 1, 1);
    Printf("\r\n0x006A = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x006C, 1, 1);
    Printf("\r\n0x006C = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x006E, 1, 1);
    Printf("\r\n0x006E = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0070, 1, 1);
    Printf("\r\n0x0070 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0080, 1, 1);
    Printf("\r\n0x0080 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0082, 1, 1);
    Printf("\r\n0x0082 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0084, 1, 1);
    Printf("\r\n0x0084 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0086, 1, 1);
    Printf("\r\n0x0086 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0088, 1, 1);
    Printf("\r\n0x0088 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x008A, 1, 1);
    Printf("\r\n0x008A = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x008C, 1, 1);
    Printf("\r\n0x008C = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x008E, 1, 1);
    Printf("\r\n0x008E = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x0090, 1, 1);
    Printf("\r\n0x0090 = 0x%x", val);
    val = I2C_Read(ADDR_TC3587, 0x00F8, 1, 1);
    Printf("\r\n0x00F8 = 0x%x", val);

    val = I2C_Read(ADDR_TC3587, 0x006A, 1, 1);
    Printf("\r\n0x006A = 0x%x", val);
#endif
}

