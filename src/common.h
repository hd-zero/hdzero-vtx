#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdint.h>
#include <string.h>

#include "sfr_def.h"
#include "sfr_ext.h"
#include "toolchain.h"

// #define HDZERO_WHOOP
// #define HDZERO_WHOOP_LITE
// #define HDZERO_RACE_V1
// #define HDZERO_RACE_V2
// #define HDZERO_FREESTYLE
// #define FOXEER_VTX
// #define HDZERO_RACE_V3
// define HDZERO_FREESTYLE_V2

/* define VTX ID start */
#if defined HDZERO_WHOOP
#define VTX_ID 0x54
#elif defined HDZERO_WHOOP_LITE
#define VTX_ID 0x55
#elif defined HDZERO_RACE_V1
#define VTX_ID 0x56
#elif defined HDZERO_RACE_V2
#define VTX_ID 0x57
#elif defined HDZERO_FREESTYLE
#define VTX_ID 0x58
#elif defined FOXEER_VTX
#define VTX_ID 0x59
#elif defined HDZERO_RACE_V3
#define VTX_ID 0x5a
#elif defined HDZERO_FREESTYLE_V2
#define VTX_ID 0x5b
#else
#define VTX_ID 0x00
#endif
/* define VTX ID end */

#if defined HDZERO_WHOOP
#define VTX_NAME "HDZ WHOOP"
#elif defined HDZERO_WHOOP_LITE
#define VTX_NAME "HDZ WHOOP LITE"
#elif defined HDZERO_RACE_V1
#define VTX_NAME "HDZ RACE V1"
#elif defined HDZERO_RACE_V2
#define VTX_NAME "HDZ RACE V2"
#elif defined HDZERO_FREESTYLE
#define VTX_NAME "HDZ FREESTYLE"
#elif defined FOXEER_VTX
#define VTX_NAME "FOX VTX"
#elif defined HDZERO_RACE_V3
#define VTX_NAME "HDZ RACE V3"
#elif defined HDZERO_FREESTYLE_V2
#define VTX_NAME "HDZ FREESTYLE V2"
#else
#define VTX_NAME "  "
#endif

// system
#define assert(c)
#define dbg_pt(a) DBG_PIN0 = a

#define EXTEND_BUF
// #define EXTEND_BUF1

// mode
// #define _RF_CALIB
// #define REV_UART
// #define VIDEO_PAT
// #define FIX_EEP

#ifndef _RF_CALIB
// #define _DEBUG_MODE
// #define _DEBUG_DM6300
// #define _DEBUG_TC3587
// #define _DEBUG_CAMERA
// #define _DEBUG_LIFETIME
// #define _DEBUG_SMARTAUDIO
// #define _DEBUG_DISPLAYPORT
// #define _DEBUG_RUNCAM
// #define _DEBUG_SPI
// #define _DEBUG_TRAMP
#endif

#define Raceband
#define USE_EFUSE

#ifndef _RF_CALIB
#define USE_MSP
#endif

#define INIT_VTX_TABLE
#define IS_RX 0

// time
#define TIMER0_1S    9588
#define TIMER0_1SD2  (TIMER0_1S >> 1)
#define TIMER0_1SD16 (TIMER0_1S >> 4)
#define I2C_BIT_DLY  40
#define MS_DLY       (237)
#define MS_DLY_SDCC  (2746)
#define PRESS_L      3
#define PRESS_LL     8
#define PWR_LMT_SEC  10

#if defined USE_SMARTAUDIO_SW || defined USE_SMARTAUDIO_HW || defined USE_TRAMP
#define WAIT_SA_LOCK   4
#define WAIT_SA_CONFIG 9
#else
#define WAIT_SA_LOCK   3
#define WAIT_SA_CONFIG 3
#endif

#define CFG_TO_SEC  10
#define CAM_DET_DLY 1000
#define DISP_TIME   3 // 3/8s

// gpio
#define SCL     P0_0
#define SDA     P0_1
#define CAM_SCL P0_0
#define CAM_SDA P0_1
#ifdef USE_PA_EN
#define PA_EN P0_2
#elif !defined USE_TC3587_LED
#define LED_1 P0_2
#endif
#if defined USE_SMARTAUDIO_SW
#define SUART_PORT P0_3
#elif defined USE_TC3587_RSTB
#define TC3587_RSTB P0_3
#endif
#define CAM_PWM P0_4
#define BTN_1   P0_5

#define SPI_CS P1_0
#define SPI_CK P1_1
#define SPI_DI P0_6
#define SPI_DO P0_7

// uart
#ifndef REV_UART
#define Mon_tx(ch)  RS_tx1(ch)
#define Mon_rx()    RS_rx1()
#define Mon_ready() RS_ready1()
#define _outchar(c) RS_tx1(c)
#define CMS_tx(ch)  RS_tx(ch)
#define CMS_rx()    RS_rx()
#define CMS_ready() RS_ready()
#else
#define Mon_tx(ch)  RS_tx(ch)
#define Mon_rx()    RS_rx()
#define Mon_ready() RS_ready()
#define _outchar(c) RS_tx(c)
#define CMS_tx(ch)  RS_tx1(ch)
#define CMS_rx()    RS_rx1()
#define CMS_ready() RS_ready1()
#endif

#define Rom_tx(ch)  RS_tx(ch)
#define Rom_rx()    RS_rx()
#define Rom_ready() RS_ready()

#define FREQ_R1 (uint16_t)5658
#define FREQ_R2 (uint16_t)5696
#define FREQ_R3 (uint16_t)5732
#define FREQ_R4 (uint16_t)5769
#define FREQ_R5 (uint16_t)5806
#define FREQ_R6 (uint16_t)5843
#define FREQ_R7 (uint16_t)5880
#define FREQ_R8 (uint16_t)5917
#define FREQ_F2 (uint16_t)5760
#define FREQ_F4 (uint16_t)5800
#define FREQ_L1 (uint16_t)5362
#define FREQ_L2 (uint16_t)5399
#define FREQ_L3 (uint16_t)5436
#define FREQ_L4 (uint16_t)5473
#define FREQ_L5 (uint16_t)5510
#define FREQ_L6 (uint16_t)5547
#define FREQ_L7 (uint16_t)5584
#define FREQ_L8 (uint16_t)5621

#define INVALID_CHANNEL 0xff

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#endif /* __COMMON_H_ */
