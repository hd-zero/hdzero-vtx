#ifndef __UART_H_
#define __UART_H_

#include "common.h"
#include "isr.h"

#ifdef EXTEND_BUF
#define BUF_MAX 2048 // 30
#else
#define BUF_MAX 255
#endif
#ifdef EXTEND_BUF1
#define BUF1_MAX 2047 // 30
#else
#define BUF1_MAX 255 // 30
#endif

void RS_tx(uint8_t c);
uint8_t RS_rx(void);
uint8_t RS_ready(void);

#ifdef EXTEND_BUF
uint16_t RS_rx_len(void);
#else
uint8_t RS_rx_len(void);
#endif

/*
#ifdef EXTEND_BUF1
uint16_t RS_rx1_len(void);
#else
uint8_t RS_rx1_len(void);
#endif
*/

void uart_set_baudrate(uint8_t baudIndex);
void uart_init();

void RS_tx1(uint8_t c);
uint8_t RS_rx1(void);
uint8_t RS_ready1(void);

extern volatile BIT_TYPE RS_Xbusy;
extern XDATA_SEG uint8_t RS_buf[BUF_MAX];
#ifdef EXTEND_BUF
extern XDATA_SEG volatile uint16_t RS_in;
extern XDATA_SEG volatile uint16_t RS_out;
#else
extern XDATA_SEG volatile uint8_t RS_in;
extern XDATA_SEG volatile uint8_t RS_out;
#endif

extern volatile BIT_TYPE RS_Xbusy1;
extern XDATA_SEG uint8_t RS_buf1[BUF1_MAX];
#ifdef EXTEND_BUF1
extern XDATA_SEG volatile uint16_t RS_in1;
extern XDATA_SEG volatile uint16_t RS_out1;
#else
extern XDATA_SEG volatile uint8_t RS_in1;
extern XDATA_SEG volatile uint8_t RS_out1;
#endif

#ifdef USE_SMARTAUDIO_SW
#define SUART_BUF_MAX 32 // has to be power of 2

extern uint8_t suart_tx_en;
void suart_txint();
void suart_rxint();

uint8_t SUART_ready();
uint8_t SUART_rx();

void SUART_tx(uint8_t *tbuf, uint8_t len);
extern uint8_t SA_is_0;
extern uint8_t SA_config;

#elif defined USE_SMARTAUDIO_HW
uint8_t SUART_ready();
uint8_t SUART_rx();
void SUART_tx(uint8_t *tbuf, uint8_t len);
#endif

#endif /* __UART_H_ */
