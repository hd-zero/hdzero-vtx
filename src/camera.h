#ifndef __CAMERA_H_
#define __CAMERA_H_

#include <stdint.h>

#include "common.h"

#define CAM_PROFILE_NUM 4

#define RUNCAM_MICRO_V1 0x42
#define RUNCAM_MICRO_V2 0x44
#define RUNCAM_NANO_90  0x46

#define CAMERA_SETTING_NUM 16
#define CAMERA_PROFILE_NUM 3
typedef enum {
    CAMERA_MFR_UNKNOW,
    CAMERA_MFR_FOXEER,
    CAMERA_MFR_RUNCAM,
} camera_manufacturer_e;

typedef enum {
    CAMERA_TYPE_UNKNOW,
    CAMERA_TYPE_RUNCAM_MICRO_V1,
    CAMERA_TYPE_RUNCAM_MICRO_V2, // include nano V2 / nano lite
    CAMERA_TYPE_RUNCAM_NANO_90,
} camera_type_e;

typedef enum {
    VDO_FMT_720P50,
    VDO_FMT_720P60,
    VDO_FMT_720P60_NEW,
    VDO_FMT_720P30,
    VDO_FMT_720X540_90,
    VDO_FMT_720X540_60,
    VDO_FMT_960x720_60
} video_format_e;

typedef enum {
    VDO_720P60_16_9,
    VDO_720P60_4_3,
    VDO_720P30_16_9,
    VDO_720P30_4_3,
    VDO_TYPE_MAX,
} VdoFormatType_e;

typedef enum {
    // Profile_MicroV1,
    Profile_MicroV2,
    Profile_NanoV2,
    Profile_NanoLite,
    Profile_Max,
} RuncamV2Profile_e;

typedef enum {
    item_enable,
    item_min,
    item_max,
    item_default,
} camera_setting_attribute;

typedef enum {
    CAM_STATUS_IDLE = 0xff,
    CAM_STATUS_PROFILE = 0x00,
    CAM_STATUS_BRIGHTNESS,
    CAM_STATUS_SHARPNESS,
    CAM_STATUS_CONTRAST,
    CAM_STATUS_SATURATION,
    CAM_STATUS_SHUTTER,
    CAM_STATUS_WBMODE,
    CAM_STATUS_WBRED,
    CAM_STATUS_WBBLUE,
    CAM_STATUS_HVFLIP,
    CAM_STATUS_NIGHT_MODE,
    CAM_STATUS_LED_MODE,
    CAM_STATUS_VDO_FMT,
    CAM_STATUS_RESET,
    CAM_STATUS_EXIT,
    CAM_STATUS_SAVE_EXIT,
    CAM_STATUS_REPOWER,

    CAM_STATUS_END,
} camStatusType_e;

void CameraInit();
uint8_t camStatusUpdate(uint8_t op);
void camMenuInit(void);

extern uint8_t camRatio;
extern uint8_t video_format;
extern uint8_t camera_type;
extern uint8_t camera_device;
extern uint8_t camera_attribute[CAMERA_SETTING_NUM][4];
extern uint8_t camera_setting_reg_set[CAMERA_SETTING_NUM];
#endif /* __CAMERA_H_ */
