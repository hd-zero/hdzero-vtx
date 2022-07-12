#ifndef __I2C_H_
#define __I2C_H_

#define SCL_SET(n)  SCL = n
#define SDA_SET(n)  SDA = n

#define SCL_GET()   SCL
#define SDA_GET()   SDA

#define DELAY_Q     {int i=(I2C_BIT_DLY>>2); while(i--);}

uint8_t I2C_Write(uint8_t slave_addr, uint16_t reg_addr, uint32_t val, uint8_t addr16, uint8_t bnum);
uint32_t I2C_Read(uint8_t slave_addr, uint16_t reg_addr, uint8_t addr16, uint8_t bnum);

uint8_t RUNCAM_Write(uint8_t cam_id, uint32_t addr, uint32_t val);
uint32_t RUNCAM_Read(uint8_t cam_id, uint32_t addr);

#endif