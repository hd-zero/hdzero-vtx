#include "common.h"
#include "global.h"
#include "print.h"

#ifdef _DEBUG_MODE

int stricmp(uint8_t *ptr1, uint8_t *ptr2)
{
    int diff;
    
    while(1){
        diff = (*ptr1++) - (*ptr2++);
        if(diff) return 1;
        
        if((*ptr1==0) && (*ptr2==0))
            break;
        else if((*ptr1==0) || (*ptr2==0))
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

void uint8ToString(uint8_t dec, uint8_t* Str)
{
    Str[0] = dec/100;
    Str[1] = dec - (Str[0] * 100);
    Str[1] = Str[1]/10;
    Str[2] = dec%10;
    
    Str[0] += '0';
    Str[1] += '0';
    Str[2] += '0';
    
    if(Str[0] == '0'){
        Str[0] = ' ';
        if(Str[1] == '0')
            Str[1] = ' ';
    }
}