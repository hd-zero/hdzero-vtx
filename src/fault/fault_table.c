/**
 *  This file is to be managed by the VTX codebase
 *  and referenced/processed by the VRX codebase.
 *
 *  FLT_TBL_ strings are NOT to be referenced within VTX codebase!
 */
#include "fault_table.h"

const char *FLT_TBL_CAMERA[FLT_TBL_CAMERA_TOTAL] = {
    "CAMERA not detected:\nReplace camera or MIPI cable.\nCheck MIPI socket for obstructions.\nCheck DM5680 chip pins are clean.",
    "CAMERA configuration reset is required",
};

const char *FLT_TBL_EEPROM[FLT_TBL_EEPROM_TOTAL] = {
    "EEPROM is not initialized, using default RF Power Table.",
};

const char *FLT_TBL_DM6300[FLT_TBL_DM6300_TOTAL] = {
    "DM6300 not detected: Chip could be damaged or not responding back to DM5680.",
};

const char *FLT_TBL_UART[FLT_TBL_UART_TOTAL] = {
    "UART Receive line not detecting data stream.",
};

const char *FLT_TBL_VTX[FLT_TBL_VTX_TOTAL] = {
    "VTX overheat protection activated.",
};

const char *FLT_TBL_I2C[FLT_TBL_I2C_TOTAL] = {
    "I2C RUNCAM write error.",
};

const char *FLT_TBL_SPI[FLT_TBL_SPI_TOTAL] = {
    "SPI write error.",
};
