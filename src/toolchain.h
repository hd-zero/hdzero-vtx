#ifndef __TOOLCHAIN__H_
#define __TOOLCHAIN__H_

#define XSTR(s) STR(s)
#define STR(s) #s

#ifdef SDCC

#include <8051.h>
#include <mcs51reg.h>
#include <mcs51/msc1210.h>

#define RO_TYPE(a,b) b
#define RO_TYPE_2(a,b,c) b
#define RO_TYPE_REV(a,b) a
#define SFR(a,b) __sfr __at(b) a
#define SBIT(a,b) __sbit __at(b) a
#define BIT(a) __bit a
#define INTERRUPT(x) __interrupt(x) __using(x)

#else

#define RO_TYPE(a,b) a b
#define RO_TYPE_2(a,b,c) a b c
#define RO_TYPE_REV(a,b) RO_TYPE(a,b)
#define SFR(a,b) sfr a = b
#define SBIT(a,b) sbit a = b
#define BIT(a) bit a
#define INTERRUPT(x) interrupt(x)

#endif

#endif

