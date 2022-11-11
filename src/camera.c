#include "camera.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "i2c_device.h"
#include "isr.h"
#include "msp_displayport.h"
#include "print.h"
#include "runcam.h"

uint8_t camera_type;
uint8_t camera_device;
uint8_t camera_mfr = CAMERA_MFR_UNKNOW;
uint8_t camera_profile_eep;
uint8_t camera_attribute[CAMERA_SETTING_NUM][4];
uint8_t camera_setting_reg_eep[CAMERA_PROFILE_NUM][CAMERA_SETTING_NUM];
uint8_t camera_setting_reg_set[CAMERA_SETTING_NUM];
uint8_t camera_setting_reg_menu[CAMERA_SETTING_NUM];
uint8_t camera_profile_menu;

uint8_t video_format = VDO_FMT_720P60;
uint8_t camRatio = 0;
uint8_t camMenuStatus = CAM_STATUS_IDLE;

void camMenuStringUpdate(uint8_t status);
void camMenuSetVdoRatioInit();

void camera_type_detect(void) {
    camera_type = CAMERA_TYPE_UNKNOW;

    runcam_type_detect();
    if (camera_type) {
        camera_mfr = CAMERA_MFR_RUNCAM;
#ifdef _DEBUG_CAMERA
        debugf("\r\ncamera mfr : RUNCAM");
        debugf("\r\ncamera type: %d", (uint16_t)camera_type);
#endif
        return;
    }
}

void camera_mode_detect() {
    uint8_t cycles = 4;
    uint8_t loss = 0;
    uint8_t detect_tries = 0;
    uint8_t status_reg = 0;

    // init tc3587 and detect fps
    WriteReg(0, 0x8F, 0x91);

    if (camera_type) {
        if (camera_type == CAMERA_TYPE_RUNCAM_NANO_90) {
            if (camera_setting_reg_set[14] == 0) {
                video_format = VDO_FMT_720X540_90;
                Init_TC3587(1);
                Set_720P90(0);
            } else if (camera_setting_reg_set[14] == 1) {
                video_format = VDO_FMT_720X540_60;
                Init_TC3587(1);
                Set_720P90(0);
            } else {
                Set_720P60(IS_RX);
                Init_TC3587(0);
                video_format = VDO_FMT_720P60;
            }
        } else {
            Set_720P60(IS_RX);
            Init_TC3587(0);
            video_format = VDO_FMT_720P60;
        }
        I2C_Write16(ADDR_TC3587, 0x0058, 0x00e0);
    } else {
        while (cycles) {
            if (video_format == VDO_FMT_720P50) {
                Init_TC3587(0);
                Set_720P50(IS_RX);
#ifdef _DEBUG_CAMERA
                debugf("\r\nCamDetect: Set 50fps.");
#endif
            } else if (video_format == VDO_FMT_720P60) {
                Init_TC3587(0);
                Set_720P60(IS_RX);
#ifdef _DEBUG_CAMERA
                debugf("\r\nCamDetect: Set 60fps.");
#endif
            }
            WAIT(100);

            for (detect_tries = 0; detect_tries < 5; detect_tries++) {
                status_reg = ReadReg(0, 0x02);
#ifdef _DEBUG_CAMERA
                debugf("\r\nCamDetect status_reg: %x", status_reg);
#endif
                if ((status_reg >> 4) != 0) {
                    loss = 1;
                }
                WAIT(5);
            }

            if (loss == 0)
                break;

            video_format = (video_format == VDO_FMT_720P60) ? VDO_FMT_720P50 : VDO_FMT_720P60;

            loss = 0;
            cycles--;
        }
    }

#ifdef _DEBUG_MODE
    debugf("\r\ncameraID: %x", (uint16_t)camera_type);
#endif
}

void camera_button_init() {
    WriteReg(0, 0x17, 0xC0);
    WriteReg(0, 0x16, 0x00);
    WriteReg(0, 0x15, 0x02);
    WriteReg(0, 0x14, 0x00);
}

