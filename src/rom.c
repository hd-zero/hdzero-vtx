#include "common.h"
#include "rom.h"
#include "uart.h"
#include "hardware.h"
#include "dm6300.h"
#include "global.h"
#include "i2c.h"
#include "i2c_device.h"
#include "spi.h"

#ifdef _RF_CALIB
#define CAL_BUF_MAX 8

void CalibProc()
{
    static uint8_t rxbuf[CAL_BUF_MAX];
    static int cnt = 0;
    uint8_t rx;
    uint32_t dcoc;
    
    if(Rom_ready()){
        rx = Rom_rx();
        
        if(rx != 0xFF){
            rxbuf[cnt++] = rx;
            if(cnt == CAL_BUF_MAX)
                cnt = 0;
        }
        else {
            switch(rxbuf[0]){
                case 'r':
                    for(cnt = 0; cnt < (FREQ_MAX+1) * (POWER_MAX+1); cnt++){
                        Rom_tx(table_power[cnt/(POWER_MAX+1)][cnt%(POWER_MAX+1)]);
                    }
                    break;
                
                case 'w':
                    table_power[RF_FREQ][RF_POWER] = rxbuf[1];

                    DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);

                    WAIT(10);
                    I2C_Write(ADDR_EEPROM, RF_FREQ * (POWER_MAX+1) + RF_POWER, table_power[RF_FREQ][RF_POWER], 0, 0);
                    break;
                
                case 'c':
                    if(rxbuf[1] <= FREQ_MAX && rxbuf[2] <= POWER_MAX){
                        RF_FREQ = rxbuf[1];
                        RF_POWER = rxbuf[2];
                        DM6300_SetChannel(RF_FREQ);
                        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                    }
                    break;
                    
                case 'f':
                    WriteReg(0, 0x8F, 0x01);
                    break;
                
                case 'o':
                    WriteReg(0, 0x8F, 0x11);
                    break;
                
                case 'D':
                    switch(rxbuf[1]){
                        case '1':
                            SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
                            dcoc = dcoc_ih | 0x8080;
                            SPI_Write(0x3, 0x380, 0x00000000, dcoc);
                            dcoc = dcoc_qh | 0x8080;
                            SPI_Write(0x3, 0x388, 0x00000000, dcoc);
                            break;
                        
                        case '2':
                            SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
                            dcoc = dcoc_ih | 0x80C0;
                            SPI_Write(0x3, 0x380, 0x00000000, dcoc);
                            dcoc = dcoc_qh | 0x8080;
                            SPI_Write(0x3, 0x388, 0x00000000, dcoc);
                            break;
                        
                        case '3':
                            SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
                            dcoc = dcoc_ih | 0xC080;
                            SPI_Write(0x3, 0x380, 0x00000000, dcoc);
                            dcoc = dcoc_qh | 0x8080;
                            SPI_Write(0x3, 0x388, 0x00000000, dcoc);
                            break;
                        
                        case '4':
                            SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
                            dcoc = dcoc_ih | 0x8080;
                            SPI_Write(0x3, 0x380, 0x00000000, dcoc);
                            dcoc = dcoc_qh | 0x80C0;
                            SPI_Write(0x3, 0x388, 0x00000000, dcoc);
                            break;
                        
                        case '5':
                            SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
                            dcoc = dcoc_ih | 0x8080;
                            SPI_Write(0x3, 0x380, 0x00000000, dcoc);
                            dcoc = dcoc_qh | 0xC080;
                            SPI_Write(0x3, 0x388, 0x00000000, dcoc);
                            break;
                        
                        case 'S':
                            // dcoc cfg
                            SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
                            dcoc = rxbuf[2];
                            dcoc <<= 8;
                            dcoc |= rxbuf[3];
                            dcoc |= dcoc_ih;
                            SPI_Write(0x3, 0x380, 0x00000000, dcoc);
                            dcoc = rxbuf[4];
                            dcoc <<= 8;
                            dcoc |= rxbuf[5];
                            dcoc |= dcoc_qh;
                            SPI_Write(0x3, 0x388, 0x00000000, dcoc);
                        
                            // write to eeprom
                            WAIT(10); I2C_Write(ADDR_EEPROM, EEP_ADDR_DCOC_EN, 0x00, 0, 0);
                            WAIT(10); I2C_Write(ADDR_EEPROM, EEP_ADDR_DCOC_IH, rxbuf[2], 0, 0);
                            WAIT(10); I2C_Write(ADDR_EEPROM, EEP_ADDR_DCOC_IL, rxbuf[3], 0, 0);
                            WAIT(10); I2C_Write(ADDR_EEPROM, EEP_ADDR_DCOC_QH, rxbuf[4], 0, 0);
                            WAIT(10); I2C_Write(ADDR_EEPROM, EEP_ADDR_DCOC_QL, rxbuf[5], 0, 0);
                            break;
                    }
                    break;
            }
            
            cnt = 0;
        }
    }
}
#endif