#include "camera.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "i2c_device.h"
#include "isr.h"
#include "msp_displayport.h"
#include "print.h"
#include "runcam.h"

uint8_t camera_type = 0;
camera_manufacturer_e camera_mfr = CAMERA_MFR_UNKNOW;
uint8_t camera_profile;
uint8_t camera_setting_reg_eep[CAMERA_PROFILE_NUM][CAMERA_SETTING_NUM];
uint8_t camera_setting_reg_set[CAMERA_SETTING_NUM];

uint8_t CAM_MODE = CAM_720P60;
/*
0: 4:3
1: 16_9 crop
2: 16_9 full
*/
uint8_t camRatio = 0;
uint8_t camMenuStatus = CAM_STATUS_IDLE;
uint8_t bw_changed = 0;

void camMenuStringUpdate(uint8_t status);
void camMenuSetVdoRatioInit();

void camera_type_detect(void) {

    runcam_type_detect();
    if (camera_type) {
        camera_mfr = CAMERA_MFR_RUNCAM;
        return;
    }

#if (0)
    foxeer_detect_type();
    if (camera_type) {
        camera_mfr = CAMERA_MFR_FOXEER;
        return;
    }
#endif
}

void camera_identify_resolution(void) {
}

uint8_t CamDetect() {
    int mode = CAM_720P60;
    uint8_t cycles = 4;
    uint8_t loss = 0;
    uint8_t detect_tries = 0;
    uint8_t status_reg = 0;

    // init tc3587 and detect fps
    WriteReg(0, 0x8F, 0x91);

    if (camera_type) {
        if (camera_type == RUNCAM_NANO_90) {
            Init_TC3587(1);
            Set_720P90(0);
            mode = CAM_720X540_90;
        } else {
            Set_720P60(IS_RX);
            Init_TC3587(0);
        }
        I2C_Write16(ADDR_TC3587, 0x0058, 0x00e0);
    } else {
        while (cycles) {
            if (mode == CAM_720P50) {
                Init_TC3587(0);
                Set_720P50(IS_RX);
                debugf("\r\nCamDetect: Set 50fps.");
            } else if (mode == CAM_720P60) {
                Init_TC3587(0);
                Set_720P60(IS_RX);
                debugf("\r\nCamDetect: Set 60fps.");
            }
            WAIT(100);

            for (detect_tries = 0; detect_tries < 5; detect_tries++) {
                status_reg = ReadReg(0, 0x02);
                debugf("\r\nCamDetect status_reg: %x", status_reg);

                if ((status_reg >> 4) != 0) {
                    loss = 1;
                }
                WAIT(5);
            }

            if (loss == 0)
                break;

            mode = (mode == CAM_720P60) ? CAM_720P50 : CAM_720P60;

            loss = 0;
            cycles--;
        }
    }

#ifdef _DEBUG_MODE
    debugf("\r\ncameraID: %x", (uint16_t)camera_type);
#endif

    return mode;
}

void camera_button_init() {
    WriteReg(0, 0x17, 0xC0);
    WriteReg(0, 0x16, 0x00);
    WriteReg(0, 0x15, 0x02);
    WriteReg(0, 0x14, 0x00);
}

/////////////////////////////////////////////////////////////////
// runcam config
void Runcam_SetBrightness(uint8_t val) {
#if (0)
    uint32_t d = 0x04004800;
    uint32_t val_32;

    camCfg_Cur.brightness = val;

    if (camera_type == RUNCAM_MICRO_V1)
        d = 0x0452484e;
    else if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_NANO_90)
        d = 0x04504850;

    val_32 = (uint32_t)val;

    // indoor
    d += val_32;
    d -= CAM_BRIGHTNESS_INITIAL;

    // outdoor
    d += (val_32 << 16);
    d -= ((uint32_t)CAM_BRIGHTNESS_INITIAL << 16);

    RUNCAM_Write(camera_type, 0x50, d);
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM brightness:0x%02x", (uint16_t)val);
#endif
#endif
}

