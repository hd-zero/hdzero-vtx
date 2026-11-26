#ifndef __SPI_H_
#define __SPI_H_

#include "stdint.h"

void SPI_Write_Byte(uint8_t dat);
uint8_t SPI_Read_Byte();
void SPI_Write(uint8_t trans, uint16_t addr, uint32_t dat_l);
void SPI_Write3(uint16_t addr, uint32_t dat);
void SPI_Write6(uint16_t addr, uint32_t dat);
void SPI_Read(uint8_t trans, uint16_t addr, uint32_t *dat_l);
void SPI_Read3(uint16_t addr, uint32_t *dat);
void SPI_Read6(uint16_t addr, uint32_t *dat);
void SPI_Init();

#endif /* __SPI_H_ */
