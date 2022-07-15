#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "common.h"

#ifdef _DEBUG_MODE

#define Prompt()        Printf("\r\nDM568X>")
#define MAX_CMD_LEN     30

//void MonHelp(void);
uint8_t MonGetCommand(void);
void Monitor(void);
void MonWrite(uint8_t mode);
void MonRead(uint8_t mode);
void chg_vtx(void);
extern XDATA_SEG uint8_t   *argv[7];	
extern BIT_TYPE verbose;
#endif //_MONITOR_H_

#endif