void Runcam_SetSharpness(uint8_t val) {
#if (0)
    uint32_t d = 0;

    camCfg_Cur.sharpness = val;

    if (camera_type == RUNCAM_MICRO_V1)
        d = 0x03FF0100;
    else if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_NANO_90)
        d = 0x03FF0000;

    if (val == 2) {
        RUNCAM_Write(camera_type, 0x0003C4, 0x03FF0000);
        RUNCAM_Write(camera_type, 0x0003CC, 0x28303840);
        RUNCAM_Write(camera_type, 0x0003D8, 0x28303840);
    } else if (val == 1) {
        RUNCAM_Write(camera_type, 0x0003C4, 0x03FF0000);
        RUNCAM_Write(camera_type, 0x0003CC, 0x14181C20);
        RUNCAM_Write(camera_type, 0x0003D8, 0x14181C20);
    } else if (val == 0) {
        RUNCAM_Write(camera_type, 0x0003C4, d);
        RUNCAM_Write(camera_type, 0x0003CC, 0x0A0C0E10);
        RUNCAM_Write(camera_type, 0x0003D8, 0x0A0C0E10);
    }
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM sharpness:0x%02x", (uint16_t)val);
#endif
#endif
}

uint8_t Runcam_SetSaturation(uint8_t val) {
#if (1)
    return 0;
#else
    uint8_t ret = 1;
    uint32_t d = 0x20242626;

    camCfg_Cur.saturation = val;

    // initial
    if (camera_type == RUNCAM_MICRO_V1)
        d = 0x20242626;
    else if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
        d = 0x24282c30;

    if (val == 0)
        d = 0x00000000;
    else if (val == 1)
        d -= 0x18181414;
    else if (val == 2)
        d -= 0x14141010;
    else if (val == 3)
        d -= 0x0c0c0808;
    else if (val == 4) // low
        d -= 0x08080404;
    else if (val == 5) // normal
        ;
    else if (val == 6) // high
        d += 0x04041418;
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM saturation:%02x", (uint16_t)val);
#endif

    ret = RUNCAM_Write(camera_type, 0x0003A4, d);
    return ret;
#endif
}

void Runcam_SetContrast(uint8_t val) {
#if (0)
    uint32_t d = 0x46484A4C;

    camCfg_Cur.contrast = val;

    if (camera_type == RUNCAM_MICRO_V1)
        d = 0x46484A4C;
    else if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
        d = 0x36383a3c;

    if (val == 0) // low
        d -= 0x06040404;
    else if (val == 1) // normal
        ;
    else if (val == 2) // high
        d += 0x04040404;

    RUNCAM_Write(camera_type, 0x00038C, d);
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM contrast:%02x", (uint16_t)val);
#endif
#endif
}

void Runcam_SetVdoRatio(uint8_t ratio) {
#if (0)
    /*
        0: 720p60 4:3 default
        1: 720p60 16:9 crop
        2: 720p30 16:9 full
    */
    if (camera_type != RUNCAM_MICRO_V2)
        return;

    if (ratio == 0)
        RUNCAM_Write(camera_type, 0x000008, 0x0008910B);
    else if (ratio == 1)
        RUNCAM_Write(camera_type, 0x000008, 0x00089102);
    else if (ratio == 2)
        RUNCAM_Write(camera_type, 0x000008, 0x00089110);
    // save
    RUNCAM_Write(camera_type, 0x000694, 0x00000310);
    RUNCAM_Write(camera_type, 0x000694, 0x00000311);
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM VdoRatio:%02x", (uint16_t)ratio);
#endif
#endif
}

#if (0)
uint8_t Runcam_GetVdoFormat(uint8_t camera_type) {
    /*
        0: 720p60 16:9
        0: 720p60 4:3
        1: 720p30 16:9
        1: 720p30 4:3
    */
    uint32_t val;
    uint8_t ret = 0;

    if (camera_type != RUNCAM_MICRO_V2)
        return ret;
    if (cameraTarget == 1)
        return ret;

    val = RUNCAM_Read(camera_type, 0x000008);
    if (val == 0x00089102) { // 720p60  16:9
        ret = 0;
        camRatio = 0;
    } else if (val == 0x0008910B) { // 720p60  4:3
        ret = 0;
        camRatio = 1;
    } else if (val == 0x00089104) { // 720p30 16:9
        ret = 1;
        camRatio = 0;
    } else if (val == 0x00089109) { // 720p60 4:3
        ret = 1;
        camRatio = 1;
    } else {
        ret = 2;
        camRatio = 0;
    }

#ifdef _DEBUG_CAMERA
    debugf("\r\nVdoFormat:%02x, 4_3:%02x", (uint16_t)ret, (uint16_t)camRatio);
#endif
    return ret;
}
#endif

