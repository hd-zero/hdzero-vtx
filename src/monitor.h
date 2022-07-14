#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "toolchain.h"

#define MAX_CMD_LEN     30
#define Prompt()        Printf("\r\nDM568X>")		

//void MonHelp(void);
uint8_t MonGetCommand(void);
void Monitor(void);
void MonWrite(uint8_t mode);
void MonRead(uint8_t mode);
void chg_vtx(void);
extern EEPROM_2(C51_XDAT, uint8_t, C51_XDAT) *argv[7];
extern BIT(verbose);
#endif //_MONITOR_H_
