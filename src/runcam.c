#include "runcam.h"
#include "camera.h"
#include "common.h"
#include "global.h"
#include "i2c.h"
#include "print.h"

const uint8_t runcam_micro_v1_attribute[CAMERA_SETTING_NUM][4] = {
    // brightness
    {1, 0x40, 0xC0, 0x80},
    // sharpness
    {1, 0x00, 0x02, 0x01},
    // contrast
    {1, 0x00, 0x02, 0x01},
    // saturation
    {1, 0x00, 0x06, 0x03},
    // shutter speed
    {0, 0x00, 0x20, 0x00},
    // wb mode
    {1, 0x00, 0x01, 0x00},
    // wb red
    {1, 0x00, 0xff, 0xc7},
    // wb blue
    {1, 0x00, 0xff, 0xca},
    // hv flip
    {0, 0x00, 0x01, 0x00},
    // night mode
    {0, 0x00, 0x01, 0x01},
    // led mode
    {1, 0x00, 0x01, 0x00},
    // video fmt
    {0, 0x00, 0x00, 0x00},

    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
};

const uint8_t runcam_micro_v2_attribute[CAMERA_SETTING_NUM][4] = {
    // brightness
    {1, 0x40, 0xC0, 0x80},
    // sharpness
    {1, 0x00, 0x02, 0x01},
    // contrast
    {1, 0x00, 0x02, 0x01},
    // saturation
    {1, 0x00, 0x06, 0x05},
    // shutter speed
    {1, 0x00, 0x20, 0x00},
    // wb mode
    {1, 0x00, 0x01, 0x00},
    // wb red
    {1, 0x00, 0xff, 0xc7},
    // wb blue
    {1, 0x00, 0xff, 0xca},
    // hv flip
    {1, 0x00, 0x01, 0x00},
    // night mode
    {1, 0x00, 0x01, 0x01},
    // led mode
    {1, 0x00, 0x01, 0x00},
    // video fmt
    {1, 0x00, 0x02, 0x00},

    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
};

const uint8_t runcam_nano_90_attribute[CAMERA_SETTING_NUM][4] = {
    // brightness
    {1, 0x40, 0xC0, 0x80},
    // sharpness
    {1, 0x00, 0x02, 0x01},
    // contrast
    {1, 0x00, 0x02, 0x01},
    // saturation
    {1, 0x00, 0x06, 0x05},
    // shutter speed
    {1, 0x00, 0x20, 0x00},
    // wb mode
    {1, 0x00, 0x01, 0x00},
    // wb red
    {1, 0x00, 0xff, 0xc7},
    // wb blue
    {1, 0x00, 0xff, 0xca},
    // hv flip
    {1, 0x00, 0x01, 0x00},
    // night mode
    {1, 0x00, 0x01, 0x01},
    // led mode
    {1, 0x00, 0x01, 0x00},
    // video fmt
    {1, 0x00, 0x03, 0x00},

    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
};

void runcam_type_detect(void) {
    uint8_t i, j;

    if (!RUNCAM_Write(RUNCAM_MICRO_V1, 0x50, 0x0452484E)) {
        camera_type = CAMERA_TYPE_RUNCAM_MICRO_V1;
        camera_device = RUNCAM_MICRO_V1;
        for (i = 0; i < CAMERA_SETTING_NUM; i++) {
            for (j = 0; j < 4; j++)
                camera_attribute[i][j] = runcam_micro_v1_attribute[i][j];
        }
    } else if (!RUNCAM_Write(RUNCAM_MICRO_V2, 0x50, 0x0452484E)) {
        camera_type = CAMERA_TYPE_RUNCAM_MICRO_V2;
        camera_device = RUNCAM_MICRO_V2;
        for (i = 0; i < CAMERA_SETTING_NUM; i++) {
            for (j = 0; j < 4; j++)
                camera_attribute[i][j] = runcam_micro_v2_attribute[i][j];
        }
    } else if (!RUNCAM_Write(RUNCAM_NANO_90, 0x50, 0x04484848)) {
        camera_type = CAMERA_TYPE_RUNCAM_NANO_90;
        camera_device = RUNCAM_NANO_90;
        for (i = 0; i < CAMERA_SETTING_NUM; i++) {
            for (j = 0; j < 4; j++)
                camera_attribute[i][j] = runcam_nano_90_attribute[i][j];
        }
    }
}