void Runcam_SetHVFlip(uint8_t val) {
#if (0)
    if (camera_type != RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
        return;

    camCfg_Cur.hvFlip = val;

    if (val == 0)
        RUNCAM_Write(camera_type, 0x000040, 0x0022ffa9);
    else if (val == 1)
        RUNCAM_Write(camera_type, 0x000040, 0x002effa9);
#endif
}

void Runcam_SetNightMode(uint8_t val) {
#if (0)
    /*
        0: night mode off
        1: night mode on
    */
    if (camera_type != RUNCAM_MICRO_V2 || camera_type != RUNCAM_MICRO_V3)
        return;

    camCfg_Cur.nightMode = val;

    if (val == 1) { // Max gain on
        RUNCAM_Write(camera_type, 0x000070, 0x10000040);
        // RUNCAM_Write(camera_type, 0x000070, 0x04000040);
        RUNCAM_Write(camera_type, 0x000718, 0x28002700);
        RUNCAM_Write(camera_type, 0x00071c, 0x29002800);
        RUNCAM_Write(camera_type, 0x000720, 0x29002900);
    } else if (val == 0) { // Max gain off
        RUNCAM_Write(camera_type, 0x000070, 0x10000040);
        // RUNCAM_Write(camera_type, 0x000070, 0x04000040);
        RUNCAM_Write(camera_type, 0x000718, 0x30002900);
        RUNCAM_Write(camera_type, 0x00071c, 0x32003100);
        RUNCAM_Write(camera_type, 0x000720, 0x34003300);
    }

    // save
    RUNCAM_Write(camera_type, 0x000694, 0x00000310);
    RUNCAM_Write(camera_type, 0x000694, 0x00000311);
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM NightMode:%02x", (uint16_t)val);
#endif
#endif
}

void Runcam_SetWB(uint8_t *wbRed, uint8_t *wbBlue, uint8_t wbMode) {
#if (0)
    uint32_t wbRed_u32 = 0x02000000;
    uint32_t wbBlue_u32 = 0x00000000;
    uint8_t i;

    camCfg_Cur.wbMode = wbMode;
    for (i = 0; i < WBMODE_MAX; i++) {
        camCfg_Cur.wbRed[i] = wbRed[i];
        camCfg_Cur.wbBlue[i] = wbBlue[i];
    }

    if (wbMode) {
        wbRed_u32 += ((uint32_t)wbRed[wbMode - 1] << 2);
        wbBlue_u32 += ((uint32_t)wbBlue[wbMode - 1] << 2);
        wbRed_u32++;
        wbBlue_u32++;
    }

    if (wbMode) { // MWB
        RUNCAM_Write(camera_type, 0x0001b8, 0x020b007b);
        RUNCAM_Write(camera_type, 0x000204, wbRed_u32);
        RUNCAM_Write(camera_type, 0x000208, wbBlue_u32);
    } else { // AWB
        RUNCAM_Write(camera_type, 0x0001b8, 0x020b0079);
    }
#endif
}

void camera_reg_write_eep(uint16_t addr, uint8_t val) {
    I2C_Write8_Wait(10, ADDR_EEPROM, addr, val);
}
uint8_t camera_reg_read_eep(uint16_t addr) {
    return I2C_Read8_Wait(10, ADDR_EEPROM, addr);
}

void camera_setting_profile_read(uint8_t profile) {
    uint8_t i;
    for (i = 0; i < CAMERA_SETTING_NUM; i++)
        camera_setting_reg_eep[profile][i] = camera_reg_read_eep(EEP_ADDR_CAM_SETTING + profile * CAMERA_SETTING_NUM + i);
}
void camera_setting_profile_write(uint8_t profile) {
    uint8_t i;
    for (i = 0; i < CAMERA_SETTING_NUM; i++)
        camera_reg_write_eep(EEP_ADDR_CAM_SETTING + profile * CAMERA_SETTING_NUM + i, camera_setting_reg_eep[profile][i]);
}
void camera_setting_profile_reset(uint8_t profile) {
    if (camera_mfr == CAMERA_MFR_RUNCAM)
        runcam_setting_profile_reset(camera_setting_reg_eep[profile]);
#if (0)
    else if (camera_mfr == CAMERA_MFR_FOXEER)
        foxeer_setting_profile_reset(camera_setting_reg_eep[profile]);
#endif
}
void camera_setting_profile_check(uint8_t profile) {
    uint8_t need_reset = 0;
    if (camera_mfr == CAMERA_MFR_RUNCAM)
        need_reset = runcam_setting_profile_check(camera_setting_reg_eep[profile]);
#if (0)
    else if (camera_mfr == CAMERA_MFR_FOXEER)
        need_reset = foxeer_setting_profile_check(camera_setting_reg_eep[profile]);
#endif

    if (need_reset) {
        camera_setting_profile_reset(profile);
        camera_setting_profile_write(profile);
    }
}

void camera_profile_read(void) {
    camera_profile = camera_reg_read_eep(EEP_ADDR_CAM_PROFILE);
}
void camera_profile_write(void) {
    camera_reg_write_eep(EEP_ADDR_CAM_PROFILE, camera_profile);
}
void camera_profile_reset(void) {
    camera_profile = 0;
}
void camera_profile_check(void) {
    if (camera_profile >= CAMERA_PROFILE_NUM) {
        camera_profile = 0;
        camera_profile_write();
    }
}

void camera_setting_read(void) {
    uint8_t i;
    camera_type_e camera_type_last;

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
    } else {
        camera_profile_read();
        camera_profile_check();
        for (i = 0; i < CAMERA_PROFILE_NUM; i++) {
            camera_setting_profile_read(i);
            camera_setting_profile_check(i);
        }
    }
}

