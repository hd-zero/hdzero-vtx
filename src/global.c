#include "common.h"
#include "global.h"
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

uint8_t Asc1Bin(uint8_t asc)
{
	if(asc>='0' && asc <='9') return (asc - '0');
	else if(asc>='a' && asc <='f') return (asc - 'a' + 0x0a);
	else if(asc>='A' && asc <='F') return (asc - 'A' + 0x0a);
    else return 0x00;
}

uint8_t Asc2Bin(uint8_t *s) // s is 2-digit hex string, such as "1a","3c" ...  => 0x1a, 0x3c
{
	uint8_t bin;

	bin = 0;
	while(*s != '\0' && *s !=' ') {
		bin = bin<<4;
		bin = bin + Asc1Bin(*s);
		s++;
	}
	return (bin);
}

uint16_t Asc4Bin(uint8_t *s) // s is 4-digit hex string
{
	uint16_t bin;

	bin = 0;
	while(*s != '\0' && *s !=' ') {
		bin = bin<<4;
		bin = bin + Asc1Bin(*s);
		s++;
	}
	return (bin);
}

uint32_t Asc8Bin(uint8_t *s) // s is 8-digit hex string
{
	uint32_t bin;

	bin = 0;
	while(*s != '\0' && *s !=' ') {
		bin = bin<<4;
		bin = bin + Asc1Bin(*s);
		s++;
	}
	return (bin);
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
void WAIT(uint32_t ms)
{
    uint32_t i,j;
    for(i=0;i<ms;i++) {
        for(j=0;j<MS_DLY;j++) {
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
    if (val == 0) {
        Str[1] = ' ';
    } else {
        Str[1] = '0' + val;
    }

    val = dec % 10;
    if (val == 0) {
        Str[2] = ' ';
    } else {
        Str[2] = '0' + val;
    }
}