#include "global.h"

#include "common.h"
#include "print.h"

#ifdef _DEBUG_MODE

uint8_t stricmp(uint8_t *ptr1, uint8_t *ptr2) {
    uint8_t lhs, rhs;
    while (1) {
        lhs = *ptr1;
        ptr1++;

        rhs = *ptr2;
        ptr2++;

        if (lhs != rhs)
            return 1;

        if ((lhs == 0) && (rhs == 0))
            break;

        if ((lhs == 0) || (rhs == 0))
            return 1;
    }
    return 0;
}

// trick the pre-compiler into doing some generic programing for us
// this avoids any type-conversions in the following functions
// as res already has the type we want
#define hex_to_int(res, val)           \
    if (val >= '0' && val <= '9')      \
        res += (val - '0');            \
    else if (val >= 'a' && val <= 'f') \
        res += (val - 'a' + 0x0a);     \
    else if (val >= 'A' && val <= 'F') \
        res += (val - 'A' + 0x0a);     \
    else                               \
        res += (0x00);

// s is 2-digit hex string, such as "1a","3c" ...  => 0x1a, 0x3c
uint8_t Asc2Bin(uint8_t *s) {
    uint8_t bin = 0;
    while (*s != '\0' && *s != ' ') {
        bin = bin << 4;
        hex_to_int(bin, *s);
        s++;
    }
    return (bin);
}

// s is 4-digit hex string
uint16_t Asc4Bin(uint8_t *s) {
    uint16_t bin = 0;
    while (*s != '\0' && *s != ' ') {
        bin = bin << 4;
        hex_to_int(bin, *s);
        s++;
    }
    return (bin);
}

// s is 8-digit hex string
uint32_t Asc8Bin(uint8_t *s) {
    uint32_t bin = 0;

    while (*s != '\0' && *s != ' ') {
        bin = bin << 4;
        hex_to_int(bin, *s);
        s++;
    }

    return bin;
}

#endif

#ifdef SDCC
void WAIT(uint16_t ms) {
    while (ms--) {
        uint16_t __ticks = MS_DLY_SDCC; // Measured manually by trial and error
        do {                            // 40 ticks total for this loop
            __ticks--;
        } while (__ticks);
    }
}
#else
void WAIT(uint32_t ms) {
    uint32_t i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < MS_DLY; j++) {
        }
    }
}
#endif

void uint8ToString(uint8_t dec, uint8_t *Str) {
    uint8_t val = dec / 100;
    if (val == 0) {
        Str[0] = ' ';
    } else {
        Str[0] = '0' + val;
    }

    val = (dec - (val * 100)) / 10;
    if (val == 0 && Str[0] == ' ') {
        Str[1] = ' ';
    } else {
        Str[1] = '0' + val;
    }

    val = dec % 10;
    Str[2] = '0' + val;
    Str[3] = '\0';
}