void camera_set(uint8_t is_init) {
    if (camera_mfr == CAMERA_MFR_RUNCAM)
        runcam_set(camera_setting_reg_eep[camera_profile], is_init);
#if (0)
    else if (camera_mfr == CAMERA_MFR_FOXEER)
        foxeer_set(camera_setting_reg_eep[camera_profile]);
#endif
}

void CameraInit() {
    camera_type_detect();
    camera_setting_read();
    camera_set(1);
    Init_TC3587(0);

    camera_button_init();
}

void Cam_Button_ENTER() { WriteReg(0, 0x14, 0x32); }
void Cam_Button_RIGHT() { WriteReg(0, 0x14, 0x58); }
void Cam_Button_DOWN() { WriteReg(0, 0x14, 0x64); }
void Cam_Button_LEFT() { WriteReg(0, 0x14, 0x3F); }
void Cam_Button_UP() { WriteReg(0, 0x14, 0x4B); }
void Cam_Button_MID() { WriteReg(0, 0x14, 0x00); }

#ifdef USE_MSP
uint8_t camStatusUpdate(uint8_t op) {
#if (1)
    return 0;
#else
    uint8_t ret = 0;
    uint8_t i;
    static uint8_t step = 1;
    static uint8_t cnt;
    static uint8_t last_op = BTN_RIGHT;

    if (op >= BTN_INVALID)
        return ret;

    if (camera_type == 0) {
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
        } // switch(op)

        return ret;
    }

    switch (camMenuStatus) {
    case CAM_STATUS_IDLE:
        if (op == BTN_MID) {
            GetCamCfg_Menu(1);
            cnt = 0;
            if (camera_type == RUNCAM_MICRO_V1)
                camMenuStatus = CAM_STATUS_BRIGHTNESS;
            else
                camMenuStatus = CAM_STATUS_PROFILE;
        }
        break;

    case CAM_STATUS_PROFILE:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_SAVE_EXIT;
        else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_BRIGHTNESS;
        else if (op == BTN_LEFT) {
            camProfile_Menu--;
            if (camProfile_Menu >= Profile_Max)
                camProfile_Menu = Profile_Max - 1;

            camProfile = camProfile_Menu;
            GetCamCfg(0);
            GetCamCfg_Menu(0);
            SetCamCfg(&camCfg_Menu, 0);
        } else if (op == BTN_RIGHT) {
            camProfile_Menu++;
            if (camProfile_Menu >= Profile_Max)
                camProfile_Menu = 0;

            camProfile = camProfile_Menu;
            GetCamCfg(0);
            GetCamCfg_Menu(0);
            SetCamCfg(&camCfg_Menu, 0);
        }
        break;

    case CAM_STATUS_BRIGHTNESS:
        if (op == BTN_UP) {
            if (camera_type == RUNCAM_MICRO_V1)
                camMenuStatus = CAM_STATUS_SAVE_EXIT;
            else
                camMenuStatus = CAM_STATUS_PROFILE;
        } else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_SHARPNESS;
        else if (op == BTN_RIGHT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_RIGHT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;

            camCfg_Menu.brightness += step;
            if (camCfg_Menu.brightness > BRIGHTNESS_MAX)
                camCfg_Menu.brightness = BRIGHTNESS_MAX;

            Runcam_SetBrightness(camCfg_Menu.brightness);
        } else if (op == BTN_LEFT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_LEFT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;

            camCfg_Menu.brightness -= step;
            if (camCfg_Menu.brightness < BRIGHTNESS_MIN)
                camCfg_Menu.brightness = BRIGHTNESS_MIN;

            Runcam_SetBrightness(camCfg_Menu.brightness);
        }
        break;

    case CAM_STATUS_SHARPNESS:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_BRIGHTNESS;
        else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_CONTRAST;
        else if (op == BTN_RIGHT) {
            camCfg_Menu.sharpness++;
            if (camCfg_Menu.sharpness > SHARPNESS_MAX)
                camCfg_Menu.sharpness = SHARPNESS_MAX;
            Runcam_SetSharpness(camCfg_Menu.sharpness);
        } else if (op == BTN_LEFT) {
            camCfg_Menu.sharpness--;
            if (camCfg_Menu.sharpness > SHARPNESS_MAX)
                camCfg_Menu.sharpness = SHARPNESS_MIN;
            Runcam_SetSharpness(camCfg_Menu.sharpness);
        }
        break;

    case CAM_STATUS_CONTRAST:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_SHARPNESS;
        if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_SATURATION;
        else if (op == BTN_RIGHT) {
            camCfg_Menu.contrast++;
            if (camCfg_Menu.contrast > CONTRAST_MAX)
                camCfg_Menu.contrast = CONTRAST_MAX;
            Runcam_SetContrast(camCfg_Menu.contrast);
        } else if (op == BTN_LEFT) {
            camCfg_Menu.contrast--;
            if (camCfg_Menu.contrast > CONTRAST_MAX)
                camCfg_Menu.contrast = CONTRAST_MIN;
            Runcam_SetContrast(camCfg_Menu.contrast);
        }
        break;

    case CAM_STATUS_SATURATION:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_CONTRAST;
        else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_WBMODE;
        else if (op == BTN_RIGHT) {
            camCfg_Menu.saturation++;
            if (camCfg_Menu.saturation > SATURATION_MAX)
                camCfg_Menu.saturation = SATURATION_MAX;
            Runcam_SetSaturation(camCfg_Menu.saturation);
        } else if (op == BTN_LEFT) {
            camCfg_Menu.saturation--;
            if (camCfg_Menu.saturation > SATURATION_MAX)
                camCfg_Menu.saturation = SATURATION_MIN;
            Runcam_SetSaturation(camCfg_Menu.saturation);
        }
        break;

    case CAM_STATUS_WBMODE:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_SATURATION;
        else if (op == BTN_DOWN) {
            if (camCfg_Menu.wbMode)
                camMenuStatus = CAM_STATUS_WBRED;
            else if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
                camMenuStatus = CAM_STATUS_HVFLIP;
            else
                camMenuStatus = CAM_STATUS_RESET;
        } else if (op == BTN_RIGHT) {
            camCfg_Menu.wbMode++;
            if (camCfg_Menu.wbMode > WBMODE_MAX)
                camCfg_Menu.wbMode = WBMODE_MAX;
            Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
        } else if (op == BTN_LEFT) {
            camCfg_Menu.wbMode--;
            if (camCfg_Menu.wbMode > WBMODE_MAX)
                camCfg_Menu.wbMode = WBMODE_MIN;
            Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
        }
        break;

    case CAM_STATUS_WBRED:
        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_WBMODE;
        else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_WBBLUE;
        else if (op == BTN_RIGHT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_RIGHT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;
            camCfg_Menu.wbRed[camCfg_Menu.wbMode - 1] += step;

            Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
        } else if (op == BTN_LEFT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_LEFT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;
            camCfg_Menu.wbRed[camCfg_Menu.wbMode - 1] -= step;

            Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
        }
        break;

    case CAM_STATUS_WBBLUE:
        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_WBRED;
        else if (op == BTN_DOWN) {
            if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
                camMenuStatus = CAM_STATUS_HVFLIP;
            else
                camMenuStatus = CAM_STATUS_RESET;
        } else if (op == BTN_RIGHT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_RIGHT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;
            camCfg_Menu.wbBlue[camCfg_Menu.wbMode - 1] += step;

            Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
        } else if (op == BTN_LEFT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_LEFT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;
            camCfg_Menu.wbBlue[camCfg_Menu.wbMode - 1] -= step;

            Runcam_SetWB(camCfg_Menu.wbRed, camCfg_Menu.wbBlue, camCfg_Menu.wbMode);
        }
        break;

    case CAM_STATUS_HVFLIP:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP) {
            if (camCfg_Menu.wbMode)
                camMenuStatus = CAM_STATUS_WBBLUE;
            else
                camMenuStatus = CAM_STATUS_WBMODE;
        } else if (op == BTN_DOWN) {
            if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
                camMenuStatus = CAM_STATUS_NIGHTMODE;
            else
                camMenuStatus = CAM_STATUS_RESET;
        } else if (op == BTN_RIGHT || op == BTN_LEFT) {
            camCfg_Menu.hvFlip = 1 - camCfg_Menu.hvFlip;
            Runcam_SetHVFlip(camCfg_Menu.hvFlip);
        }
        break;

    case CAM_STATUS_NIGHTMODE:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP) {
            if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
                camMenuStatus = CAM_STATUS_HVFLIP;
            else if (camCfg_Menu.wbMode)
                camMenuStatus = CAM_STATUS_WBBLUE;
            else
                camMenuStatus = CAM_STATUS_WBMODE;
        } else if (op == BTN_DOWN) {
            if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
                camMenuStatus = CAM_STATUS_VDO_RATIO;
            else
                camMenuStatus = CAM_STATUS_RESET;
        } else if (op == BTN_RIGHT || op == BTN_LEFT) {
            camCfg_Menu.nightMode = 1 - camCfg_Menu.nightMode;
            Runcam_SetNightMode(camCfg_Menu.nightMode);
        }
        break;

    case CAM_STATUS_VDO_RATIO:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP) {
            if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
                camMenuStatus = CAM_STATUS_NIGHTMODE;
            else if (camCfg_Menu.wbMode)
                camMenuStatus = CAM_STATUS_WBBLUE;
            else
                camMenuStatus = CAM_STATUS_WBMODE;
        } else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_RESET;
        else if (op == BTN_RIGHT) {
            camMenuSetVdoRatioInit();
            camMenuStatus = CAM_STATUS_SET_VDO_RATIO_4_3;
        }
        break;

    case CAM_STATUS_SET_VDO_RATIO_4_3:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_SET_VDO_RATIO_RETURN;
        else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_CROP;
        else if (op == BTN_RIGHT) {
            Runcam_SetVdoRatio(0);
            camera_write_reg_eep(EEP_ADDR_CAM_RATIO, 0);
            memset(osd_buf, 0x20, sizeof(osd_buf));
            strcpy(osd_buf[1] + osd_menu_offset + 2, " NEED TO REPOWER VTX");
            camMenuStatus = CAM_STATUS_REPOWER;
        }
        break;

    case CAM_STATUS_SET_VDO_RATIO_16_9_CROP:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_SET_VDO_RATIO_4_3;
        else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_FULL;
        else if (op == BTN_RIGHT) {
            Runcam_SetVdoRatio(1);
            camera_write_reg_eep(EEP_ADDR_CAM_RATIO, 1);
            memset(osd_buf, 0x20, sizeof(osd_buf));
            strcpy(osd_buf[1] + osd_menu_offset + 2, " NEED TO REPOWER VTX");
            camMenuStatus = CAM_STATUS_REPOWER;
        }
        break;

    case CAM_STATUS_SET_VDO_RATIO_16_9_FULL:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_CROP;
        else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_SET_VDO_RATIO_RETURN;
        else if (op == BTN_RIGHT) {
            Runcam_SetVdoRatio(2);
            camera_write_reg_eep(EEP_ADDR_CAM_RATIO, 2);
            memset(osd_buf, 0x20, sizeof(osd_buf));
            strcpy(osd_buf[1] + osd_menu_offset + 2, " NEED TO REPOWER VTX");
            camMenuStatus = CAM_STATUS_REPOWER;
        }
        break;

    case CAM_STATUS_SET_VDO_RATIO_RETURN:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_SET_VDO_RATIO_16_9_FULL;
        else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_SET_VDO_RATIO_4_3;
        else if (op == BTN_RIGHT) {
            camMenuInit();
            camMenuStatus = CAM_STATUS_VDO_RATIO;
        }
        break;

    case CAM_STATUS_RESET:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP) {
            if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3)
                camMenuStatus = CAM_STATUS_VDO_RATIO;
            else if (camCfg_Menu.wbMode)
                camMenuStatus = CAM_STATUS_WBBLUE;
            else
                camMenuStatus = CAM_STATUS_WBMODE;
        } else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_EXIT;
        else if (op == BTN_RIGHT) {
            if (camera_type == RUNCAM_MICRO_V1) {
                camCfg_Menu.brightness = camParameterInit[0][0];
                camCfg_Menu.sharpness = camParameterInit[0][1];
                camCfg_Menu.saturation = camParameterInit[0][2];
                camCfg_Menu.contrast = camParameterInit[0][3];
                camCfg_Menu.hvFlip = camParameterInit[0][4];
                camCfg_Menu.nightMode = camParameterInit[0][5];
                camCfg_Menu.wbMode = camParameterInit[0][6];
                for (i = 0; i < WBMODE_MAX; i++) {
                    camCfg_Menu.wbRed[i] = camParameterInit[0][7 + i];
                    camCfg_Menu.wbBlue[i] = camParameterInit[0][11 + i];
                }
            } else if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3) {
                camCfg_Menu.brightness = camParameterInit[camProfile_Menu + 1][0];
                camCfg_Menu.sharpness = camParameterInit[camProfile_Menu + 1][1];
                camCfg_Menu.saturation = camParameterInit[camProfile_Menu + 1][2];
                camCfg_Menu.contrast = camParameterInit[camProfile_Menu + 1][3];
                camCfg_Menu.hvFlip = camParameterInit[camProfile_Menu + 1][4];
                camCfg_Menu.nightMode = camParameterInit[camProfile_Menu + 1][5];
                camCfg_Menu.wbMode = camParameterInit[camProfile_Menu + 1][6];
                for (i = 0; i < WBMODE_MAX; i++) {
                    camCfg_Menu.wbRed[i] = camParameterInit[camProfile_Menu + 1][7 + i];
                    camCfg_Menu.wbBlue[i] = camParameterInit[camProfile_Menu + 1][11 + i];
                }
            }
            SetCamCfg(&camCfg_Menu, 0);
        }
        break;

    case CAM_STATUS_EXIT:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_RESET;
        else if (op == BTN_DOWN)
            camMenuStatus = CAM_STATUS_SAVE_EXIT;
        else if (op == BTN_RIGHT) {
            camMenuStatus = CAM_STATUS_IDLE;
            msp_tx_cnt = 0;
            ret = 1;
            SetCamCfg(&camCfg, 0);
        }
        break;

    case CAM_STATUS_SAVE_EXIT:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            camMenuStatus = CAM_STATUS_EXIT;
        else if (op == BTN_DOWN) {
            if (camera_type == RUNCAM_MICRO_V1)
                camMenuStatus = CAM_STATUS_BRIGHTNESS;
            else
                camMenuStatus = CAM_STATUS_PROFILE;
        } else if (op == BTN_RIGHT) {
            camMenuStatus = CAM_STATUS_IDLE;
            msp_tx_cnt = 0;
            ret = 1;
            SaveCamCfg_Menu();
        }
        break;

    case CAM_STATUS_REPOWER:
        break;

    default:
        break;
    } // switch(camMenuStatus)

    last_op = op;
    camMenuStringUpdate(camMenuStatus);

    return ret;
