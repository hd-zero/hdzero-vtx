#ifndef __SPI_H_
#define __SPI_H_

void SPI_Write(uint8_t trans, uint16_t addr, uint32_t dat_h, uint32_t dat_l);
void SPI_Read (uint8_t trans, uint16_t addr, uint32_t* dat_h, uint32_t* dat_l);
void SPI_Init();


#endif
