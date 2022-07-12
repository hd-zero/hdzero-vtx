#ifndef __HARDWARE_H_
#define __HARDWARE_H_

#include "stdint.h"
#include "common.h"
#include "i2c.h"
#include "i2c_device.h"

typedef enum{
    ON,
    OFF,
}ledType_e;
typedef enum{
    BW_27M,
    BW_20M
}BWType_e;
#define PWR_DEFAULT     2
#define SPARKLE_T       20

// vtx parameter
#define EEP_ADDR_RF_FREQ         0x80
#define EEP_ADDR_RF_POWER        0x81
#define EEP_ADDR_LPMODE          0x82
#define EEP_ADDR_PITMODE         0x83
#define EEP_ADDR_25MW            0x84
#define EEP_ADDR_SA_LOCK         0x88
#define EEP_ADDR_POWER_LOCK      0x89
#define EEP_ADDR_VTX_CONFIG      0x8a

#define EEP_ADDR_LIFETIME_0      0xF0
#define EEP_ADDR_LIFETIME_1      0xF1
#define EEP_ADDR_LIFETIME_2      0xF2
#define EEP_ADDR_LIFETIME_3      0xF3
// camera parameter
#define EEP_ADDR_CAM_PROFILE     0x3f  //
                                       // [3:0] used for runcam v1
                                       // [7:4] used for runcam v2
 
//                              Micro V1    Micro V2  Nano V2     Nano Lite
#define EEP_ADDR_CAM_BRIGHTNESS  0x40//      0x50      0x60        0x70
#define EEP_ADDR_CAM_SHARPNESS   0x41//      0x51      0x61        0x71
#define EEP_ADDR_CAM_SATURATION  0x42//      0x52      0x62        0x72
#define EEP_ADDR_CAM_CONTRAST    0x43//      0x53      0x63        0x73
#define EEP_ADDR_CAM_HVFLIP      0x44//      0x54      0x64        0x74
#define EEP_ADDR_CAM_NIGHTMODE   0x45//      0x55      0x65        0x75
//#define EEP_ADDR_CAM_RATIO       0x46//      0x56      0x66        0x76
#define EEP_ADDR_CAM_WBMODE      0x47//      0x57      0x67        0x77
#define EEP_ADDR_CAM_WBRED       0x48//      0x58      0x68        0x78
//#define EEP_ADDR_CAM_WBRED       0x49//      0x59      0x69        0x79
//#define EEP_ADDR_CAM_WBRED       0x4a//      0x5a      0x6a        0x7a
//#define EEP_ADDR_CAM_WBRED       0x4b//      0x5b      0x6b        0x7b
#define EEP_ADDR_CAM_WBBLUE      0x4c//      0x5c      0x6c        0x7c
//#define EEP_ADDR_CAM_WBBLUE      0x4d//      0x5d      0x6d        0x7d
//#define EEP_ADDR_CAM_WBBLUE      0x4e//      0x5e      0x6e        0x7e
//#define EEP_ADDR_CAM_WBBLUE      0x4f//      0x5f      0x6f        0x7f    

#define EEP_ADDR_DCOC_EN        0xC0
#define EEP_ADDR_DCOC_IH        0xC1
#define EEP_ADDR_DCOC_IL        0xC2
#define EEP_ADDR_DCOC_QH        0xC3
#define EEP_ADDR_DCOC_QL        0xC4

#define VTX_B_ID    0x4C
#define VTX_M_ID    0x50
#define VTX_S_ID    0x54
#define VTX_R_ID    0x58
#define VTX_WL_ID   0x59
#define VTX_L_ID    0x5C

#define FREQ_MAX        7
#define FREQ_MAX_EXT    9
#if defined VTX_L
    #define POWER_MAX   3
#elif defined VTX_M
    #define POWER_MAX   2
#else
    #define POWER_MAX   1
#endif

#ifdef VTX_L
    #define LED_BLUE_ON     I2C_Write(ADDR_TC3587, 0x0014, 0x0000, 1, 1)
    #define LED_BLUE_OFF    I2C_Write(ADDR_TC3587, 0x0014, 0x8000, 1, 1)
#else
    #define LED_BLUE_ON     LED_1 = ON
    #define LED_BLUE_OFF    LED_1 = OFF
#endif

void Init_HW();
void OnButton1();
void Video_Detect();
void PwrLMT();
void TempDetect();

void Init_6300RF(uint8_t freq, uint8_t pwr);

void GetVtxParameter();
void Setting_Save();
void Imp_RF_Param();
void CFG_Back();

void Set_720P50(uint8_t page);
void Set_720P60(uint8_t page);
void Set_720P30(uint8_t page, uint8_t is_43);

void Flicker_LED(uint8_t n);
#ifdef VTX_L
extern uint8_t powerLock;
#endif
extern uint8_t RF_FREQ;
extern uint8_t RF_POWER;
extern uint8_t LP_MODE;
extern uint8_t PIT_MODE;
extern uint8_t OFFSET_25MW;
extern uint8_t KEYBOARD_ON;
extern uint8_t EE_VALID;
extern uint8_t RF_BW;

extern uint8_t pwr_offset;
extern uint8_t heat_protect;

extern uint8_t g_IS_ARMED;
extern uint8_t g_IS_ARMED_last;
extern uint8_t g_IS_PARALYZE;
extern uint8_t fc_lock;
extern uint8_t vtx_pit;
extern uint8_t vtx_pit_save;
extern uint8_t SA_lock;
extern uint8_t pwr_lmt_sec;
extern uint8_t pwr_lmt_done;

extern uint8_t cur_pwr;

extern uint8_t last_SA_lock;
extern uint8_t pwr_init;
extern uint8_t ch_init;

extern uint8_t led_status;

extern uint8_t dispF_cnt;
extern uint8_t temp_err;
#endif