#endif
}
#endif

void camMenuDrawBracket(void) {
#if (0)
    if (camera_type == RUNCAM_MICRO_V1) {
        osd_buf[CAM_STATUS_PROFILE][osd_menu_offset + 17] = ' ';
        osd_buf[CAM_STATUS_PROFILE][osd_menu_offset + 29] = ' ';
    } else {
        osd_buf[CAM_STATUS_PROFILE][osd_menu_offset + 17] = '<';
        osd_buf[CAM_STATUS_PROFILE][osd_menu_offset + 29] = '>';
    }
    osd_buf[CAM_STATUS_BRIGHTNESS][osd_menu_offset + 19] = '<';
    osd_buf[CAM_STATUS_BRIGHTNESS][osd_menu_offset + 29] = '>';
    osd_buf[CAM_STATUS_SHARPNESS][osd_menu_offset + 19] = '<';
    osd_buf[CAM_STATUS_SHARPNESS][osd_menu_offset + 29] = '>';
    osd_buf[CAM_STATUS_CONTRAST][osd_menu_offset + 19] = '<';
    osd_buf[CAM_STATUS_CONTRAST][osd_menu_offset + 29] = '>';
    osd_buf[CAM_STATUS_SATURATION][osd_menu_offset + 19] = '<';
    osd_buf[CAM_STATUS_SATURATION][osd_menu_offset + 29] = '>';
    osd_buf[CAM_STATUS_WBMODE][osd_menu_offset + 19] = '<';
    osd_buf[CAM_STATUS_WBMODE][osd_menu_offset + 29] = '>';
    if (camCfg_Menu.wbMode == 0) {
        osd_buf[CAM_STATUS_WBRED][osd_menu_offset + 19] = ' ';
        osd_buf[CAM_STATUS_WBRED][osd_menu_offset + 29] = ' ';
        osd_buf[CAM_STATUS_WBBLUE][osd_menu_offset + 19] = ' ';
        osd_buf[CAM_STATUS_WBBLUE][osd_menu_offset + 29] = ' ';
    } else {
        osd_buf[CAM_STATUS_WBRED][osd_menu_offset + 19] = '<';
        osd_buf[CAM_STATUS_WBRED][osd_menu_offset + 29] = '>';
        osd_buf[CAM_STATUS_WBBLUE][osd_menu_offset + 19] = '<';
        osd_buf[CAM_STATUS_WBBLUE][osd_menu_offset + 29] = '>';
    }
    if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_MICRO_V3) {
        osd_buf[CAM_STATUS_HVFLIP][osd_menu_offset + 19] = '<';
        osd_buf[CAM_STATUS_HVFLIP][osd_menu_offset + 29] = '>';
        osd_buf[CAM_STATUS_NIGHTMODE][osd_menu_offset + 19] = '<';
        osd_buf[CAM_STATUS_NIGHTMODE][osd_menu_offset + 29] = '>';
    }
