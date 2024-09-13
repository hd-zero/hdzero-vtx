#include "camera.h"
#include "common.h"
#include "dm6300.h"
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
uint8_t camRatio = 0; // 0->16:9   1->4:3
uint8_t camMenuStatus = CAM_STATUS_IDLE;
uint8_t reset_isp_need = 0;

uint8_t camera_is_3v3 = 0;

void camera_type_detect(void) {
    camera_type = CAMERA_TYPE_UNKNOW;

    runcam_type_detect();
    if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V1 ||
        camera_type == CAMERA_TYPE_RUNCAM_MICRO_V2 ||
        camera_type == CAMERA_TYPE_RUNCAM_NANO_90 ||
        camera_type == CAMERA_TYPE_RUNCAM_MICRO_V3) {
        camera_mfr = CAMERA_MFR_RUNCAM;
        return;
    }
}

void camera_ratio_detect(void) {
    switch (camera_type) {
    case CAMERA_TYPE_RUNCAM_MICRO_V1:
        camRatio = 0;
        break;
    case CAMERA_TYPE_RUNCAM_MICRO_V2:
    case CAMERA_TYPE_RUNCAM_MICRO_V3:
        camRatio = (camera_setting_reg_set[11] == 0);
        break;
    case CAMERA_TYPE_RUNCAM_NANO_90:
        camRatio = 1;
        break;
#ifdef USE_TP9950
    case CAMERA_TYPE_OUTDATED:
        camRatio = I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_CAM_RATIO);
        if (camRatio > 1)
            camRatio = 1;
        break;
#endif
    default:
        camRatio = 0;
        break;
    }
}

