#ifndef _VTX_H_
#define _VTX_H_

/**
 * This file serves as the main interface for common
 * code and the detail specific implemenations are
 * not to be directly referenced as it will promote
 * a standard interface for VTX across the codebase.
 */

#ifdef VTX_WL
#include "vtx_wl.h"
#endif

#ifdef VTX_L
#include "vtx_l.h"
#endif

#ifdef VTX_S
#include "vtx_s.h"
#endif

#ifdef VTX_R
#include "vtx_r.h"
#endif

/*
 * Standard intefaces provided by VTX targets
 */
extern const char* vtxProductName();
extern const uint8_t** vtxBandTables();
extern const uint8_t** vtxPowerTables();

#endif