void camera_reg_write_eep(uint16_t addr, uint8_t val) {
    I2C_Write8_Wait(10, ADDR_EEPROM, addr, val);
    debugf("\r\neep write(%02x,%d)", addr, (uint16_t)val);
}
uint8_t camera_reg_read_eep(uint16_t addr) {
    return I2C_Read8_Wait(10, ADDR_EEPROM, addr);
}

void camera_setting_profile_read(uint8_t profile) {
    uint8_t i, addr;
    for (i = 0; i < CAMERA_SETTING_NUM; i++) {
        addr = EEP_ADDR_CAM_SETTING + profile * CAMERA_SETTING_NUM + i;
        camera_setting_reg_eep[profile][i] = camera_reg_read_eep(addr);
    }
}
void camera_setting_profile_write(uint8_t profile) {
    uint8_t i, j;
    uint8_t addr;
    if (profile == 0xff) {
        for (i = 0; i < CAMERA_SETTING_NUM; i++) {
            for (j = 0; j < CAMERA_PROFILE_NUM; j++) {
                addr = EEP_ADDR_CAM_SETTING + j * CAMERA_SETTING_NUM + i;
                camera_reg_write_eep(addr, camera_setting_reg_eep[j][i]);
            }
        }
    } else {
        for (i = 0; i < CAMERA_SETTING_NUM; i++) {
            addr = EEP_ADDR_CAM_SETTING + profile * CAMERA_SETTING_NUM + i;
            camera_reg_write_eep(addr, camera_setting_reg_eep[profile][i]);
        }
    }
}
void camera_setting_profile_reset(uint8_t profile) {
    if (camera_mfr == CAMERA_MFR_RUNCAM)
        runcam_setting_profile_reset(camera_setting_reg_eep[profile]);
}
void camera_setting_profile_check(uint8_t profile) {
    uint8_t need_reset = 0;
    if (camera_mfr == CAMERA_MFR_RUNCAM)
        need_reset = runcam_setting_profile_check(camera_setting_reg_eep[profile]);

    if (need_reset) {
        camera_setting_profile_reset(profile);
        camera_setting_profile_write(profile);
#ifdef _DEBUG_CAMERA
        debugf("\r\ncamera setting need to be reset");
#endif
    }
}
void camera_profile_read(void) {
    camera_profile_eep = camera_reg_read_eep(EEP_ADDR_CAM_PROFILE);
}
void camera_profile_write(void) {
    camera_reg_write_eep(EEP_ADDR_CAM_PROFILE, camera_profile_eep);
}
void camera_profile_reset(void) {
    camera_profile_eep = 0;
}
void camera_profile_check(void) {
    if (camera_profile_eep >= CAMERA_PROFILE_NUM) {
        camera_profile_eep = 0;
        camera_profile_write();
    }
}

void camera_setting_read(void) {
    uint8_t camera_type_last;
    uint8_t i;

    if (camera_type == CAMERA_TYPE_UNKNOW)
        return;

    camera_type_last = camera_reg_read_eep(EEP_ADDR_CAM_TYPE);
    if (camera_type_last != camera_type) {
        camera_profile_reset();
        camera_profile_write();
        for (i = 0; i < CAMERA_PROFILE_NUM; i++) {
            camera_setting_profile_reset(i);
            camera_setting_profile_write(i);
        }
        camera_reg_write_eep(EEP_ADDR_CAM_TYPE, camera_type);
        i = camera_reg_read_eep(EEP_ADDR_CAM_TYPE);
#ifdef _DEBUG_CAMERA
        debugf("\r\ncamera changed(%d==>%d), reset camera setting", camera_type_last, i);
#endif
    } else {
        camera_profile_read();
        camera_profile_check();
        for (i = 0; i < CAMERA_PROFILE_NUM; i++) {
            camera_setting_profile_read(i);
            camera_setting_profile_check(i);
        }
    }

    camera_profile_menu = camera_profile_eep;
}

void camera_setting_reg_menu_update(void) {
    uint8_t i;
    for (i = 0; i < CAMERA_SETTING_NUM; i++)
        camera_setting_reg_menu[i] = camera_setting_reg_eep[camera_profile_menu][i];

#ifdef _DEBUG_CAMERA
    debugf("\r\ncamera profile:%d", camera_profile_menu);
#endif
}