#ifdef USE_TP9950
void camera_mode_detect(uint8_t init) {

    uint32_t frame = 0;
    uint32_t agc_en = 0;
    uint8_t id = 0;

    init = 0;

#ifdef USE_TC3587_RSTB
    TC3587_RSTB = 0;
    WAIT(100);
    TC3587_RSTB = 1;
    WAIT(100);
#endif

    Set_720P60_8bit(0);
    id = I2C_Read8(ADDR_TP9950, 0xfe);
    id = I2C_Read8(ADDR_TP9950, 0xff);
    WAIT(200);

    Set_720P60_8bit(0);
    id = I2C_Read8(ADDR_TP9950, 0xfe);
    id = I2C_Read8(ADDR_TP9950, 0xff);
    WAIT(200);

    I2C_Write8(ADDR_TP9950, 0x26, 0x01);
    I2C_Write8(ADDR_TP9950, 0x07, 0xC0);
    I2C_Write8(ADDR_TP9950, 0x0B, 0xC0);
    I2C_Write8(ADDR_TP9950, 0x22, 0x35);
    agc_en = I2C_Read8(ADDR_TP9950, 0x06);
    agc_en &= 0xFB;
    I2C_Write8(ADDR_TP9950, 0x06, agc_en);

    I2C_Write8(ADDR_TP9950, 0x02, 0xca);
    I2C_Write8(ADDR_TP9950, 0x0b, 0xc0);
    I2C_Write8(ADDR_TP9950, 0x0c, 0x03);
    I2C_Write8(ADDR_TP9950, 0x0d, 0x50);
    I2C_Write8(ADDR_TP9950, 0x15, 0x13);
    I2C_Write8(ADDR_TP9950, 0x16, 0x16);
    I2C_Write8(ADDR_TP9950, 0x17, 0x00);
    I2C_Write8(ADDR_TP9950, 0x18, 0x19);
    I2C_Write8(ADDR_TP9950, 0x19, 0xD0);
    I2C_Write8(ADDR_TP9950, 0x1a, 0x25);
    I2C_Write8(ADDR_TP9950, 0x20, 0x30);
    I2C_Write8(ADDR_TP9950, 0x21, 0x84);
    I2C_Write8(ADDR_TP9950, 0x22, 0x36);
    I2C_Write8(ADDR_TP9950, 0x23, 0x3c);
    I2C_Write8(ADDR_TP9950, 0x26, 0x05);
    I2C_Write8(ADDR_TP9950, 0x2b, 0x60);
    I2C_Write8(ADDR_TP9950, 0x2c, 0x0a);
    I2C_Write8(ADDR_TP9950, 0x2d, 0x30);
    I2C_Write8(ADDR_TP9950, 0x2e, 0x70);
    I2C_Write8(ADDR_TP9950, 0x30, 0x48);
    I2C_Write8(ADDR_TP9950, 0x31, 0xbb);
    I2C_Write8(ADDR_TP9950, 0x32, 0x2e);
    I2C_Write8(ADDR_TP9950, 0x33, 0x90);
    I2C_Write8(ADDR_TP9950, 0x39, 0x1c);
    I2C_Write8(ADDR_TP9950, 0x3B, 0x26);
    I2C_Write8(ADDR_TP9950, 0x18, 0x19);
    I2C_Write8(ADDR_TP9950, 0x40, 0x08);
    I2C_Write8(ADDR_TP9950, 0x13, 0x04);
    I2C_Write8(ADDR_TP9950, 0x14, 0x04);
    I2C_Write8(ADDR_TP9950, 0x40, 0x00);
    I2C_Write8(ADDR_TP9950, 0x35, 0x05);
    I2C_Write8(ADDR_TP9950, 0xfa, 0x08);
    I2C_Write8(ADDR_TP9950, 0x4C, 0x40);
    I2C_Write8(ADDR_TP9950, 0x4e, 0x05);

    I2C_Write8(ADDR_TP9950, 0x1c, 0x06);
    I2C_Write8(ADDR_TP9950, 0x1d, 0x72);
    // **** soft reset **** //
    I2C_Write8(ADDR_TP9950, 0x06, 0xb2);

    video_format = VDO_FMT_720P60;
    camera_type = CAMERA_TYPE_OUTDATED;
    camera_ratio_detect();
    LED_BLUE_ON;
    led_status = ON;

    RF_BW = BW_27M;
    RF_BW_last = RF_BW;

#ifdef _RF_CALIB
    WAIT(1000);
    if (I2C_Read8(ADDR_TP9950, 0x01) != 0x7E) { // if camera lost
        WriteReg(0, 0x50, 0x01);                // set to video pattern
    }
    RF_POWER = 0;
    RF_FREQ = 0;
    Init_6300RF(RF_FREQ, RF_POWER);
    DM6300_AUXADC_Calib();
#endif
}
#else
void camera_mode_detect(uint8_t init) {
    uint8_t cycles = 4;
    uint8_t loss = 0;
    uint8_t detect_tries = 0;
    uint8_t status_reg = 0;

    // init tc3587 and detect fps
    WriteReg(0, 0x8F, 0x91);

    if (camera_type == CAMERA_TYPE_RUNCAM_NANO_90) {
        Init_TC3587(1);
        if (camera_setting_reg_set[11] == 0) {
            Set_540P90(0);
            video_format = VDO_FMT_540P90;
        } else if (camera_setting_reg_set[11] == 1) {
            Set_540P90_crop(0);
            video_format = VDO_FMT_540P90_CROP;
        } else if (camera_setting_reg_set[11] == 2) {
            Set_540P60(0);
            video_format = VDO_FMT_540P60;
        } else if (camera_setting_reg_set[11] == 3) {
            Set_960x720P60(0);
            video_format = VDO_FMT_960x720P60;
        }
        I2C_Write16(ADDR_TC3587, 0x0058, 0x00e0);
    } else if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V1) {
        Set_720P60(IS_RX);
        Init_TC3587(0);
        video_format = VDO_FMT_720P60;
        I2C_Write16(ADDR_TC3587, 0x0058, 0x00e0);
    } else if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V2 || camera_type == CAMERA_TYPE_RUNCAM_MICRO_V3) {
        if (camera_setting_reg_set[11] == 3) {
            Set_1080P30(IS_RX);
            video_format = VDO_FMT_1080P30;
        } else {
            Set_720P60(IS_RX);
            video_format = VDO_FMT_720P60;
        }
        Init_TC3587(0);
        I2C_Write16(ADDR_TC3587, 0x0058, 0x00e0);
    } else {
        while (cycles) {
            if (video_format == VDO_FMT_720P50) {
                Init_TC3587(0);
                Set_720P50(IS_RX);
            } else if (video_format == VDO_FMT_720P60) {
                Init_TC3587(0);
                Set_720P60(IS_RX);
            }
            WAIT(100);

            for (detect_tries = 0; detect_tries < 5; detect_tries++) {
                status_reg = ReadReg(0, 0x02);
                if ((status_reg >> 4) != 0) {
                    loss = 1;
                }
                WAIT(5);
            }

            if (loss == 0) {
                camera_type = CAMERA_TYPE_OUTDATED;
                break;
            }

            video_format = (video_format == VDO_FMT_720P60) ? VDO_FMT_720P50 : VDO_FMT_720P60;

            loss = 0;
            cycles--;
        }

        if (cycles == 0) {
            Set_720P60(0);
            video_format = VDO_FMT_720P60;
            I2C_Write16(ADDR_TC3587, 0x0058, 0x00e0);
            camera_type = CAMERA_TYPE_RESERVED;
        }
    }
    camera_ratio_detect();

    if (init) {
        if (camera_type == CAMERA_TYPE_RUNCAM_NANO_90 && camera_setting_reg_set[11] == 2)
            RF_BW = BW_17M;
        else
            RF_BW = BW_27M;
        RF_BW_last = RF_BW;
    }
}
#endif

