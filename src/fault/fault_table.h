#ifndef __FAULT_TABLE_H_
#define __FAULT_TABLE_H_

/**
 *  This file is to be managed by the VTX codebase
 *  and referenced/processed by the VRX codebase.
 *
 *  FLT_TBL_ strings are NOT to be referenced within VTX codebase!
 */

typedef enum {
    FLT_TBL_CAMERA_NOT_DETECTED = 0,
    FLT_TBL_CAMERA_CFG_RESET,

    FLT_TBL_CAMERA_TOTAL,
} fault_table_camera_e;
extern const char *FLT_TBL_CAMERA[FLT_TBL_CAMERA_TOTAL];

typedef enum {
    FLT_TBL_EEPROM_NOT_INITIALIZED = 0,

    FLT_TBL_EEPROM_TOTAL,
} fault_table_eeprom_e;
extern const char *FLT_TBL_EEPROM[FLT_TBL_EEPROM_TOTAL];

typedef enum {
    FLT_TBL_DM6300_NOT_DETECTED = 0,

    FLT_TBL_DM6300_TOTAL,
} fault_table_dm6300_e;
extern const char *FLT_TBL_DM6300[FLT_TBL_DM6300_TOTAL];

typedef enum {
    FLT_TBL_UART_RX_DATA_TIMEOUT = 0,

    FLT_TBL_UART_TOTAL,
} fault_table_uart_e;
extern const char *FLT_TBL_UART[FLT_TBL_UART_TOTAL];

typedef enum {
    FLT_TBL_VTX_HEAT_PROTECTION_ACTIVE = 0,

    FLT_TBL_VTX_TOTAL,
} fault_table_vtx_e;
extern const char *FLT_TBL_VTX[FLT_TBL_VTX_TOTAL];

typedef enum {
    FLT_TBL_I2C_RUNCAM_WRITE_ERROR = 0,

    FLT_TBL_I2C_TOTAL,
} fault_table_i2c_e;
extern const char *FLT_TBL_I2C[FLT_TBL_I2C_TOTAL];

typedef enum {
    FLT_TBL_SPI_WRITE_ERROR = 0,

    FLT_TBL_SPI_TOTAL,
} fault_table_spi_e;
extern const char *FLT_TBL_SPI[FLT_TBL_SPI_TOTAL];

#endif /* __FAULT_TABLE_H_ */
