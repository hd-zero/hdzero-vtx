#ifndef __I2C_H_
#define __I2C_H_

#include "stdint.h"

uint8_t I2C_Write8(uint8_t slave_addr, uint8_t reg_addr, uint8_t val);
uint8_t I2C_Write8_Wait(uint16_t ms, uint8_t slave_addr, uint8_t reg_addr, uint8_t val);
uint8_t I2C_Write16(uint8_t slave_addr, uint16_t reg_addr, uint16_t val);
uint8_t I2C_Write16_a8(uint8_t slave_addr, uint8_t reg_addr, uint16_t val);

uint8_t I2C_Read8(uint8_t slave_addr, uint8_t reg_addr);
uint8_t I2C_Read8_Wait(uint16_t ms, uint8_t slave_addr, uint8_t reg_addr);
uint16_t I2C_Read16(uint8_t slave_addr, uint16_t reg_addr);
uint16_t I2C_Read16_a8(uint8_t slave_addr, uint8_t reg_addr);

uint8_t RUNCAM_Write(uint8_t cam_id, uint32_t addr, uint32_t val);
uint32_t RUNCAM_Read(uint8_t cam_id, uint32_t addr);
uint8_t RUNCAM_Read_Write(uint8_t cam_id, uint32_t addr, uint32_t val);

#endif /* __I2C_H_ */