void camera_button_init() {
    WriteReg(0, 0x17, 0xC0);
    WriteReg(0, 0x16, 0x00);
    WriteReg(0, 0x15, 0x02);
    WriteReg(0, 0x14, 0x00);
}

void camera_reg_write_eep(uint16_t addr, uint8_t val) {
    I2C_Write8_Wait(10, ADDR_EEPROM, addr, val);
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

    if (camera_type == CAMERA_TYPE_UNKNOW ||
        camera_type == CAMERA_TYPE_OUTDATED ||
        camera_type == CAMERA_TYPE_RESERVED)
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
}

void camera_setting_reg_eep_update(void) {
    uint8_t i;
    for (i = 0; i < CAMERA_SETTING_NUM; i++)
        camera_setting_reg_eep[camera_profile_menu][i] = camera_setting_reg_menu[i];
}

uint8_t camera_set(uint8_t *camera_setting_reg, uint8_t save, uint8_t init) {
    uint8_t ret = 0;
    if (camera_mfr == CAMERA_MFR_RUNCAM) {
        ret = runcam_set(camera_setting_reg);
        if (save || (init & ret))
            runcam_save();
    }

    return ret;
}

void camera_init(void) {
    camera_type_detect();
    camera_setting_read();
    camera_setting_reg_menu_update();
    reset_isp_need = camera_set(camera_setting_reg_menu, 0, 1);

    if (reset_isp_need) {
        if (camera_mfr == CAMERA_MFR_RUNCAM) {
            runcam_reset_isp();
        }
    }

    camera_mode_detect(1);

    camera_button_init();
}

void camera_button_op(uint8_t op) {
    switch (op) {
    case BTN_UP:
        camera_button_up;
        break;
    case BTN_DOWN:
        camera_button_down;
        break;
    case BTN_LEFT:
        camera_button_left;
        break;
    case BTN_RIGHT:
        camera_button_right;
        break;
    case BTN_ENTER:
        camera_button_enter;
        break;
    case BTN_MID:
        camera_button_mid;
        break;
    default:
        break;
    }
}

void camera_menu_draw_bracket(void) {
    uint8_t i;
    for (i = CAM_STATUS_BRIGHTNESS; i <= CAM_STATUS_VDO_FMT; i++) {
        osd_buf[i][osd_menu_offset + 18] = '<';
        osd_buf[i][osd_menu_offset + 29] = '>';
    }
}