#endif
}

void camMenuInit() {
#if (0)
    uint8_t i = 0;

    memset(osd_buf, 0x20, sizeof(osd_buf));
    disp_mode = DISPLAY_CMS;
    if (camera_type == 0)
        Cam_Button_ENTER();
    else {
        strcpy(osd_buf[i++] + osd_menu_offset + 2, "----CAMERA MENU----");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " PROFILE");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " BRIGHTNESS");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " SHARPNESS");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " CONTRAST");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " SATURATION");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " WB MODE");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " WB RED");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " WB BLUE");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " HV FLIP");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " MAX GAIN");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " ASPECT RATIO");
        if (camera_type == RUNCAM_MICRO_V1)
            strcpy(osd_buf[i - 1] + osd_menu_offset + 19, "NOT SUPPORT");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " RESET");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " EXIT");
        strcpy(osd_buf[i++] + osd_menu_offset + 2, " SAVE&EXIT");

        camMenuDrawBracket();
        if (camera_type == RUNCAM_MICRO_V1)
            camMenuStringUpdate(CAM_STATUS_BRIGHTNESS);
        else
            camMenuStringUpdate(CAM_STATUS_PROFILE);
    }
#endif
}

void camMenuSetVdoRatioInit() {
#if (0)
    memset(osd_buf, 0x20, sizeof(osd_buf));
    strcpy(osd_buf[0] + osd_menu_offset + 2, "--VIDEO FORMAT--");
    strcpy(osd_buf[1] + osd_menu_offset + 3, "SET 720P 4:3  DEFAULT");
    strcpy(osd_buf[2] + osd_menu_offset + 3, "SET 720P 16:9 CROP");
    strcpy(osd_buf[3] + osd_menu_offset + 3, "SET 720P 16:9 FULL");
    strcpy(osd_buf[4] + osd_menu_offset + 3, "RETURN");
    osd_buf[1][osd_menu_offset + 2] = '>';
#endif
}

const char *cam_names[] = {"   MICRO V2", "    NANO V2", "  NANO LITE"};
const char *low_med_high_strs[] = {"   LOW", "MEDIUM", "  HIGH"};
const char *off_on_strs[] = {"  OFF", "   ON"};

void camMenuStringUpdate(uint8_t status) {
}