#include "uart.h"
#include "common.h"
#include "global.h"
#include "hardware.h"
#include "print.h"
#include <stdint.h>

XDATA_SEG uint8_t RS_buf[BUF_MAX];
#ifdef EXTEND_BUF
XDATA_SEG volatile uint16_t RS_in = 0;
XDATA_SEG volatile uint16_t RS_out = 0;
volatile BIT_TYPE RS_Xbusy = 0;
#else
XDATA_SEG volatile uint8_t RS_in = 0;
XDATA_SEG volatile uint8_t RS_out = 0;
volatile BIT_TYPE RS_Xbusy = 0;
#endif

XDATA_SEG uint8_t RS_buf1[BUF1_MAX];
#ifdef EXTEND_BUF1
XDATA_SEG volatile uint16_t RS_in1 = 0;
XDATA_SEG volatile uint16_t RS_out1 = 0;
volatile BIT_TYPE RS_Xbusy1 = 0;
#else
XDATA_SEG volatile uint8_t RS_in1 = 0;
XDATA_SEG volatile uint8_t RS_out1 = 0;
volatile BIT_TYPE RS_Xbusy1 = 0;
#endif

void uart_set_baudrate(uint8_t baudIndex) {
#if defined _DEBUG_MODE || defined _RF_CALIB
    baudIndex = 0;
#endif
    switch (baudIndex) {
    case 0: // 115200
        CKCON = 0x1F;
        TH1 = 0xEC;
        break;
    case 1: // 230400
        CKCON = 0x1F;
        TH1 = 0xF6;
        break;
    }
#ifdef _DEBUG_MODE
    debugf("\r\nSet uart baudrate to %bx", baudIndex);
#endif
}

void uart_init() {
#ifdef USE_TRAMP
    // tramp protocol need 115200 bps.
    BAUDRATE = 0;
#else
    BAUDRATE = I2C_Read8(ADDR_EEPROM, EEP_ADDR_BAUDRATE);
#endif
    if (BAUDRATE > 1)
        BAUDRATE = 0;

    uart_set_baudrate(BAUDRATE);
}

uint8_t RS_ready(void) {
    if (RS_in == RS_out)
        return 0;
    else
        return 1;
}

uint8_t RS_rx(void) {
    uint8_t ret;

    ret = RS_buf[RS_out];
    RS_out++;
    if (RS_out >= BUF_MAX)
        RS_out = 0;

    return ret;
}

void RS_tx(uint8_t c) {
    timer_ms10x_lst = timer_ms10x;
    while (1) {
        if ((!RS_Xbusy) || (timer_ms10x - timer_ms10x_lst > 100)) {
            SBUF0 = c;
            RS_Xbusy = 1;
            break;
        }
    }
}

#ifdef EXTEND_BUF
uint16_t RS_rx_len(void)
#else
uint8_t RS_rx_len(void)
#endif
{
    if (RS_out < RS_in)
        return RS_out + BUF_MAX - RS_in;
    else
        return RS_out - RS_in;
}

uint8_t RS_ready1(void) {
    if (RS_in1 == RS_out1)
        return 0;
    else
        return 1;
}

uint8_t RS_rx1(void) {
    uint8_t ret;
    ret = RS_buf1[RS_out1];
    RS_out1++;
    if (RS_out1 >= BUF1_MAX)
        RS_out1 = 0;

    return ret;
}

void RS_tx1(uint8_t c) {
    timer_ms10x_lst = timer_ms10x;
    while (1) {
        if ((!RS_Xbusy1) || (timer_ms10x - timer_ms10x_lst > 100)) {
            SBUF1 = c;
            RS_Xbusy1 = 1;
            break;
        }
    }
}

/*
#ifdef EXTEND_BUF
uint16_t RS_rx1_len(void)
#else
uint8_t RS_rx1_len(void)
#endif
{
     if(RS_out1 < RS_in1)
         return RS_out1+BUF_MAX - RS_in1;
     else
         return RS_out1 - RS_in1;
}*/
