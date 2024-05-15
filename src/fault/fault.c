#include "fault.h"

/**
 *  Global storage of faults entries.
 */
static fault_msg_t g_fault_msgs[MAX_FAULTS];

/**
 *  Keep track of used fault msgs.
 */
static uint8_t g_fault_head = 0;
static uint8_t g_fault_tail = 0;

void enc_fault_msg(const fault_msg_t *msg, uint8_t *buffer) {
    buffer[0] = msg->mask;
    buffer[1] = msg->data;
}

void dec_fault_msg(const uint8_t *buffer, fault_msg_t *msg) {
    msg->mask = buffer[0];
    msg->data = buffer[1];
}

void extract_fault_msg(const fault_msg_t *msg, fault_id_t *id, fault_et_e *et, fault_dt_e *dt, uint8_t *data) {
    *id = (msg->mask & 0xF0) >> 4;
    *et = (msg->mask & 0x0E) >> 1;
    *dt = (msg->mask & 0x01);
    *data = msg->data;
}

void add_fault(fault_et_e et, fault_dt_e dt, uint8_t data) {
    // Once the buffer is full we drop any new reports
    if (g_fault_head < MAX_FAULTS) {
        g_fault_msgs[g_fault_head].mask =
            ((g_fault_head << 4) & 0xF0) |
            ((et << 1) & 0x0E) |
            (dt & 0x01);

        g_fault_msgs[g_fault_head].data = data;
        g_fault_head++;
    }
}

fault_msg_t *get_next_fault() {
    fault_msg_t *tmp = &g_fault_msgs[g_fault_tail++];

    // Wrap back to beginning
    if (g_fault_tail >= g_fault_head) {
        g_fault_tail = 0;
    }

    return tmp;
}
