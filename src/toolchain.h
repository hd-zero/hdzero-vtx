#ifndef __TOOLCHAIN__H_
#define __TOOLCHAIN__H_

#ifdef SDCC

#include <8051.h>
#include <mcs51reg.h>
#include <mcs51/msc1210.h>

#define XDATA_SEG __xdata
#define IDATA_SEG __idata
#define PDATA_SEG __pdata
#define CODE_SEG __code
#define BIT_TYPE __bit 
#define INTERRUPT __interrupt 

#define SFR_DEF(name, loc) __sfr __at(loc) name
#define SBIT_DEF(name, loc) __sbit __at(loc) name

#else // Keil

#define XDATA_SEG xdata
#define IDATA_SEG idata
#define PDATA_SEG pdata
#define CODE_SEG code
#define BIT_TYPE bit 
#define INTERRUPT interrupt

#define SFR(name, loc) sfr name = loc
#define SBIT(name, loc) sbit name = loc

#endif

#endif