void camera_setting_reg_eep_update(void) {
    uint8_t i;
    for (i = 0; i < CAMERA_SETTING_NUM; i++)
        camera_setting_reg_eep[camera_profile_menu][i] = camera_setting_reg_menu[i];
}

void camera_set(uint8_t *camera_setting_reg, uint8_t save) {
    if (camera_mfr == CAMERA_MFR_RUNCAM) {
        runcam_set(camera_setting_reg);
        if (save)
            runcam_save();
    }
}

void CameraInit(void) {
    camera_type_detect();
    camera_setting_read();
    camera_setting_reg_menu_update();
    camera_set(camera_setting_reg_menu, 1);
    camera_mode_detect();

    camera_button_init();
}

void Cam_Button_ENTER() { WriteReg(0, 0x14, 0x32); }
void Cam_Button_RIGHT() { WriteReg(0, 0x14, 0x58); }
void Cam_Button_DOWN() { WriteReg(0, 0x14, 0x64); }
void Cam_Button_LEFT() { WriteReg(0, 0x14, 0x3F); }
void Cam_Button_UP() { WriteReg(0, 0x14, 0x4B); }
void Cam_Button_MID() { WriteReg(0, 0x14, 0x00); }

void camera_button_op(uint8_t op) {
    switch (op) {
    case BTN_UP:
        Cam_Button_UP();
        break;
    case BTN_DOWN:
        Cam_Button_DOWN();
        break;
    case BTN_LEFT:
        Cam_Button_LEFT();
        break;
    case BTN_RIGHT:
        Cam_Button_RIGHT();
        break;
    case BTN_ENTER:
        Cam_Button_ENTER();
        break;
    case BTN_MID:
        Cam_Button_MID();
        break;
    default:
        break;
    }
}

void camMenuDrawBracket(void) {
    uint8_t i;
    for (i = CAM_STATUS_PROFILE; i <= CAM_STATUS_VDO_FMT; i++) {
        if (i == CAM_STATUS_PROFILE) {
            osd_buf[i][osd_menu_offset + 19] = '<';
            osd_buf[i][osd_menu_offset + 29] = '>';
        } else if (camera_attribute[i - CAM_STATUS_PROFILE][item_enable]) {
            osd_buf[i][osd_menu_offset + 19] = '<';
            osd_buf[i][osd_menu_offset + 29] = '>';
        }
    }
}

