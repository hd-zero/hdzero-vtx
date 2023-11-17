#include "spi.h"

#include "common.h"
#include "global.h"
#include "print.h"

#define SET_CS(n) SPI_CS = n
#define SET_CK(n) SPI_CK = n
#define SET_DO(n) SPI_DO = n
#define SET_DI(n) SPI_DI = n

#define SPI_DLY     \
    {               \
        int i = 1;  \
        while (i--) \
            ;       \
    }

void SPI_Init() {
    SET_CS(1);
    SET_CK(0);
    SET_DO(0);
    SET_DI(1);
}

void SPI_Write_Byte(uint8_t dat) {
    int8_t i;
    for (i = 7; i >= 0; i--) {
        SPI_DLY;

        SET_DO((dat >> i) & 0x01);
        SPI_DLY;

        SET_CK(1);
        SPI_DLY;

        SET_CK(0);
    }
}

uint8_t SPI_Read_Byte() {
    int8_t i;
    uint8_t ret = 0;

    for (i = 7; i >= 0; i--) {
        SPI_DLY;

        SET_CK(1);
        SPI_DLY;

        ret = (ret << 1) | SPI_DI;
        SET_CK(0);
    }

    return ret;
}

void SPI_Write(uint8_t trans, uint16_t addr, uint32_t dat_l) {
    uint32_t rh = 0, rl = 0;
    uint16_t rlh = 0, rll = 0;

    int i, N;
    uint8_t byte;

    if (trans == 6)
        N = 2;
    else
        N = trans + 1;

    byte = 0x80 | (trans << 4) | (addr >> 8);

    // start SPI timing
    SET_CS(0);
    SPI_DLY;
    SPI_DLY; // start

    SPI_Write_Byte(byte);
    byte = addr & 0xFF;
    SPI_Write_Byte(byte);

    for (i = N - 1; i >= 0; i--) {
        if (i >= 4) {
            SPI_Write_Byte(0);
        } else {
            byte = (dat_l >> (i << 3)) & 0xFF;
            SPI_Write_Byte(byte);
        }
    }

    SPI_DLY;
    SPI_DLY;
    SET_CS(1); // end
    SET_CK(0);
    SET_DO(0);

#ifdef _DEBUG_SPI
    SPI_Read(trans, addr, &rl);
    if (dat_l != rl)
        debugf("                           --- W or R error !!   wdat=%lx", dat_l);
#endif
}

void SPI_Read(uint8_t trans, uint16_t addr, uint32_t *dat_l) {
    int i, N;
    uint8_t byte;
    uint16_t rlh = 0, rll = 0;

    if (trans == 6)
        N = 2;
    else
        N = trans + 1;

    byte = 0x00 | (trans << 4) | (addr >> 8);

    // start SPI timing
    SET_CS(0);
    SPI_DLY;
    SPI_DLY; // start

    SPI_Write_Byte(byte);
    byte = addr & 0xFF;
    SPI_Write_Byte(byte);

    // WAIT(100);

    for (i = N - 1; i >= 0; i--) {
        if (i >= 4) {
            SPI_Read_Byte();
        } else {
            *dat_l = (*dat_l) << 8;
            *dat_l |= SPI_Read_Byte();
        }
    }

    SPI_DLY;
    SPI_DLY;
    SET_CS(1); // end
    SET_CK(0);
    SET_DO(0);

#ifdef _DEBUG_SPI
    rlh = ((*dat_l) >> 16) & 0xFFFF;
    rll = (*dat_l) & 0xFFFF;
    debugf("\r\nSPI READ: addr=%x  data=%x%x.\r\n", addr, rlh, rll);
#endif
}
