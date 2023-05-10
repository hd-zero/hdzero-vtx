#ifndef __TRAMP_PROTOCOL_H__
#define __TRAMP_PROTOCOL_H__

#include "common.h"
#include "uart.h"

#define tr_ready(void) RS_ready(void)
#define tr_read(void)  RS_rx(void)
#define tr_tx(c)       RS_tx(c)

typedef enum {
    S_WAIT_LEN = 0, // Waiting for a packet len
    S_WAIT_CODE,    // Waiting for a response code
    S_DATA,         // Waiting for rest of the packet.
} trampReceiveState_e;

void tramp_receive(void);

extern uint8_t tr_tx_busy;

#endif