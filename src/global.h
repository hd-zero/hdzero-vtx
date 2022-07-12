#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define HI_BYTE(a) 	((a>>8)&0xFF)
#define LO_BYTE(a) 	(a&0xFF)

#define ASSERT(a) 
#define abs(a) ( ((a)<0)?(-(a)):(a))

//uint8_t toupper(uint8_t ch);
uint8_t Asc2Bin(uint8_t *s);
uint16_t Asc4Bin(uint8_t *s);
uint32_t Asc8Bin(uint8_t *s);
uint8_t Asc1Bin(uint8_t asc);
//uint32_t a2i(uint8_t *str);

int stricmp(uint8_t *ptr1, uint8_t *ptr2);

void Bin2Asc(int16_t i, uint8_t *string);
void uint8ToString(uint8_t dec, uint8_t* Str);
//void Bin1Asc(int16_t i, uint8_t *string);

//void Bin4Asc(int32_t i, uint8_t *string); 

void WAIT(uint32_t ms);

void memcopy(uint8_t* dst, uint8_t* src, uint32_t len);

#endif //_GLOBAL_H_
