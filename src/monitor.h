#ifndef _MONITOR_H_
#define _MONITOR_H_

#define MAX_CMD_LEN     30
#define Prompt()        Printf("\r\nDM568X>")		

//void MonHelp(void);
uint8_t MonGetCommand(void);
void Monitor(void);
void MonWrite(uint8_t mode);
void MonRead(uint8_t mode);
void chg_vtx(void);
extern XDATA_SEG uint8_t   *argv[7];	
extern bit verbose;
#endif //_MONITOR_H_
