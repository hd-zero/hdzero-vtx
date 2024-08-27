#include "uart.h"
#include "common.h"
#include "global.h"
#include "hardware.h"
#include "print.h"
#include "smartaudio_protocol.h"
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

#ifdef USE_SMARTAUDIO_HW
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

#else
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
#endif
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

////////////////////////////////////////////////////////////////////////////
// SUART TX
#ifdef USE_SMARTAUDIO_SW
XDATA_SEG uint8_t SUART_rbuf[SUART_BUF_MAX];
XDATA_SEG uint8_t SUART_rin = 0, SUART_rout = 0, SUART_rERR = 0;

void suart_rxint() // ISR
{
    static uint8_t isSearchingStart = 1;
    static uint8_t si_d = 1;
    static uint8_t bcnt = 0;
    static uint8_t rxbyte = 0;
    static uint8_t cnt = 0;
    uint8_t si;

    si = SUART_PORT;

    if (si)
        SA_is_0 = 0;

    if (isSearchingStart) {
        if (si_d && !si && !SA_is_0) {
            isSearchingStart = 0;
            bcnt = 0;
            rxbyte = 0;
            cnt = 0x0;
        }
        si_d = si;
        return;
    } else {
        cnt++;
        if (cnt >= 2) {
            cnt = 0;
            if (bcnt < 8) {
                rxbyte >>= 1;
                if (si)
                    rxbyte |= 0x80;
                bcnt++;
            } else {
                isSearchingStart = 1;
                si_d = 1;

                SUART_rbuf[SUART_rin++] = rxbyte;
                SUART_rin &= (SUART_BUF_MAX - 1);
            }
        }
    }
}

uint8_t SUART_ready() {
    if (SUART_rin == SUART_rout)
        return 0;
    else
        return 1;
}

uint8_t SUART_rx() {
    uint8_t ret;

    ret = SUART_rbuf[SUART_rout];
    SUART_rout++;
    SUART_rout &= (SUART_BUF_MAX - 1);

    return ret;
}

////////////////////////////////////////////////////////////////////////////
// SUART TX
uint8_t suart_tx_en = 0;
uint8_t suart_txbcnt = 0;
uint16_t suart_txdat;

void suart_txint() // ISR
{
    static uint8_t cnt = 0;

    cnt++;
    if (cnt >= 2) {
        cnt = 0;
        SUART_PORT = suart_txdat & 1;
        suart_txdat >>= 1;
        suart_txbcnt++;
        if (suart_txbcnt >= 11) {
            suart_tx_en = 0;
            SUART_PORT = 1;
        }
    }
}

void SUART_tx_byte(uint8_t dat) {
    suart_txdat = (dat << 1) | 0xFE00;
    suart_txbcnt = 0;
    suart_tx_en = 1;
}

void SUART_tx(uint8_t *tbuf, uint8_t len) {
    uint8_t i;
    for (i = 0; i < len; i++) {
        SUART_tx_byte(tbuf[i]);
        while (suart_tx_en)
            ;
    }
}
#elif defined USE_SMARTAUDIO_HW
uint8_t SUART_ready() {
    return RS_ready1();
}
uint8_t SUART_rx() {
    return RS_rx1();
}
void SUART_tx(uint8_t *tbuf, uint8_t len) {
    uint8_t i;
    sa_status = SA_ST_TX;
    for (i = 0; i < len; i++) {
        RS_tx1(tbuf[i]);
        WAIT(2); // extern 1 stop bits
    }
    sa_status = SA_ST_IDLE;
    uart_set_baudrate(BAUDRATE);
}
#endif