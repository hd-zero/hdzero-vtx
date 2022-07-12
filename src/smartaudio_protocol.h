#ifndef _SMARTAUDIO_PROTOCOL_H_
#define _SMARTAUDIO_PROTOCOL_H_

#include "common.h"

#ifdef USE_SMARTAUDIO

#define FREQ_R1  (uint16_t)5658
#define FREQ_R2  (uint16_t)5696
#define FREQ_R3  (uint16_t)5732
#define FREQ_R4  (uint16_t)5769
#define FREQ_R5  (uint16_t)5806
#define FREQ_R6  (uint16_t)5843
#define FREQ_R7  (uint16_t)5880
#define FREQ_R8  (uint16_t)5917
#define FREQ_F2  (uint16_t)5760
#define FREQ_F4  (uint16_t)5800

//#define DBG_SMARTAUDIO

#define SA_HEADER0_BYTE 0xAA
#define SA_HEADER1_BYTE 0x55
#define SA_VERSION_BYTE 0x09
#define SA_GET_SETTINGS 0x01
#define SA_SET_PWR 0x02
#define SA_SET_CH 0x03
#define SA_SET_FREQ 0x04
#define SA_SET_MODE 0x05

typedef enum
{
    SA_HEADER0,
    SA_HEADER1,
    SA_CMD,
    SA_LENGTH,
    SA_PAYLOAD,
    SA_CRC

} SA_status_e;

uint8_t SA_task();
uint8_t SA_Process();
void    SA_Init();

extern uint8_t SA_dbm;
extern uint8_t crc8tab[256];
uint8_t pwr_to_dbm(uint8_t pwr);
#endif //USE_SMARTAUDIO
#endif //_SMARTAUDIO_PROTOCOL_H_