void runcam_setting_profile_reset(uint8_t *setting_profile) {
    uint8_t i;
    for (i = 0; i < CAMERA_SETTING_NUM; i++)
        setting_profile[i] = camera_attribute[i][CAM_SETTING_ITEM_DEFAULT];
}

uint8_t runcam_setting_profile_check(uint8_t *setting_profile) {
    uint8_t i;
    for (i = 0; i < CAMERA_SETTING_NUM; i++) {
        if (camera_attribute[i][CAM_SETTING_ITEM_ENBALE]) {
            if (setting_profile[i] < camera_attribute[i][CAM_SETTING_ITEM_MIN])
                return 1;
            if (setting_profile[i] > camera_attribute[i][CAM_SETTING_ITEM_MAX])
                return 1;
        }
    }
    return 0;
}

void runcam_brightness(uint8_t val, uint8_t led_mode) {
    uint32_t d;
    uint32_t val_32;

    camera_setting_reg_set[0] = val;
    camera_setting_reg_set[10] = led_mode;

    if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V1)
        d = 0x0452004e;
    else if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V2)
        d = 0x04500050;
    else // if (camera_type == CAMERA_TYPE_RUNCAM_NANO_90)
        d = 0x04480048;

    if (led_mode)
        d |= 0x00002800;
    else
        d |= 0x00004800;

    val_32 = (uint32_t)val;

    // indoor
    d += val_32;

    d -= camera_attribute[0][CAM_SETTING_ITEM_DEFAULT];
    // outdoor
    d += (val_32 << 16);
    d -= ((uint32_t)camera_attribute[0][CAM_SETTING_ITEM_DEFAULT] << 16);

    RUNCAM_Write(camera_device, 0x50, d);
#ifdef _DEBUG_RUNCAM
    debugf("\r\nRUNCAM brightness:0x%02x, led_mode:%02x", (uint16_t)val, (uint16_t)led_mode);
#endif
}

void runcam_sharpness(uint8_t val) {
    uint32_t d;

    camera_setting_reg_set[1] = val;

    if (val == 0) {
        if (camera_type == RUNCAM_MICRO_V1)
            d = 0x03FF0100;
        else // if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_NANO_90)
            d = 0x03FF0000;
        RUNCAM_Write(camera_device, 0x0003C4, d);
        RUNCAM_Write(camera_device, 0x0003CC, 0x0A0C0E10);
        RUNCAM_Write(camera_device, 0x0003D8, 0x0A0C0E10);
    } else if (val == 1) {
        RUNCAM_Write(camera_device, 0x0003C4, 0x03FF0000);
        RUNCAM_Write(camera_device, 0x0003CC, 0x14181C20);
        RUNCAM_Write(camera_device, 0x0003D8, 0x14181C20);
    } else if (val == 2) {
        RUNCAM_Write(camera_device, 0x0003C4, 0x03FF0000);
        RUNCAM_Write(camera_device, 0x0003CC, 0x28303840);
        RUNCAM_Write(camera_device, 0x0003D8, 0x28303840);
    }
#ifdef _DEBUG_RUNCAM
    debugf("\r\nRUNCAM sharpness:0x%02x", (uint16_t)val);
#endif
}

void runcam_contrast(uint8_t val) {
    uint32_t d;

    camera_setting_reg_set[2] = val;

    if (camera_type == RUNCAM_MICRO_V1)
        d = 0x46484A4C;
    else // if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_NANO_90)
        d = 0x36383a3c;

    if (val == 0) // low
        d -= 0x06040404;
    else if (val == 1) // normal
        ;
    else if (val == 2) // high
        d += 0x04040404;

    RUNCAM_Write(camera_device, 0x00038C, d);
#ifdef _DEBUG_RUNCAM
    debugf("\r\nRUNCAM contrast:%02x", (uint16_t)val);
#endif
}

