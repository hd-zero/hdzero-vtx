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
#define SFR(a,b) __sfr __at(b) a
#define SBIT(a,b) __sbit __at(b) a
#define BIT __bit 
#define INTERRUPT __interrupt 

#else

#define XDATA_SEG xdata
#define IDATA_SEG idata
#define PDATA_SEG pdata
#define CODE_SEG code
#define SFR(a,b) sfr a = b
#define SBIT(a,b) sbit a = b
#define BIT bit 
#define INTERRUPT interrupt

#endif

#endif

