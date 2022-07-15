#ifndef _VTX_H_
#define _VTX_H_

#include "hdzero.h"

/*
 * Standard intefaces provided by VTX targets
 */
extern const char* vtxProductName();
extern const uint8_t** vtxBandTables();
extern const uint8_t** vtxPowerTables();

#endif
