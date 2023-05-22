#ifndef __TRAMP_PROTOCOL_H__
#define __TRAMP_PROTOCOL_H__

#include "common.h"
#include "uart.h"

#ifdef USE_TRAMP
#define tr_ready(void) RS_ready1(void)
#define tr_read(void)  RS_rx1(void)
#define tr_tx(c)       RS_tx1(c)

typedef enum {
    S_WAIT_LEN = 0, // Waiting for a packet len
    S_WAIT_CODE,    // Waiting for a response code
    S_DATA,         // Waiting for rest of the packet.
} trampReceiveState_e;

void tramp_receive(void);
void tramp_init(void);
#endif
extern uint8_t tr_tx_busy;
extern uint8_t tramp_lock;
#endif