#ifndef __TOOLCHAIN__H_
#define __TOOLCHAIN__H_

#ifdef SDCC

#define XDATA_SEG __xdata

#else // Keil

#define XDATA_SEG xdata

#endif

#endif