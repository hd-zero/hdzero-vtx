#ifndef __UART_H__
#define __UART_H__


//#define DEBUG_SA

#ifdef EXTEND_BUF
#define  BUF_MAX        2048 //30
#else
#define  BUF_MAX        255
#endif
#ifdef EXTEND_BUF1
#define  BUF1_MAX       2047    //30
#else
#define  BUF1_MAX	    255     //30
#endif
#include "common.h"

#define RS_tx(c)  while(1) { if( !RS_Xbusy  ) { SBUF0 = c; RS_Xbusy = 1;  break; } }
#define RS_tx1(c) while(1) { if( !RS_Xbusy1 ) { SBUF1 = c; RS_Xbusy1 = 1; break; } }

uint8_t RS_ready(void);

uint8_t RS_rx(void);

#ifdef EXTEND_BUF
uint16_t RS_rx_len(void);
#else
uint8_t RS_rx_len(void);
#endif

uint8_t RS_ready1(void);

/*
#ifdef EXTEND_BUF1
uint16_t RS_rx1_len(void);
#else
uint8_t RS_rx1_len(void);
#endif
*/

uint8_t RS_rx1(void);

extern bit RS_Xbusy;
extern XDATA_SEG uint8_t RS_buf[BUF_MAX];
#ifdef EXTEND_BUF
extern XDATA_SEG uint16_t RS_in, RS_out;
#else
extern XDATA_SEG uint8_t RS_in, RS_out;
#endif

extern bit RS_Xbusy1;
extern XDATA_SEG uint8_t RS_buf1[BUF1_MAX];
#ifdef EXTEND_BUF1
extern XDATA_SEG uint16_t RS_in1, RS_out1;
#else
extern XDATA_SEG uint8_t RS_in1, RS_out1;
#endif

#ifdef USE_SMARTAUDIO

#define SUART_BUF_MAX 32  //has to be power of 2

extern uint8_t suart_tx_en;
void suart_txint();
void suart_rxint();

uint8_t SUART_ready();
uint8_t SUART_rx();

void SUART_tx(uint8_t* tbuf,uint8_t len);
extern uint8_t SA_is_0;
extern uint8_t SA_config;
#endif //USE_SMARTAUDIO

#endif