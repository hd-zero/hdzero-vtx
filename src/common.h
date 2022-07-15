#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <string.h>
#include "sfr_def.h"
#include "sfr_ext.h"
#include "toolchain.h"

#define VERSION 0x41
#define BETA 0x03

// target
//#define VTX_L
//#define VTX_WL
#define VTX_S
//#define VTX_R

// system
#define KEILC
#define assert(c)
#define dbg_pt(a)   DBG_PIN0 = a

#define EXTEND_BUF
//#define EXTEND_BUF1

// mode
//#define _RF_CALIB
//#define REV_UART
//#define VIDEO_PAT

//#define FIX_EEP

#ifndef _RF_CALIB
//#define _DEBUG_MODE
//#define _DEBUG_DM6300
//#define _DEBUG_TC3587
//#define _DEBUG_MSP_SET_VTX_CONFIG
//#define _DEBUG_LIFETIME
#endif

#define Raceband
#define USE_EFUSE
#ifndef VTX_S
#define USE_SMARTAUDIO
#endif
#define INIT_VTX_TABLE

#define IS_RX 0

// time
#define TIMER0_1S       9588
#define TIMER0_1SD2     (TIMER0_1S>>1)
#define TIMER0_1SD16    (TIMER0_1S>>4)
#define I2C_BIT_DLY     40
#define MS_DLY          237
#define PRESS_L         3
#define PRESS_LL        8
#define PWR_LMT_SEC     10
#define WAIT_SA_CONFIG  9
#define CFG_TO_SEC      10
#define CAM_DET_DLY     1000
#define DISPF_TIME      3   // 3/8s

// gpio
#define SCL             P0_0
#define SDA             P0_1
#define CAM_SCL         P0_0
#define CAM_SDA         P0_1
#ifdef VTX_L
#define PA_EN           P0_2
#else
#define LED_1           P0_2
#endif
#ifdef USE_SMARTAUDIO
#define SUART_PORT      P0_3
#else
#define TC3587_RSTB     P0_3
#endif
#define CAM_PWM         P0_4
#define BTN_1           P0_5

#define SPI_CS          P1_0
#define SPI_CK          P1_1
#define SPI_DI          P0_6
#define SPI_DO          P0_7

// uart
#ifndef REV_UART
#define Mon_tx(ch)      RS_tx1(ch)
#define Mon_rx()        RS_rx1()
#define Mon_ready()     RS_ready1()
#define _outchar(c)     RS_tx1(c)
#define CMS_tx(ch)      RS_tx(ch)
#define CMS_rx()        RS_rx()
#define CMS_ready()     RS_ready()
#else
#define Mon_tx(ch)      RS_tx(ch)
#define Mon_rx()        RS_rx()
#define Mon_ready()     RS_ready()
#define _outchar(c)     RS_tx(c)
#define CMS_tx(ch)      RS_tx1(ch)
#define CMS_rx()        RS_rx1()
#define CMS_ready()     RS_ready1()
#endif

#define Rom_tx(ch)      RS_tx(ch)
#define Rom_rx()        RS_rx()
#define Rom_ready()     RS_ready()

#endif //_COMMON_H_