void camera_menu_draw_value(void) {
    const char *wb_mode_str[] = {"   AUTO", " MANUAL"};
    const char *switch_str[] = {"    OFF", "     ON"};
    const char *hv_flip_str[] = {"    OFF", "     ON", " V ONLY", " H ONLY"};
    const char *resolution_runcam_micro_v2[] = {"      4:3 ", " 16:9CROP ", " 16:9FULL ", "  1080@30 "};
    const char *resolution_runcam_nano_90[] = {"   540P@90", "540@90CROP", "   540P@60", "960X720@60"};
    const char *resolution_runcam_micro_v3[] = {"      4:3 ", " 16:9CROP ", " 16:9FULL ", "  1080@30 "};

    uint8_t str[4];
    uint8_t i;
    uint8_t dat;

    // profile
    osd_buf[0][osd_menu_offset + 28] = '1' + camera_profile_menu;

    for (i = CAM_STATUS_BRIGHTNESS; i <= CAM_STATUS_VDO_FMT; i++) {
        if (camera_attribute[i - 1][CAM_SETTING_ITEM_ENBALE] == 0)
            strcpy(&osd_buf[i][osd_menu_offset + 27], "-");
        else {
            switch (i) {
            case CAM_STATUS_BRIGHTNESS: // brightness
                if (camera_setting_reg_menu[0] > camera_attribute[0][CAM_SETTING_ITEM_DEFAULT]) {
                    dat = camera_setting_reg_menu[0] - camera_attribute[0][CAM_SETTING_ITEM_DEFAULT];
                    uint8ToString(dat, str);
                    if (dat > 99) {
                        strcpy(&osd_buf[1][osd_menu_offset + 24], "+");
                        strcpy(&osd_buf[1][osd_menu_offset + 25], str);
                    } else if (dat > 9) {
                        strcpy(&osd_buf[1][osd_menu_offset + 24], " +");
                        strcpy(&osd_buf[1][osd_menu_offset + 26], str + 1);
                    } else {
                        strcpy(&osd_buf[1][osd_menu_offset + 24], "  +");
                        strcpy(&osd_buf[1][osd_menu_offset + 27], str + 2);
                    }
                } else if (camera_setting_reg_menu[0] < camera_attribute[0][CAM_SETTING_ITEM_DEFAULT]) {
                    dat = camera_attribute[0][CAM_SETTING_ITEM_DEFAULT] - camera_setting_reg_menu[0];
                    uint8ToString(dat, str);
                    if (dat > 99) {
                        strcpy(&osd_buf[1][osd_menu_offset + 24], "-  ");
                        strcpy(&osd_buf[1][osd_menu_offset + 25], str);
                    } else if (dat > 9) {
                        strcpy(&osd_buf[1][osd_menu_offset + 24], " -");
                        strcpy(&osd_buf[1][osd_menu_offset + 26], str + 1);
                    } else {
                        strcpy(&osd_buf[1][osd_menu_offset + 24], "  -");
                        strcpy(&osd_buf[1][osd_menu_offset + 27], str + 2);
                    }
                } else {
                    strcpy(&osd_buf[1][osd_menu_offset + 24], "   0");
                }
                break;
            case CAM_STATUS_SHARPNESS:
            case CAM_STATUS_CONTRAST:
            case CAM_STATUS_SATURATION:
                uint8ToString(camera_setting_reg_menu[i - 1] + 1, str);
                strcpy(&osd_buf[i][osd_menu_offset + 25], str);
                break;
            case CAM_STATUS_SHUTTER:
                if (camera_setting_reg_menu[i - 1] == 0)
                    strcpy(&osd_buf[i][osd_menu_offset + 24], "AUTO");
                else {
                    uint8ToString(camera_setting_reg_menu[i - 1], str);
                    osd_buf[i][osd_menu_offset + 24] = ' ';
                    strcpy(&osd_buf[i][osd_menu_offset + 25], str);
                }
                break;

            case CAM_STATUS_WBRED:  // wb red
            case CAM_STATUS_WBBLUE: // wb blue
                uint8ToString(camera_setting_reg_menu[i - 1], str);
                strcpy(&osd_buf[i][osd_menu_offset + 25], str);
                break;
            case CAM_STATUS_HVFLIP: // hv flip
                strcpy(&osd_buf[i][osd_menu_offset + 21], hv_flip_str[camera_setting_reg_menu[i - 1]]);
                break;
            case CAM_STATUS_NIGHT_MODE: // night mode
            case CAM_STATUS_LED_MODE:   // led mode
                strcpy(&osd_buf[i][osd_menu_offset + 21], switch_str[camera_setting_reg_menu[i - 1]]);
                break;

            case CAM_STATUS_WBMODE: // wb mode
                strcpy(&osd_buf[i][osd_menu_offset + 21], wb_mode_str[camera_setting_reg_menu[i - 1]]);
                break;

            case CAM_STATUS_VDO_FMT: // vdo fmt
                if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V2) {
                    strcpy(&osd_buf[i][osd_menu_offset + 19], resolution_runcam_micro_v2[camera_setting_reg_menu[i - 1]]);
                } else if (camera_type == CAMERA_TYPE_RUNCAM_NANO_90) {
                    strcpy(&osd_buf[i][osd_menu_offset + 19], resolution_runcam_nano_90[camera_setting_reg_menu[i - 1]]);
                } else if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V3) {
                    strcpy(&osd_buf[i][osd_menu_offset + 19], resolution_runcam_micro_v3[camera_setting_reg_menu[i - 1]]);
                }
                osd_buf[i][osd_menu_offset + 29] = '>';
                break;
            default:
                break;
            }
        }
    }
}

