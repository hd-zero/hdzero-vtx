#ifndef __CAMERA_H_
#define __CAMERA_H_
#include <stdint.h>
#include "common.h"

#ifdef _DEBUG_MODE
//#define _DEBUG_CAMERA
#endif

#define RUNCAM_PROFILE_M_TYPE    4

#define RUNCAM_MICRO_V1          0x42
#define RUNCAM_MICRO_V2          0x44

#define CAM_BRIGHTNESS_INITIAL      0x80
#define CAM_SHARPNESS_INITIAL       1
#define CAM_SATURATION_INITIAL      3
#define CAM_CONTRAST_INITIAL        1
#define CAM_HVFLIP_INITIAL          0
#define CAM_NIGHTMODE_INITIAL       1
#define CAM_WBRED_INITIAL           0xC7    // 0x314>>2
#define CAM_WBBLUE_INITIAL          0xCA    // 0x328>>2
#define CAM_WBMODE_INITIAL          0

#define PROFILE_MAX_V1              0x01
#define PROFILE_MAX_V2              0x05
#define BRIGHTNESS_MIN              0x40
#define BRIGHTNESS_MAX              0xC0
#define SHARPNESS_MIN               0x00
#define SHARPNESS_MAX               0x02
#define SATURATION_MIN              0x00
#define SATURATION_MAX              0x06
#define CONTRAST_MIN                0x00
#define CONTRAST_MAX                0x02
#define HVFLIP_MAX                  0x01
#define NIGHTMODE_MAX               0x01
#define WBMODE_MIN                  0x00
#define WBMODE_MAX                  0x04

typedef enum{
    CAM_720P50,
    CAM_720P60,
    CAM_720P60_NEW,
    CAM_720P30,
}camFPSType_e;

typedef enum{
    VDO_720P60_16_9,
    VDO_720P60_4_3,
    VDO_720P30_16_9,
    VDO_720P30_4_3,
    VDO_TYPE_MAX,
}VdoFormatType_e;

typedef enum{
    Profile_MicroV2_Auto,
    Profile_NanoV2_Auto,
    Profile_NanoLite_Auto,
    Profile_MicroV2_Manual,
    Profile_NanoV2_Manual,
    Profile_NanoLite_Manual,
}RuncamV2Profile_e;

typedef struct{
    uint8_t brightness;
    uint8_t sharpness;
    uint8_t saturation;
    uint8_t contrast;
    uint8_t hvFlip;
    uint8_t nightMode;
    uint8_t wbMode;
    uint8_t wbRed[WBMODE_MAX];
    uint8_t wbBlue[WBMODE_MAX];
}cameraConfig_t;

typedef enum{
    CAM_STATUS_IDLE = 0,
    CAM_STATUS_PROFILE,
    CAM_STATUS_BRIGHTNESS,
    CAM_STATUS_SHARPNESS,
    CAM_STATUS_CONTRAST,
    CAM_STATUS_SATURATION,
    CAM_STATUS_WBMODE,
    CAM_STATUS_WBRED,
    CAM_STATUS_WBBLUE,
    CAM_STATUS_HVFLIP,
    CAM_STATUS_NIGHTMODE,
    CAM_STATUS_VDO_RATIO,
    CAM_STATUS_RESET,
    CAM_STATUS_EXIT,
    CAM_STATUS_SAVE_EXIT,
    CAM_STATUS_SET_VDO_RATIO_4_3,
    CAM_STATUS_SET_VDO_RATIO_16_9_0,
    CAM_STATUS_SET_VDO_RATIO_16_9_1,
    CAM_STATUS_SET_VDO_RATIO_RETURN,
    CAM_STATUS_END,
    CAM_STATUS_REPOWER,
}camStatusType_e;

void CameraInit();
uint8_t camStatusUpdate(uint8_t op);
void camMenuInit();

extern uint8_t cam_4_3;
extern uint8_t CAM_MODE;
extern uint8_t cameraID;
#endif
