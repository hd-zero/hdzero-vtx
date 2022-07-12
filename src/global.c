#include "common.h"
#include "global.h"
#include "print.h"


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

/*uint32_t a2i(uint8_t *str)   // str in dec string, such as "345", "738" ... => 345,738
{
	uint32_t num=0;
	uint8_t i;

	for(i=0; ; i++, str++) {
		if( *str=='\0' || *str==' ' ) break;
		num = num*10 + *str - '0';
	}
	return num;
}*/

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

#ifdef _DEBUG_MODE
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

void WAIT(uint32_t ms)
{
    uint32_t i,j;
    for(i=0;i<ms;i++) {
        for(j=0;j<MS_DLY;j++) {
        }
    }
}

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

void memcopy(uint8_t* dst, uint8_t* src, uint32_t len)
{
    uint32_t i;
    for(i=0;i<len;i++)
        dst[i] = src[i];
}