void camera_menu_init(void) {
    const char *cam_menu_string[] = {
        "CAMERA MENU     PROFILE < >",
        "BRIGHTNESS",
        "SHARPNESS",
        "CONTRAST",
        "SATURATION",
        "SHUTTER",
        "WB MODE",
        "WB RED",
        "WB BLUE",
        "HV FLIP",
        "MAX GAIN",
        "LED MODE",
        "VIDEO MODE",
        "RESET",
        "EXIT",
        "SAVE&EXIT",
    };
    char *osd_buf_p;
    uint8_t i;

    memset(osd_buf, 0x20, sizeof(osd_buf));
    disp_mode = DISPLAY_CMS;

    for (i = 0; i <= 15; i++) {
        osd_buf_p = osd_buf[i] + osd_menu_offset + 3;
        strcpy(osd_buf_p, cam_menu_string[i]);
    }
    camera_profile_menu = camera_profile_eep;
    camera_setting_reg_menu_update();
    camera_menu_draw_bracket();
    camera_menu_draw_value();
}
void camera_menu_show_repower(void) {
    memset(osd_buf, 0x20, sizeof(osd_buf));
    strcpy(osd_buf[1] + osd_menu_offset + 3, "BW IS CHANGED:");
    if (RF_BW == BW_17M) {
        strcpy(osd_buf[2] + osd_menu_offset + 3, "    WIDE -> NARROW");
    } else {
        strcpy(osd_buf[2] + osd_menu_offset + 3, "    NARROW -> WIDE");
    }
    strcpy(osd_buf[3] + osd_menu_offset + 3, "NEED TO RECONFIG VTX(PRESS OK)");
    strcpy(osd_buf[4] + osd_menu_offset + 3, "NEED TO RECONFIG BW ON GOGGLE");
    strcpy(osd_buf[5] + osd_menu_offset + 3, "> OK");
}

void camera_menu_show_saving(void) {
    memset(osd_buf, 0x20, sizeof(osd_buf));
    strcpy(osd_buf[1] + osd_menu_offset + 3, "SAVING CAMERA PARAMETERS");
    strcpy(osd_buf[2] + osd_menu_offset + 3, "DO NOT POWER OFF THE VTX");
}

void camera_menu_cursor_update(uint8_t erase) {
    if (erase)
        osd_buf[camMenuStatus][osd_menu_offset + 2] = ' ';
    else
        osd_buf[camMenuStatus][osd_menu_offset + 2] = '>';
}
void camera_menu_item_toggle(uint8_t op) {
    camera_menu_cursor_update(1);
    if (op == BTN_DOWN) {
        camMenuStatus++;
        if (camMenuStatus > CAM_STATUS_SAVE_EXIT)
            camMenuStatus = CAM_STATUS_PROFILE;
    } else if (op == BTN_UP) {
        camMenuStatus--;
        if (camMenuStatus > CAM_STATUS_SAVE_EXIT)
            camMenuStatus = CAM_STATUS_SAVE_EXIT;
    }
    camera_menu_cursor_update(0);
}

