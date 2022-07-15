#ifndef __STDINT_H__
#define __STDINT_H__

#ifdef SDCC
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef char		int8_t;
typedef unsigned short  uint16_t;
typedef short		int16_t;
typedef unsigned long	uint32_t;
typedef long		int32_t;
#endif

#endif //__STDINT_H__
