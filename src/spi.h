#ifndef __SPI_H_
#define __SPI_H_

#include "stdint.h"

void SPI_Write(uint8_t trans, uint16_t addr, uint32_t dat_l);
void SPI_Read(uint8_t trans, uint16_t addr, uint32_t *dat_l);
void SPI_Init();

#endif /* __SPI_H_ */