void camera_profile_menu_toggle(uint8_t op) {
    if (op == BTN_RIGHT) {
        camera_profile_menu++;
        if (camera_profile_menu == CAMERA_PROFILE_NUM)
            camera_profile_menu = 0;
        camera_setting_reg_menu_update();
        reset_isp_need |= camera_set(camera_setting_reg_menu, 0, 0);
    } else if (op == BTN_LEFT) {
        camera_profile_menu--;
        if (camera_profile_menu > CAMERA_PROFILE_NUM)
            camera_profile_menu = CAMERA_PROFILE_NUM - 1;
        camera_setting_reg_menu_update();
        reset_isp_need |= camera_set(camera_setting_reg_menu, 0, 0);
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
    uint8_t item = camMenuStatus - 1;
    uint8_t step = 1;

    switch (camMenuStatus) {
    case CAM_STATUS_BRIGHTNESS:
    case CAM_STATUS_WBRED:
    case CAM_STATUS_WBBLUE:
        if (!camera_attribute[item][CAM_SETTING_ITEM_ENBALE])
            return;
#if (0)
        // if wb mode if auto, do not modify wbred/wbblue
        if ((camMenuStatus == CAM_STATUS_WBRED || camMenuStatus == CAM_STATUS_WBBLUE) &&
            (!camera_setting_reg_menu[4]))
            return;
#endif

        if (op == BTN_RIGHT) {
            step = camera_menu_long_press(op, last_op, 0);
            if (((camera_setting_reg_menu[item] + step) & 0xff) < camera_setting_reg_menu[item]) { // overflow
                camera_setting_reg_menu[item] = 0xff;
            } else {
                camera_setting_reg_menu[item] += step;
            }

            if (camera_setting_reg_menu[item] > camera_attribute[item][CAM_SETTING_ITEM_MAX])
                camera_setting_reg_menu[item] = camera_attribute[item][CAM_SETTING_ITEM_MAX];

            camera_set(camera_setting_reg_menu, 0, 0);
        } else if (op == BTN_LEFT) {
            step = camera_menu_long_press(op, last_op, 0);
            if (((camera_setting_reg_menu[item] - step) & 0xff) > camera_setting_reg_menu[item]) // overflow
                camera_setting_reg_menu[item] = 0;
            else
                camera_setting_reg_menu[item] -= step;

            if (camera_setting_reg_menu[item] < camera_attribute[item][CAM_SETTING_ITEM_MIN])
                camera_setting_reg_menu[item] = camera_attribute[item][CAM_SETTING_ITEM_MIN];

            camera_set(camera_setting_reg_menu, 0, 0);
        } else if (op == BTN_MID) {
            step = camera_menu_long_press(op, last_op, 1);
        }
        break;
    case CAM_STATUS_SHARPNESS:
    case CAM_STATUS_CONTRAST:
    case CAM_STATUS_SATURATION:
    case CAM_STATUS_SHUTTER:
    case CAM_STATUS_WBMODE:
    case CAM_STATUS_HVFLIP:
    case CAM_STATUS_NIGHT_MODE:
    case CAM_STATUS_LED_MODE:
    case CAM_STATUS_VDO_FMT:
        if (!camera_attribute[item][CAM_SETTING_ITEM_ENBALE])
            return;

        if (op == BTN_RIGHT) {
            camera_setting_reg_menu[item]++;
            if (camera_setting_reg_menu[item] > camera_attribute[item][CAM_SETTING_ITEM_MAX])
                camera_setting_reg_menu[item] = camera_attribute[item][CAM_SETTING_ITEM_MIN];

            if (camMenuStatus != CAM_STATUS_VDO_FMT) { // vdo format will be configured when exit camera menu
                camera_set(camera_setting_reg_menu, 0, 0);
            }
        } else if (op == BTN_LEFT) {
            camera_setting_reg_menu[item]--;
            if (camera_attribute[item][CAM_SETTING_ITEM_MIN] == 0) {
                if (camera_setting_reg_menu[item] > camera_attribute[item][CAM_SETTING_ITEM_MAX])
                    camera_setting_reg_menu[item] = camera_attribute[item][CAM_SETTING_ITEM_MAX];
            } else if (camera_setting_reg_menu[item] < camera_attribute[item][CAM_SETTING_ITEM_MIN])
                camera_setting_reg_menu[item] = camera_attribute[item][CAM_SETTING_ITEM_MAX];

            if (camMenuStatus != CAM_STATUS_VDO_FMT) { // vdo format will be configured when exit camera menu
                camera_set(camera_setting_reg_menu, 0, 0);
            }
        }
        break;

    default:
        break;
    }
}

#ifdef USE_MSP
uint8_t camera_status_update(uint8_t op) {

    uint8_t ret = 0;
    static uint8_t step = 1;
    static uint8_t last_op = BTN_RIGHT;
    static uint16_t saving_start_sec = 0;

    if (op >= BTN_INVALID)
        return ret;

    if (camera_type == CAMERA_TYPE_UNKNOW ||
        camera_type == CAMERA_TYPE_OUTDATED) {
        camera_button_op(op);
        return ret;
    }

    switch (camMenuStatus) {
    case CAM_STATUS_IDLE:
        if (op == BTN_MID) {
            reset_isp_need = 0;
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
    case CAM_STATUS_SHUTTER:
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
            ; // note brightness/wb red/wb blue support long press
        else if (last_op == op)
            break;

        camera_menu_item_toggle(op);

        if (camMenuStatus == CAM_STATUS_PROFILE)
            camera_profile_menu_toggle(op);
        else
            camera_setting_reg_menu_toggle(last_op, op);
        camera_menu_draw_value();
        break;
    case CAM_STATUS_RESET:
        if (last_op == op)
            break;

        camera_menu_item_toggle(op);

        if (op == BTN_RIGHT) {
            uint8_t video_mode = camera_setting_reg_menu[11];
            camera_setting_profile_reset(camera_profile_menu);
            camera_setting_reg_menu_update();
            camera_setting_reg_menu[11] = video_mode;
            camera_set(camera_setting_reg_menu, 0, 0);
            camera_menu_draw_value();
        }
        break;

    case CAM_STATUS_EXIT:
        if (last_op == op)
            break;

        camera_menu_item_toggle(op);

        if (op == BTN_RIGHT) {
            camera_setting_reg_menu_update();
            camera_set(camera_setting_reg_menu, 0, 0);

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

            if (RF_BW_to_be_changed()) {
                camera_menu_show_repower();
                camMenuStatus = CAM_STATUS_REPOWER;
            } else {
                camera_profile_write();
                reset_isp_need |= camera_set(camera_setting_reg_menu, 1, 0);
                camera_setting_reg_eep_update();
                camera_setting_profile_write(0xff);

                if (reset_isp_need) {
                    if (camera_mfr == CAMERA_MFR_RUNCAM) {
                        runcam_reset_isp();
                        camera_mode_detect(0);
                    }
                }
                camera_menu_show_saving();
                saving_start_sec = seconds;
                camMenuStatus = CAM_STATUS_SAVING;
            }
        }
        break;
    case CAM_STATUS_REPOWER:
        if (op == BTN_RIGHT) {
            camera_profile_write();
            reset_isp_need |= camera_set(camera_setting_reg_menu, 1, 0);
            camera_setting_reg_eep_update();
            camera_setting_profile_write(0xff);
            if (reset_isp_need) {
                if (camera_mfr == CAMERA_MFR_RUNCAM) {
                    runcam_reset_isp();
                    camera_mode_detect(0);
                }
            }

            if (PIT_MODE != PIT_OFF) {
                Init_6300RF(RF_FREQ, POWER_MAX + 1);
                vtx_pit = PIT_P1MW;
            } else {
                WriteReg(0, 0x8F, 0x00);
                WriteReg(0, 0x8F, 0x01);
                DM6300_Init(RF_FREQ, RF_BW);
                DM6300_SetChannel(RF_FREQ);
                DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                cur_pwr = RF_POWER;
                WriteReg(0, 0x8F, 0x11);
            }
            DM6300_AUXADC_Calib();

            camera_menu_show_saving();
            saving_start_sec = seconds;
            camMenuStatus = CAM_STATUS_SAVING;
        }
        break;

    case CAM_STATUS_SAVING:
        if (seconds - saving_start_sec > 1) {
            camMenuStatus = CAM_STATUS_IDLE;
            ret = 1;
        }
        break;

    default:
        break;
    } // switch(camMenuStatus)

    last_op = op;

    return ret;
}
#endif

void camera_select_menu_init(void) {
    const char *cam_select_menu_string[] = {
        "> ECO CAMERA MENU",
        "  LUX CAMERA MENU",
        "  VTX RATIO FOR LUX CAMERA  <4:3> ",
        "  EXIT",
    };
    char *osd_buf_p;
    uint8_t i;

    for (i = 0; i <= CAM_SELECT_EXIT; i++) {
        osd_buf_p = osd_buf[i] + osd_menu_offset;
        strcpy(osd_buf_p, cam_select_menu_string[i]);
    }
    camera_select_menu_ratio_upate();
}

void camera_select_menu_cursor_update(uint8_t index) {
    uint8_t i;
    for (i = 0; i <= CAM_SELECT_EXIT; i++) {
        if (i == index)
            osd_buf[i][osd_menu_offset] = '>';
        else
            osd_buf[i][osd_menu_offset] = ' ';
    }
}

void camera_select_menu_ratio_upate() {
    if (camRatio == 1)
        strcpy(osd_buf[2] + osd_menu_offset + 28, "<4:3> ");
    else
        strcpy(osd_buf[2] + osd_menu_offset + 28, "<16:9>");
}

void camera_menu_mode_exit_note() {
    const char note_string[] = "LEFT MOVE THROTTLE TO EXIT CAMERA MENU";
    strcpy(osd_buf[15] + 5, note_string);
}