#include "common.h"
#include "i2c.h"
#include "print.h"
#include "global.h"

void I2C_start()
{
    SDA_SET(1);
    DELAY_Q;
    
    SCL_SET(1);
    DELAY_Q;
    
    SDA_SET(0);
    DELAY_Q;
    DELAY_Q;
    
    SCL_SET(0);
    DELAY_Q;
}

void I2C_stop()
{
    SDA_SET(0);
    DELAY_Q;
    
    SCL_SET(1);
    DELAY_Q;
    
    SDA_SET(1);
    DELAY_Q;
    DELAY_Q;
}

uint8_t I2C_ack()
{
    uint8_t ret;
    
    SDA_SET(1);
    DELAY_Q;
    
    SCL_SET(1);
    ret = SDA_GET();
    DELAY_Q;
    DELAY_Q;
    
    SCL_SET(0);
    DELAY_Q;
    
    return ret;
}

uint8_t I2C_write_byte(uint8_t val)
{
    uint8_t i;
    for(i=0;i<8;i++){
        if(val>>7)
            SDA_SET(1);
        else
            SDA_SET(0);
        DELAY_Q;
        
        SCL_SET(1);
        DELAY_Q;
        DELAY_Q;
        
        SCL_SET(0);
        DELAY_Q;
        
        val <<= 1;
    }
    
    i = I2C_ack();
    
    return i;
}

// addr16(reg_addr bits): 0 = 8bit; 1 = 16bit
// bnum(data bits): 0 = 8bit; 1 = 16bit; 2 = 24bit; 3 = 32 bit
uint8_t I2C_Write(uint8_t slave_addr, uint16_t reg_addr, uint32_t val, uint8_t addr16, uint8_t bnum)
{
    uint8_t slave, reg_addr_1,value;
    uint32_t rdat;
    
    slave = slave_addr << 1;
    
    I2C_start();
    
    value = I2C_write_byte(slave);
    if(value){
        I2C_stop();
        return 1;
    }
    
    // reg addr
    if(addr16){
        reg_addr_1 = reg_addr >> 8;
        I2C_write_byte(reg_addr_1);
    }
    
    reg_addr_1 = reg_addr &0xFF;
    I2C_write_byte(reg_addr_1);
    
    // data
    if(bnum > 2){
        value = val >> 24;
        I2C_write_byte(value);
    }
    if(bnum > 1){
        value = val >> 16;
        I2C_write_byte(value);
    }
    if(bnum > 0){
        value = val >> 8;
        I2C_write_byte(value);
    }
    value = val;
    I2C_write_byte(value);
    
    I2C_stop();
    
    rdat = I2C_Read(slave_addr, reg_addr, addr16, bnum);
    //Printf("\r\n0x%4x, 0x%4x", reg_addr, (uint16_t)val);
    return 0;
}

uint8_t I2C_read_byte(uint8_t no_ack)
{
    uint8_t i;
    uint8_t val = 0;
    
    for(i=0;i<8;i++){
        DELAY_Q;
        SCL_SET(1);
        
        val <<= 1;
        val |= SDA_GET();
        
        DELAY_Q;
        DELAY_Q;
        
        SCL_SET(0);
        DELAY_Q;
    }
    
    // master ack
    SDA_SET(no_ack);
    DELAY_Q;
    
    SCL_SET(1);
    DELAY_Q;
    DELAY_Q;
    
    SCL_SET(0);
    DELAY_Q;
    
    SDA_SET(1);
    
    return val;
}

// addr16(reg_addr bits): 0 = 8bit; 1 = 16bit
// bnum(data bits): 0 = 8bit; 1 = 16bit; 2 = 24bit; 3 = 32 bit
uint32_t I2C_Read(uint8_t slave_addr, uint16_t reg_addr, uint8_t addr16, uint8_t bnum)
{
    uint8_t slave, reg_addr_1, val, i, last;
    uint32_t value = 0;
    slave = slave_addr << 1;
    
    I2C_start();

    I2C_write_byte(slave);
    
    // reg addr
    if(addr16){
        reg_addr_1 = reg_addr >> 8;
        I2C_write_byte(reg_addr_1);
    }
    
    reg_addr_1 = reg_addr &0xFF;
    I2C_write_byte(reg_addr_1);
    
    I2C_start();
    
    I2C_write_byte(slave|0x01);
    
    // data
    for(i=bnum+1; i>0; i--){
        last = (i==1);
        val = I2C_read_byte(last);
        value = (value<<8) | val;
    }

    I2C_stop();

    return value;
}

/////////////////////////////////////////////////////////////////
// runcam I2C
uint8_t RUNCAM_Write(uint8_t cam_id, uint32_t addr, uint32_t val)
{
    uint8_t value;

    I2C_start();                 // start

    value = I2C_write_byte(cam_id);// slave
    if(value){
        I2C_stop();
        Printf("\r\nerror");
        return 1;
    }
    
    I2C_write_byte(0x12);        // write cmd
    
    value = (addr >> 16) & 0xFF; // ADDR[23:16]
    I2C_write_byte(value);
    
    value = (addr >> 8) & 0xFF;  // ADDR[15:8]
    I2C_write_byte(value);
    
    value = addr & 0xFF;         // ADDR[7:0]
    I2C_write_byte(value);
    
    value = (val >> 24) & 0xFF;  // DATA[31:24]
    I2C_write_byte(value);
    
    value = (val >> 16) & 0xFF;  // DATA[23:16]
    I2C_write_byte(value);
    
    value = (val >> 8) & 0xFF;   // DATA[15:8]
    I2C_write_byte(value);
    
    value = val & 0xFF;          // DATA[7:0]
    I2C_write_byte(value);
    
    I2C_stop();                  // stop

    return 0;
}

uint32_t RUNCAM_Read(uint8_t cam_id, uint32_t addr)
{
    uint8_t value;
    uint32_t ret = 0;
    
    I2C_start();                 // start
    
    I2C_write_byte(cam_id);      // slave
    
    I2C_write_byte(0x13);        // read cmd
    
    value = (addr >> 16) & 0xFF; // ADDR[23:16]
    I2C_write_byte(value);
    
    value = (addr >> 8) & 0xFF;  // ADDR[15:8]
    I2C_write_byte(value);
    
    value = addr & 0xFF;         // ADDR[7:0]
    I2C_write_byte(value);
    
    I2C_start();                 // start
    
    I2C_write_byte(cam_id | 0x01);        // slave | 0x01
    
    value = I2C_read_byte(0);    // read DATA[31:24]
    ret   = (ret << 8) | value;
    
    value = I2C_read_byte(0);    // read DATA[23:16]
    ret   = (ret << 8) | value;
    
    value = I2C_read_byte(0);    // read DATA[15:8]
    ret   = (ret << 8) | value;
    
    value = I2C_read_byte(1);    // read DATA[7:0]
    ret   = (ret << 8) | value;
    
    I2C_stop();                  // stop
    
    return ret;
}