void camMenuDrawValue(void) {
    const char *wb_mode_str[] = {"  AUTO", "MANUAL"};
    const char *switch_str[] = {"OFF", " ON"};
    const char *resolution_runcam_micro_v2[] = {"      4:3", "16:9 CLIP", "16:9 FULL"};
    const char *resolution_runcam_nano_90[] = {"540P@90", "540P@60", "720P@60"};

    uint8_t str[4];
    uint8_t i;
    uint8_t dat;

    for (i = CAM_STATUS_PROFILE; i <= CAM_STATUS_VDO_FMT; i++)
        switch (i) {
        case CAM_STATUS_PROFILE: // profile
            uint8ToString(camera_profile_menu + 1, str);
            strcpy(&osd_buf[i][osd_menu_offset + 21], str);
            break;
        case CAM_STATUS_BRIGHTNESS: // brightness
            if (camera_setting_reg_menu[0] > camera_attribute[0][item_default]) {
                dat = camera_setting_reg_menu[0] - camera_attribute[0][item_default];
                osd_buf[i][osd_menu_offset + 21] = '+';

            } else if (camera_setting_reg_menu[0] < camera_attribute[0][item_default]) {
                dat = camera_attribute[0][item_default] - camera_setting_reg_menu[0];
                osd_buf[i][osd_menu_offset + 21] = '-';
            } else {
                osd_buf[i][osd_menu_offset + 21] = ' ';
                dat = 0;
            }
            uint8ToString(dat, str);
            strcpy(&osd_buf[i][osd_menu_offset + 22], str);
            break;
        case CAM_STATUS_SHARPNESS:  // sharpness
        case CAM_STATUS_CONTRAST:   // contrast
        case CAM_STATUS_SATURATION: // saturation
            if (camera_attribute[i - 2][item_enable]) {
                uint8ToString(camera_setting_reg_menu[i - 2] + 1, str);
                strcpy(&osd_buf[i][osd_menu_offset + 21], str);
            }
            break;
        case CAM_STATUS_WBRED:  // wb red
        case CAM_STATUS_WBBLUE: // wb blue
            if (camera_attribute[i - 2][item_enable]) {
                uint8ToString(camera_setting_reg_menu[i - 2], str);
                strcpy(&osd_buf[i][osd_menu_offset + 21], str);
            }
            break;
        case CAM_STATUS_WBMODE: // wb mode
            if (camera_attribute[i - 2][item_enable]) {
                strcpy(&osd_buf[i][osd_menu_offset + 21], wb_mode_str[camera_setting_reg_menu[i - 2]]);
            }
            break;
        case CAM_STATUS_HVFLIP:     // hv flip
        case CAM_STATUS_NIGHT_MODE: // night mode
        case CAM_STATUS_LED_MODE:   // led mode
            if (camera_attribute[i - 2][item_enable]) {
                strcpy(&osd_buf[i][osd_menu_offset + 21], switch_str[camera_setting_reg_menu[i - 2]]);
            }
            break;
        case CAM_STATUS_VDO_FMT: // vdo fmt
            if (camera_attribute[i - 2][item_enable]) {
                if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V2) {
                    strcpy(&osd_buf[i][osd_menu_offset + 21], resolution_runcam_micro_v2[camera_setting_reg_menu[i - 2]]);
                } else if (camera_type == CAMERA_TYPE_RUNCAM_NANO_90) {
                    strcpy(&osd_buf[i][osd_menu_offset + 21], resolution_runcam_nano_90[camera_setting_reg_menu[i - 2]]);
                }
            }
            break;
        default:
            break;
        }
}

void camMenuInit(void) {
    memset(osd_buf, 0x20, sizeof(osd_buf));
    disp_mode = DISPLAY_CMS;
    if (camera_type == 0)
        Cam_Button_ENTER();
    else {
        strcpy(osd_buf[0] + osd_menu_offset + 1, "----CAMERA MENU----");
        strcpy(osd_buf[1] + osd_menu_offset + 3, "PROFILE");
        strcpy(osd_buf[2] + osd_menu_offset + 3, "BRIGHTNESS");
        strcpy(osd_buf[3] + osd_menu_offset + 3, "SHARPNESS");
        strcpy(osd_buf[4] + osd_menu_offset + 3, "CONTRAST");
        strcpy(osd_buf[5] + osd_menu_offset + 3, "SATURATION");
        strcpy(osd_buf[6] + osd_menu_offset + 3, "WB MODE");
        strcpy(osd_buf[7] + osd_menu_offset + 3, "WB RED");
        strcpy(osd_buf[8] + osd_menu_offset + 3, "WB BLUE");
        strcpy(osd_buf[9] + osd_menu_offset + 3, "HV FLIP");
        strcpy(osd_buf[10] + osd_menu_offset + 3, "MAX GAIN");
        strcpy(osd_buf[11] + osd_menu_offset + 3, "LED MODE");
        strcpy(osd_buf[12] + osd_menu_offset + 3, "ASPECT RATIO");
        strcpy(osd_buf[13] + osd_menu_offset + 3, "RESET");
        strcpy(osd_buf[14] + osd_menu_offset + 3, "EXIT");
        strcpy(osd_buf[15] + osd_menu_offset + 3, "SAVE&EXIT");
        camMenuDrawBracket();
        camMenuDrawValue();
#if (0)
        if (camera_type == RUNCAM_MICRO_V1)
            camMenuStringUpdate(CAM_STATUS_BRIGHTNESS);
        else
            camMenuStringUpdate(CAM_STATUS_PROFILE);
#endif
    }
}
void camera_menu_cursor_update(uint8_t erase) {
    if (erase)
        osd_buf[camMenuStatus][1] = ' ';
    else
        osd_buf[camMenuStatus][1] = '>';
}
void camera_menu_item_toggle(uint8_t op) {
    camera_menu_cursor_update(1);
    if (op == BTN_DOWN) {
        camMenuStatus++;
        if (camMenuStatus > CAM_STATUS_SAVE_EXIT)
            camMenuStatus = CAM_STATUS_PROFILE;
    } else if (op == BTN_UP) {
        camMenuStatus--;
        if (camMenuStatus < CAM_STATUS_PROFILE)
            camMenuStatus = CAM_STATUS_SAVE_EXIT;
    }
    camera_menu_cursor_update(0);
}

