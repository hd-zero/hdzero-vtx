#ifndef __TOOLCHAIN__H_
#define __TOOLCHAIN__H_

#ifdef SDCC

#define IDATA_SEG __idata
#define XDATA_SEG __xdata

#define BIT_TYPE __bit

#define SFR_DEF(name, loc) __sfr __at(loc) name
#define SBIT_DEF(name, loc) __sbit __at(loc) name

#else // Keil

#define IDATA_SEG idata
#define XDATA_SEG xdata

#define BIT_TYPE bit

#define SFR_DEF(name, loc) sfr name = loc
#define SBIT_DEF(name, loc) sbit name = loc

#endif

#endif