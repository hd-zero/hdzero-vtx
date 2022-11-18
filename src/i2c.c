#include "i2c.h"

#include "common.h"
#include "global.h"
#include "print.h"

#define SCL_SET(n) SCL = n
#define SDA_SET(n) SDA = n

#define SCL_GET() SCL
#define SDA_GET() SDA

#ifdef SDCC
void delay_10us() {
    __asm__(
        "mov r7,#91\n"
        "00000$:\n"
        "djnz r7,00000$\n");
}
#define DELAY_Q delay_10us()
#else
#define DELAY_Q                     \
    {                               \
        int i = (I2C_BIT_DLY >> 2); \
        while (i--)                 \
            ;                       \
    }
#endif

void I2C_start() {
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

void I2C_stop() {
    SDA_SET(0);
    DELAY_Q;

    SCL_SET(1);
    DELAY_Q;

    SDA_SET(1);
    DELAY_Q;
    DELAY_Q;
}

uint8_t I2C_ack() {
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

uint8_t I2C_write_byte(uint8_t val) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (val >> 7)
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

uint8_t I2C_Write8(uint8_t slave_addr, uint8_t reg_addr, uint8_t val) {
    uint8_t slave = slave_addr << 1;
    uint8_t value;

    I2C_start();

    value = I2C_write_byte(slave);
    if (value) {
        I2C_stop();
        return 1;
    }

    I2C_write_byte(reg_addr);

    // data
    I2C_write_byte(val);

    I2C_stop();

    value = I2C_Read8(slave_addr, reg_addr);
    // debugf("\r\n0x%4x, 0x%4x", reg_addr, (uint16_t)val);
    return 0;
}

uint8_t I2C_Write8_Wait(uint16_t ms, uint8_t slave_addr, uint8_t reg_addr, uint8_t val) {
    WAIT(ms);
    // debugf("\r\nEEP:0x%04x, 0x%04x", reg_addr, (uint16_t)val);
    return I2C_Write8(slave_addr, reg_addr, val);
}

uint8_t I2C_Write16(uint8_t slave_addr, uint16_t reg_addr, uint16_t val) {
    uint8_t slave, reg_addr_1, value;
    uint16_t rdat;

    slave = slave_addr << 1;

    I2C_start();

    value = I2C_write_byte(slave);
    if (value) {
        I2C_stop();
        return 1;
    }

    // reg addr
    reg_addr_1 = reg_addr >> 8;
    I2C_write_byte(reg_addr_1);

    reg_addr_1 = reg_addr & 0xFF;
    I2C_write_byte(reg_addr_1);

    // data
    value = val >> 8;
    I2C_write_byte(value);

    value = val;
    I2C_write_byte(value);

    I2C_stop();

    rdat = I2C_Read16(slave_addr, reg_addr);
    // debugf("\r\n0x%4x, 0x%4x", reg_addr, (uint16_t)val);
    return 0;
}

uint8_t I2C_read_byte(uint8_t no_ack) {
    uint8_t i;
    uint8_t val = 0;

    for (i = 0; i < 8; i++) {
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

uint8_t I2C_Read8(uint8_t slave_addr, uint8_t reg_addr) {
    uint8_t slave, val;
    slave = slave_addr << 1;

    I2C_start();

    I2C_write_byte(slave);

    I2C_write_byte(reg_addr);

    I2C_start();

    I2C_write_byte(slave | 0x01);

    // data
    val = I2C_read_byte(1);

    I2C_stop();

    return val;
}

uint8_t I2C_Read8_Wait(uint16_t ms, uint8_t slave_addr, uint8_t reg_addr) {
    WAIT(ms);
    return I2C_Read8(slave_addr, reg_addr);
}

uint16_t I2C_Read16(uint8_t slave_addr, uint16_t reg_addr) {
    uint8_t slave = slave_addr << 1;
    uint8_t reg_addr_1, val;
    uint16_t value = 0;

    I2C_start();

    I2C_write_byte(slave);

    // reg addr
    reg_addr_1 = reg_addr >> 8;
    I2C_write_byte(reg_addr_1);

    reg_addr_1 = reg_addr & 0xFF;
    I2C_write_byte(reg_addr_1);

    I2C_start();

    I2C_write_byte(slave | 0x01);

    // data
    val = I2C_read_byte(0);
    value = (value << 8) | val;

    val = I2C_read_byte(1);
    value = (value << 8) | val;

    I2C_stop();

    return value;
}

/////////////////////////////////////////////////////////////////
// runcam I2C
uint8_t RUNCAM_Write(uint8_t cam_id, uint32_t addr, uint32_t val) {
    uint8_t value;

    I2C_start(); // start

    value = I2C_write_byte(cam_id); // slave
    if (value) {
        I2C_stop();
#ifdef _DEBUG_RUNCAM
        debugf("\r\nRUNCAM_Write error id: %x value: %d", cam_id, value);
#endif
        return 1;
    }

    I2C_write_byte(0x12); // write cmd

    value = (addr >> 16) & 0xFF; // ADDR[23:16]
    I2C_write_byte(value);

    value = (addr >> 8) & 0xFF; // ADDR[15:8]
    I2C_write_byte(value);

    value = addr & 0xFF; // ADDR[7:0]
    I2C_write_byte(value);

    value = (val >> 24) & 0xFF; // DATA[31:24]
    I2C_write_byte(value);

    value = (val >> 16) & 0xFF; // DATA[23:16]
    I2C_write_byte(value);

    value = (val >> 8) & 0xFF; // DATA[15:8]
    I2C_write_byte(value);

    value = val & 0xFF; // DATA[7:0]
    I2C_write_byte(value);

    I2C_stop(); // stop

    return 0;
}

uint32_t RUNCAM_Read(uint8_t cam_id, uint32_t addr) {
    uint8_t value;
    uint32_t ret = 0;

    I2C_start(); // start

    I2C_write_byte(cam_id); // slave

    I2C_write_byte(0x13); // read cmd

    value = (addr >> 16) & 0xFF; // ADDR[23:16]
    I2C_write_byte(value);

    value = (addr >> 8) & 0xFF; // ADDR[15:8]
    I2C_write_byte(value);

    value = addr & 0xFF; // ADDR[7:0]
    I2C_write_byte(value);

    I2C_start(); // start

    I2C_write_byte(cam_id | 0x01); // slave | 0x01

    value = I2C_read_byte(0); // read DATA[31:24]
    ret = (ret << 8) | value;

    value = I2C_read_byte(0); // read DATA[23:16]
    ret = (ret << 8) | value;

    value = I2C_read_byte(0); // read DATA[15:8]
    ret = (ret << 8) | value;

    value = I2C_read_byte(1); // read DATA[7:0]
    ret = (ret << 8) | value;

    I2C_stop(); // stop

    return ret;
}