void camera_profile_menu_toggle(uint8_t op) {
    if (op == BTN_RIGHT) {
        camera_profile_menu++;
        if (camera_profile_menu > 2)
            camera_profile_menu = 0;
        camera_setting_reg_menu_update();
        camera_set(camera_setting_reg_menu, 0);
    } else if (op == BTN_LEFT) {
        camera_profile_menu--;
        if (camera_profile_menu > 2)
            camera_profile_menu = 2;
        camera_setting_reg_menu_update();
        camera_set(camera_setting_reg_menu, 0);
    }
}

uint8_t camera_menu_long_press(uint8_t op, uint8_t last_op, uint8_t is_init) {
    static uint8_t cnt = 0;
    static uint8_t step = 1;
    if (is_init || (op == BTN_MID)) {
        cnt = 0;
        step = 1;
    } else if ((last_op == BTN_RIGHT) || (last_op == BTN_LEFT)) {
        cnt++;
        cnt &= 0x7f;
        step = cnt >> 3;
        if (!step)
            step = 1;
    }

    return step;
}

void camera_setting_reg_menu_toggle(uint8_t op, uint8_t last_op) {
    uint8_t item = camMenuStatus - 2;

    switch (camMenuStatus) {
    case CAM_STATUS_BRIGHTNESS:
    case CAM_STATUS_WBRED:
    case CAM_STATUS_WBBLUE:
        if (!camera_attribute[item][item_enable])
            return;
#if (0)
        // if wb mode if auto, do not modify wbred/wbblue
        if ((camMenuStatus == CAM_STATUS_WBRED || camMenuStatus == CAM_STATUS_WBBLUE) &&
            (!camera_setting_reg_menu[4]))
            return;
#endif

        if (op == BTN_RIGHT) {
            camera_setting_reg_menu[item] += camera_menu_long_press(op, last_op, 0);
            if (camera_setting_reg_menu[item] > camera_attribute[item][item_max])
                camera_setting_reg_menu[item] = camera_attribute[item][item_min];

            camera_set(camera_setting_reg_menu, 0);
        } else if (op == BTN_LEFT) {
            camera_setting_reg_menu[item] -= camera_menu_long_press(op, last_op, 0);
            if (camera_setting_reg_menu[item] < camera_attribute[item][item_min])
                camera_setting_reg_menu[item] = camera_attribute[item][item_max];

            camera_set(camera_setting_reg_menu, 0);
        } else if (op == BTN_MID) {
            camera_menu_long_press(op, last_op, 1);
        }
        break;
    case CAM_STATUS_SHARPNESS:
    case CAM_STATUS_CONTRAST:
    case CAM_STATUS_SATURATION:
    case CAM_STATUS_WBMODE:
    case CAM_STATUS_HVFLIP:
    case CAM_STATUS_NIGHT_MODE:
    case CAM_STATUS_LED_MODE:
    case CAM_STATUS_VDO_FMT:
        if (!camera_attribute[item][item_enable])
            return;

        if (op == BTN_RIGHT) {
            camera_setting_reg_menu[item]++;
            if (camera_setting_reg_menu[item] > camera_attribute[item][item_max])
                camera_setting_reg_menu[item] = camera_attribute[item][item_min];
            camera_set(camera_setting_reg_menu, 0);
        } else if (op == BTN_LEFT) {
            camera_setting_reg_menu[item]--;
            if (camera_attribute[item][item_min] == 0) {
                if (camera_setting_reg_menu[item] > camera_attribute[item][item_max])
                    camera_setting_reg_menu[item] = camera_attribute[item][item_max];
            } else if (camera_setting_reg_menu[item] < camera_attribute[item][item_min])
                camera_setting_reg_menu[item] = camera_attribute[item][item_max];
            camera_set(camera_setting_reg_menu, 0);
        }
        break;

    default:
        break;
    }
}

