#ifndef __PRINT_H_
#define __PRINT_H_

#include <stdarg.h>

void _debugf(const char *fmt, ...);
void _verbosef(const char *fmt, ...);

#ifdef _DEBUG_MODE
#define debugf _debugf
#define verbosef _verbosef
#else

#ifdef SDCC
#define debugf
#define verbosef
#else
// workaround keils warning C275: expression with possibly no effect
static void _nopf(const char *fmt, ...) { fmt = fmt; }
#define debugf _nopf
#define verbosef _nopf
#endif

#endif

#endif /* __PRINT_H_ */
