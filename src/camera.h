#ifndef __CAMERA_H_
#define __CAMERA_H_

#include "common.h"
#include "sfr_ext.h"
#include <stdint.h>

#define CAM_PROFILE_NUM 4

#define RUNCAM_MICRO_V1 0x42
#define RUNCAM_MICRO_V2 0x44
#define RUNCAM_NANO_90  0x46
#define RUNCAM_MICRO_V3 0x48

#define CAMERA_SETTING_NUM 16
#define CAMERA_PROFILE_NUM 3

extern uint8_t camera_is_3v3;

#define camera_button_enter WriteReg(0, 0x14, camera_is_3v3 ? 50 : 65)
#define camera_button_right WriteReg(0, 0x14, camera_is_3v3 ? 88 : 89)
#define camera_button_down  WriteReg(0, 0x14, camera_is_3v3 ? 100 : 100)
#define camera_button_left  WriteReg(0, 0x14, camera_is_3v3 ? 63 : 72)
#define camera_button_up    WriteReg(0, 0x14, camera_is_3v3 ? 75 : 80)
#define camera_button_mid   WriteReg(0, 0x14, camera_is_3v3 ? 0 : 44)

typedef enum {
    CAMERA_MFR_UNKNOW,
    CAMERA_MFR_FOXEER,
    CAMERA_MFR_RUNCAM,
} camera_manufacture_e;

typedef enum {
    CAMERA_TYPE_UNKNOW,
    CAMERA_TYPE_RESERVED,        // include foxeer digisight v3
    CAMERA_TYPE_OUTDATED,        // include runcam(orange)
    CAMERA_TYPE_RUNCAM_MICRO_V1, // include hdz nano v1
    CAMERA_TYPE_RUNCAM_MICRO_V2, // include hzd nano v2 / hdz nano lite
    CAMERA_TYPE_RUNCAM_NANO_90,
    CAMERA_TYPE_RUNCAM_MICRO_V3,
} camera_type_e;

typedef enum {
    VDO_FMT_720P50,
    VDO_FMT_720P60,
    VDO_FMT_720P60_NEW,
    VDO_FMT_720P30,
    VDO_FMT_540P90,
    VDO_FMT_540P90_CROP,
    VDO_FMT_540P60,
    VDO_FMT_960x720P60,
    VDO_FMT_1080P30,
} video_format_e;

typedef enum {
    CAM_SETTING_ITEM_ENBALE,
    CAM_SETTING_ITEM_MIN,
    CAM_SETTING_ITEM_MAX,
    CAM_SETTING_ITEM_DEFAULT,
} camera_setting_attribute_e;

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
    CAM_STATUS_SAVING,

    CAM_STATUS_END,
} camera_status_e;

typedef enum {
    CAM_SELECT_RUNCAM_ECO = 0,
    CAM_SELECT_RUNCAM_LUX,

    CAM_SELECT_RATIO,
    CAM_SELECT_EXIT,
} camera_select_e;

void camera_init();
uint8_t camera_status_update(uint8_t op);
void camera_menu_init(void);
void camera_select_menu_init(void);
void camera_select_menu_cursor_update(uint8_t index);
void camera_select_menu_ratio_upate();
void camera_menu_mode_exit_note();

extern uint8_t camRatio;
extern uint8_t video_format;
extern uint8_t camera_type;
extern uint8_t camera_device;
extern uint8_t camera_attribute[CAMERA_SETTING_NUM][4];
extern uint8_t camera_setting_reg_set[CAMERA_SETTING_NUM];
extern uint8_t camera_setting_reg_menu[CAMERA_SETTING_NUM];
#endif /* __CAMERA_H_ */
