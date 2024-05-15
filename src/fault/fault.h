#ifndef __FAULT_H_
#define __FAULT_H_

#include "fault_table.h"
#include "stdint.h"

/**
 *  Constants & Types
 */
#define MAX_FAULTS 16 // Max reportable faults.
typedef uint8_t fault_id_t;

/**
 * Fault Message:
 *
 *  0xFF 0xEE
 *  ---- ----
 *  |    DATA
 *  |
 *  1111 111 1
 *     |   | |
 *     |   | fault_dt_e
 *     |   fault_et_e
 *     fault_id_t
 */
typedef struct {
    uint8_t mask;
    uint8_t data;
} fault_msg_t;

/**
 *  Data Type published, flt_msg_t bit 1
 */
typedef enum {
    FLT_DT_TABLE = 0,
    FLT_DT_VALUE,
} fault_dt_e;

/**
 *  Event Type published, flt_msg_t bits 2-4
 */
typedef enum {
    FLT_ET_NONE = 0,
    FLT_ET_CAMERA,
    FLT_ET_EEPROM,
    FLT_ET_DM6300,
    FLT_ET_UART,
    FLT_ET_VTX,
    FLT_ET_I2C,
    FLT_ET_SPI,
} fault_et_e;

/**
 *  Serialize to byte array
 */
void enc_fault_msg(const fault_msg_t *msg, uint8_t *buffer);

/**
 *  Deserialize to structure
 */
void dec_fault_msg(const uint8_t *buffer, fault_msg_t *msg);

/**
 *  Extract fault attributes
 */
void extract_fault_msg(const fault_msg_t *msg, fault_id_t *id, fault_et_e *et, fault_dt_e *dt, uint8_t *data);

/**
 *  Inserts a fault record into database.
 */
void add_fault(fault_et_e et, fault_dt_e dt, uint8_t data);

/**
 *  Returns a fault record from database.
 */
fault_msg_t *get_next_fault();

#endif /* __FAULT_H_ */