#ifdef USE_MSP
uint8_t camStatusUpdate(uint8_t op) {

    uint8_t ret = 0;
    static uint8_t step = 1;
    // static uint8_t cnt;
    static uint8_t last_op = BTN_RIGHT;

    if (op >= BTN_INVALID)
        return ret;

    if (camera_type == 0) {
        camera_button_op(op);
        return ret;
    }

    switch (camMenuStatus) {
    case CAM_STATUS_IDLE:
        if (op == BTN_MID) {
            camera_profile_menu = camera_profile_eep;
            camera_setting_reg_menu_update();
            // camera_menu_item_string_update();
            camera_menu_long_press(op, last_op, 1);
            camMenuStatus = CAM_STATUS_PROFILE;
            camera_menu_cursor_update(0);
        }
        break;

    case CAM_STATUS_PROFILE:
    case CAM_STATUS_BRIGHTNESS:
    case CAM_STATUS_SHARPNESS:
    case CAM_STATUS_CONTRAST:
    case CAM_STATUS_SATURATION:
    case CAM_STATUS_WBMODE:
    case CAM_STATUS_WBRED:
    case CAM_STATUS_WBBLUE:
    case CAM_STATUS_HVFLIP:
    case CAM_STATUS_NIGHT_MODE:
    case CAM_STATUS_LED_MODE:
    case CAM_STATUS_VDO_FMT:
        if (camMenuStatus == CAM_STATUS_BRIGHTNESS ||
            camMenuStatus == CAM_STATUS_WBRED ||
            camMenuStatus == CAM_STATUS_WBBLUE)
            ; // note brightness/wbred/wbblue support long press
        else if (last_op == op)
            break;

        camera_menu_item_toggle(op);

        if (camMenuStatus == CAM_STATUS_PROFILE)
            camera_profile_menu_toggle(op);
        else
            camera_setting_reg_menu_toggle(last_op, op);
        camMenuDrawValue();
        break;
    case CAM_STATUS_RESET:
        if (last_op == op)
            break;
        camera_menu_item_toggle(op);
        if (op == BTN_RIGHT) {
            camera_setting_profile_reset(camera_profile_menu);
            camera_setting_reg_menu_update();
            camera_set(camera_setting_reg_menu, 0);
            camMenuDrawValue();
        }
        break;

    case CAM_STATUS_EXIT:
        if (last_op == op)
            break;
        camera_menu_item_toggle(op);
        if (op == BTN_RIGHT) {
            camera_setting_reg_menu_update();
            camera_set(camera_setting_reg_menu, 0);
            camMenuStatus = CAM_STATUS_IDLE;
            ret = 1;
        }
        break;
    case CAM_STATUS_SAVE_EXIT:
        if (last_op == op)
            break;
        camera_menu_item_toggle(op);
        if (op == BTN_RIGHT) {
            camera_profile_eep = camera_profile_menu;
            camera_profile_write();
            camera_set(camera_setting_reg_menu, 1);
            camera_setting_reg_eep_update();
            camera_setting_profile_write(0xff);
            camMenuStatus = CAM_STATUS_IDLE;
            ret = 1;
        }
        break;
    case CAM_STATUS_REPOWER:
        break;

    default:
        break;
    } // switch(camMenuStatus)

    last_op = op;
    // camMenuStringUpdate(camMenuStatus);

    return ret;
}
#endif

const char *cam_names[] = {"   MICRO V2", "    NANO V2", "  NANO LITE"};
const char *low_med_high_strs[] = {"   LOW", "MEDIUM", "  HIGH"};
const char *off_on_strs[] = {"  OFF", "   ON"};