#ifndef __MONITOR_H_
#define __MONITOR_H_

#define MAX_CMD_LEN 30
#define Prompt() debugf("\r\nDM568X>")

// void MonHelp(void);
uint8_t MonGetCommand(void);
void Monitor(void);
void MonWrite(uint8_t mode);
void MonRead(uint8_t mode);
void chg_vtx(void);
extern XDATA_SEG uint8_t *argv[7];

#endif /* __MONITOR_H_ */
