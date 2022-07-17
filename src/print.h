#ifndef __PRINT_H__
#define __PRINT_H__

#include <stdarg.h>

void _debugf(const char *fmt, ...);
void _verbosef(const char *fmt, ...);

#ifdef _DEBUG_MODE
#define debugf _debugf
#define verbosef _verbosef
#else
#define debugf 
#define verbosef
#endif

#endif