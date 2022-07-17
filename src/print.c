#include "print.h"
#include "common.h"
#include "uart.h"

#include <stdio.h>

extern BIT_TYPE verbose;

static XDATA_SEG char print_buf[128];

void debugf(const char *fmt, ...) {
#ifdef _DEBUG_MODE
    int len = 0;
    int i = 0;
    va_list ap;

    va_start(ap, fmt);
    len = vsprintf(print_buf, fmt, ap);
    va_end(ap);

    for (; i < len; i++) {
        _outchar(print_buf[i]);
    }
#endif
}

void verbosef(const char *fmt, ...) {
#ifdef _DEBUG_MODE
    int len = 0;
    int i = 0;
    va_list ap;

    if (!verbose) {
      return;
    }

    va_start(ap, fmt);
    len = vsprintf(print_buf, fmt, ap);
    va_end(ap);

    for (; i < len; i++) {
        _outchar(print_buf[i]);
    }
#endif
}