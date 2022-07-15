#ifndef __TOOLCHAIN__H_
#define __TOOLCHAIN__H_

#define XSTR(s) STR(s)
#define STR(s) #s
#define CONCAT(a,b) a ## b

#ifdef SDCC

#include <8051.h>
#include <mcs51reg.h>
#include <mcs51/msc1210.h>

#define EEPROM(a,b) CONCAT(__,a) b
#define EEPROM_2(a,b,c) CONCAT(__,a) b
#define EEPROM_3(a,b) a CONCAT(__,b)
#define SFR(a,b) __sfr __at(b) a
#define SBIT(a,b) __sbit __at(b) a
#define BIT(a) __bit a
#define INTERRUPT(x) __interrupt x

#else

#define EEPROM(a,b) a b
#define EEPROM_2(a,b,c) a b c
#define EEPROM_3(a,b) EEPROM(a,b)
#define SFR(a,b) sfr a = b
#define SBIT(a,b) sbit a = b
#define BIT(a) bit a
#define INTERRUPT(x) interrupt(x)

#endif

#endif