uint8_t runcam_saturation(uint8_t val) {
    uint8_t ret = 1;
    uint32_t d;

    camera_setting_reg_set[3] = val;

    // initial
    if (camera_type == RUNCAM_MICRO_V1)
        d = 0x20242626;
    else // if (camera_type == RUNCAM_MICRO_V2 || camera_type == RUNCAM_NANO_90)
        d = 0x24282c30;

    if (val == 0)
        d = 0x00000000;
    else if (val == 1)
        d -= 0x18181414;
    else if (val == 2)
        d -= 0x14141010;
    else if (val == 3)
        d -= 0x0c0c0808;
    else if (val == 4)
        d -= 0x08080404;
    else if (val == 5)
        d += 0x00000000;
    else if (val == 6)
        d += 0x04041418;

    ret = RUNCAM_Write(camera_device, 0x0003A4, d);

#ifdef _DEBUG_RUNCAM
    debugf("\r\nRUNCAM saturation:%02x", (uint16_t)val);
#endif
    return ret;
}

void runcam_wb(uint8_t wbMode, uint8_t wbRed, uint8_t wbBlue) {
    uint32_t wbRed_u32 = 0x02000000;
    uint32_t wbBlue_u32 = 0x00000000;

    camera_setting_reg_set[5] = wbMode;
    camera_setting_reg_set[6] = wbRed;
    camera_setting_reg_set[7] = wbBlue;

    if (wbMode) {
        wbRed_u32 += ((uint32_t)wbRed << 2);
        wbBlue_u32 += ((uint32_t)wbBlue << 2);
        wbRed_u32++;
        wbBlue_u32++;
    }

    if (wbMode) { // MWB
        RUNCAM_Write(camera_device, 0x0001b8, 0x020b007b);
        RUNCAM_Write(camera_device, 0x000204, wbRed_u32);
        RUNCAM_Write(camera_device, 0x000208, wbBlue_u32);
    } else { // AWB
        RUNCAM_Write(camera_device, 0x0001b8, 0x020b0079);
    }
#ifdef _DEBUG_RUNCAM
    debugf("\r\nRUNCAM wb:red(%02x),blue(%02x),mode(%02x)",
           (uint16_t)wbRed, (uint16_t)wbBlue, (uint16_t)wbMode);
#endif
}

void runcam_hv_flip(uint8_t val) {
    if (camera_type != CAMERA_TYPE_RUNCAM_MICRO_V2 && camera_type != CAMERA_TYPE_RUNCAM_NANO_90)
        return;

    camera_setting_reg_set[8] = val;

    if (val == 0)
        RUNCAM_Write(camera_device, 0x000040, 0x0022ffa9);
    else if (val == 1)
        RUNCAM_Write(camera_device, 0x000040, 0x002effa9);
#ifdef _DEBUG_RUNCAM
    debugf("\r\nRUNCAM hvFlip:%02x", (uint16_t)val);
#endif
}

void runcam_night_mode(uint8_t val) {
    /*
        0: night mode off
        1: night mode on
    */
    if (camera_type != CAMERA_TYPE_RUNCAM_MICRO_V2 && camera_type != CAMERA_TYPE_RUNCAM_NANO_90)
        return;

    camera_setting_reg_set[9] = val;

    if (val == 0) { // Max gain off
        RUNCAM_Write(camera_device, 0x000070, 0x10000040);
        RUNCAM_Write(camera_device, 0x000718, 0x30002900);
        RUNCAM_Write(camera_device, 0x00071c, 0x32003100);
        RUNCAM_Write(camera_device, 0x000720, 0x34003300);
    } else if (val == 1) { // Max gain on
        RUNCAM_Write(camera_device, 0x000070, 0x10000040);
        RUNCAM_Write(camera_device, 0x000718, 0x28002700);
        RUNCAM_Write(camera_device, 0x00071c, 0x29002800);
        RUNCAM_Write(camera_device, 0x000720, 0x29002900);
    }
#ifdef _DEBUG_RUNCAM
    debugf("\r\nRUNCAM nightMode:%02x", (uint16_t)val);
#endif
}

