#include "lifetime.h"
#include "print.h"

uint32_t sysLifeTime = 0;
uint32_t sysLifeTime_last = 0;
uint8_t LifeTimeOP = 0;

void Get_EEP_LifeTime(void) {
    uint8_t u8;

    u8 = I2C_Read8(ADDR_EEPROM, EEP_ADDR_LIFETIME_0);
    sysLifeTime = (uint32_t)u8;
    u8 = I2C_Read8(ADDR_EEPROM, EEP_ADDR_LIFETIME_1);
    sysLifeTime += (uint32_t)u8 << 8;
    u8 = I2C_Read8(ADDR_EEPROM, EEP_ADDR_LIFETIME_2);
    sysLifeTime += (uint32_t)u8 << 16;
    u8 = I2C_Read8(ADDR_EEPROM, EEP_ADDR_LIFETIME_3);
    sysLifeTime += (uint32_t)u8 << 24;

    if (sysLifeTime == 0xffffffff) {
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_0, 0x00);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_1, 0x00);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_2, 0x00);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_3, 0x00);
        sysLifeTime = 0;
    }

    sysLifeTime_last = sysLifeTime;
}

void Update_EEP_LifeTime(void) {
#if !defined(_DEBUG_MODE) && !defined(VIDEO_PAT) && !defined(_RF_CALIB)
    uint8_t u8;
    uint32_t diff;
    static uint16_t lstSeconds = 0;

    if (seconds - lstSeconds >= 10) {
        sysLifeTime++;
        lstSeconds = seconds;
        if (sysLifeTime >= 3599999) // stop at 9999 hours
            sysLifeTime = 3599999;
    } else {
        return;
    }

    diff = sysLifeTime_last ^ sysLifeTime;

    if ((diff >> 0) & 0xff) {
        u8 = (sysLifeTime >> 0) & 0xff;
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_0, u8);
    }

    if ((diff >> 8) & 0xff) {
        u8 = (sysLifeTime >> 8) & 0xff;
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_1, u8);
    }

    if ((diff >> 16) & 0xff) {
        u8 = (sysLifeTime >> 16) & 0xff;
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_2, u8);
    }

    if ((diff >> 24) & 0xff) {
        u8 = (sysLifeTime >> 24) & 0xff;
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_3, u8);
    }

    sysLifeTime_last = sysLifeTime;
#endif
}

char *parseLifeTime(void) {
    static char lifetime[7]; // ex, "9999HR" or "59MIN"
    uint32_t hours = sysLifeTime / 360;
    uint32_t num;
    uint8_t pos = 0;

    if (hours == 0) { // display minutes for the first hour
    
        uint8_t minutes = (sysLifeTime % 360) / 6;
        num = (minutes % 60) / 10; // update every 10 minutes
        if (num > 0) {
            lifetime[pos++] = '0' + num;
        }
        lifetime[pos] = '0' + (minutes % 10);
        memcpy(lifetime+pos+1, "MIN", 4);

    } else { // then just display hours

        uint32_t num = hours;
        uint8_t digits = 0;
        while ( num != 0) {
            digits++;
            num /= 10;
        }

        // defensive check
        if (digits > (sizeof(lifetime) - 3)) {
            memcpy(lifetime, "ERROR", 6);

        } else {
            uint8_t pos = digits-1;
            while (hours != 0) {
                lifetime[pos--] = '0' + (hours % 10);
                hours /= 10;
            }
            memcpy(lifetime+digits, "HR", 3);
        }
    }

    return lifetime;
}
