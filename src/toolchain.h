#ifndef __TOOLCHAIN__H_
#define __TOOLCHAIN__H_

#ifdef SDCC

#define IDATA_SEG __idata
#define XDATA_SEG __xdata

#else // Keil

#define IDATA_SEG idata
#define XDATA_SEG xdata

#endif

#endif