void runcam_video_format(uint8_t val) {
    /*
    RUNCAM_MICRO_V2:
        0: 960x720@60
        1: 1280x720@60 crop
        2: 1280x720@60 full

    RUNCAM_NANO_90:
        0: 720x540@90
        1: 720x540@90 crop
        2: 720x540@60
        3: 960x720@60
    */
    camera_setting_reg_set[11] = val;

    if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V2) {
        if (val == 0)
            RUNCAM_Write(camera_device, 0x000008, 0x0008910B);
        else if (val == 1)
            RUNCAM_Write(camera_device, 0x000008, 0x00089102);
        else // if (val == 2)
            RUNCAM_Write(camera_device, 0x000008, 0x00089110);
    } else if (camera_type == CAMERA_TYPE_RUNCAM_NANO_90) {
        if (val == 0)
            RUNCAM_Write(camera_device, 0x000008, 0x8008811d);
#if (0) // Not yet supported
        else if (val == 1)
            RUNCAM_Write(camera_device, 0x000008, 0x83088120);
        else if (val == 2)
            RUNCAM_Write(camera_device, 0x000008, 0x8108811e);
        else // if (val == 3)
            RUNCAM_Write(camera_device, 0x000008, 0x8208811f);
#endif
    }
#ifdef _DEBUG_RUNCAM
    debugf("\r\nRUNCAM video format:%02x", (uint16_t)val);
#endif
}

void runcam_shutter(uint8_t val) {
    uint32_t dat = 0;
    camera_setting_reg_set[4] = val;

    if (val == 0) { // auto
        if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V2)
            dat = 0x460;
        else if (camera_type == CAMERA_TYPE_RUNCAM_NANO_90)
            dat = 0x447;
    } else // manual
        dat = (uint32_t)(val)*25;
    RUNCAM_Write(camera_device, 0x00006c, dat);
    RUNCAM_Write(camera_device, 0x000044, 0x80009629);
}

uint8_t runcam_setting_update_need(uint8_t *setting_p, uint8_t start, uint8_t stop) {
    uint8_t i;
    for (i = start; i <= stop; i++) {
        if (camera_setting_reg_set[i] != setting_p[i])
            return 1;
    }
    return 0;
}

void runcam_save(void) {
    RUNCAM_Write(camera_device, 0x000694, 0x00000310);
    RUNCAM_Write(camera_device, 0x000694, 0x00000311);
#ifdef _DEBUG_RUNCAM
    debugf("\r\nRUNCAM Save");
#endif
}

uint8_t runcam_set(uint8_t *setting_profile) {
    static uint8_t init_done = 0;
    uint8_t ret = 0;
    if (!init_done || runcam_setting_update_need(setting_profile, 0, 0) || runcam_setting_update_need(setting_profile, 10, 10))
        runcam_brightness(setting_profile[0], setting_profile[10]); // include led_mode

    if (!init_done || runcam_setting_update_need(setting_profile, 1, 1))
        runcam_sharpness(setting_profile[1]);

    if (!init_done || runcam_setting_update_need(setting_profile, 2, 2))
        runcam_contrast(setting_profile[2]);

    if (!init_done || runcam_setting_update_need(setting_profile, 3, 3))
        runcam_saturation(setting_profile[3]);

    if (!init_done || runcam_setting_update_need(setting_profile, 4, 4))
        runcam_shutter(setting_profile[4]);

    if (!init_done || runcam_setting_update_need(setting_profile, 5, 7))
        runcam_wb(setting_profile[5], setting_profile[6], setting_profile[7]);

    if (!init_done || runcam_setting_update_need(setting_profile, 8, 8))
        runcam_hv_flip(setting_profile[8]);

    if (!init_done || runcam_setting_update_need(setting_profile, 9, 9))
        runcam_night_mode(setting_profile[9]);

    if (!init_done || runcam_setting_update_need(setting_profile, 11, 11)) {
        runcam_video_format(setting_profile[11]);
        ret = 1;
    }
    if (!init_done)
        init_done = 1;
    return ret;
}