#include "runcam.h"
#include "camera.h"
#include "common.h"
#include "global.h"
#include "i2c.h"

const uint8_t runcam_micro_v1_attribute[CAMERA_SETTING_NUM][4] = {
    // brightness
    {1, 0x40, 0xC0, 0x80},
    // sharpness
    {1, 0x00, 0x02, 0x01},
    // contrast
    {1, 0x00, 0x02, 0x01},
    // saturation
    {1, 0x00, 0x06, 0x03},
    // wb mode
    {1, 0x00, 0x03, 0x00},
    // wb red[0]
    {1, 0x00, 0xff, 0xc7},
    // wb blue[0]
    {1, 0x00, 0xff, 0xca},
    // wb red[1]
    {1, 0x00, 0xff, 0xc7},
    // wb blue[1]
    {1, 0x00, 0xff, 0xca},
    // wb red[2]
    {1, 0x00, 0xff, 0xc7},
    // wb blue[2]
    {1, 0x00, 0xff, 0xca},
    // hv flip
    {0, 0x00, 0x01, 0x00},
    // night mode
    {0, 0x00, 0x00, 0x00},
    // video fmt
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
    // wb mode
    {1, 0x00, 0x03, 0x00},
    // wb red[0]
    {1, 0x00, 0xff, 0xc7},
    // wb blue[0]
    {1, 0x00, 0xff, 0xca},
    // wb red[1]
    {1, 0x00, 0xff, 0xc7},
    // wb blue[1]
    {1, 0x00, 0xff, 0xca},
    // wb red[2]
    {1, 0x00, 0xff, 0xc7},
    // wb blue[2]
    {1, 0x00, 0xff, 0xca},
    // hv flip
    {1, 0x00, 0x01, 0x00},
    // night mode
    {1, 0x00, 0x01, 0x00},
    // video fmt
    {1, 0x00, 0x02, 0x00},

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
    // wb mode
    {1, 0x00, 0x03, 0x00},
    // wb red[0]
    {1, 0x00, 0xff, 0xc7},
    // wb blue[0]
    {1, 0x00, 0xff, 0xca},
    // wb red[1]
    {1, 0x00, 0xff, 0xc7},
    // wb blue[1]
    {1, 0x00, 0xff, 0xca},
    // wb red[2]
    {1, 0x00, 0xff, 0xc7},
    // wb blue[2]
    {1, 0x00, 0xff, 0xca},
    // hv flip
    {1, 0x00, 0x01, 0x00},
    // night mode
    {1, 0x00, 0x01, 0x00},
    // video fmt
    {1, 0x00, 0x02, 0x00},

    {0, 0x00, 0x00, 0x00},
    {0, 0x00, 0x00, 0x00},
};

static uint8_t runcam_attribute[CAMERA_SETTING_NUM][4];
static uint8_t camera_device;

void runcam_type_detect(void) {
    uint8_t i, j;

    if (!RUNCAM_Write(RUNCAM_MICRO_V1, 0x50, 0x0452484E)) {
        camera_type = CAMERA_TYPE_RUNCAM_MICRO_V1;
        camera_device = RUNCAM_MICRO_V1;
        for (i = 0; i < CAMERA_SETTING_NUM; i++) {
            for (j = 0; j < 4; j++)
                runcam_attribute[i][j] = runcam_micro_v1_attribute[i][j];
        }
    } else if (!RUNCAM_Write(RUNCAM_MICRO_V2, 0x50, 0x0452484E)) {
        camera_type = CAMERA_TYPE_RUNCAM_MICRO_V2;
        camera_device = RUNCAM_MICRO_V2;
        for (i = 0; i < CAMERA_SETTING_NUM; i++) {
            for (j = 0; j < 4; j++)
                runcam_attribute[i][j] = runcam_micro_v2_attribute[i][j];
        }
    } else if (!RUNCAM_Write(RUNCAM_NANO_90, 0x50, 0x04484848)) {
        camera_type = CAMERA_TYPE_RUNCAM_NANO_90;
        camera_device = RUNCAM_NANO_90;
        for (i = 0; i < CAMERA_SETTING_NUM; i++) {
            for (j = 0; j < 4; j++)
                runcam_attribute[i][j] = runcam_nano_90_attribute[i][j];
        }
    }
}

void runcam_setting_profile_reset(uint8_t *setting_profile) {
    uint8_t i;
    for (i = 0; i < CAMERA_SETTING_NUM; i++)
        setting_profile[i] = runcam_attribute[i][item_default];
}

uint8_t runcam_setting_profile_check(uint8_t *setting_profile) {
    uint8_t i;
    for (i = 0; i < CAMERA_SETTING_NUM; i++) {
        if (runcam_micro_v1_attribute[i][item_enbale]) {
            if (setting_profile[i] < runcam_attribute[i][item_min])
                return 1;
            if (setting_profile[i] > runcam_attribute[i][item_max])
                return 1;
        }
    }
    return 0;
}

void runcam_brightness(uint8_t val) {
    uint32_t d = 0x04004800;
    uint32_t val_32;

    if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V1)
        d = 0x0452484e;
    else if (camera_type == CAMERA_TYPE_RUNCAM_MICRO_V2 || camera_type == CAMERA_TYPE_RUNCAM_NANO_90)
        d = 0x04504850;

    val_32 = (uint32_t)val;

    // indoor
    d += val_32;
    d -= runcam_attribute[0][item_default];

    // outdoor
    d += (val_32 << 16);
    d -= ((uint32_t)runcam_attribute[0][item_default] << 16);

    RUNCAM_Write(camera_device, 0x50, d);
}

void runcam_set(uint8_t *setting_profile, uint8_t is_init) {
    static uint8_t setting_cur[CAMERA_SETTING_NUM] = {0};

    if (is_init || (setting_cur[0] != setting_profile[0])) {
        runcam_brightness(setting_profile[0]);
        setting_cur[0] = setting_profile[0];
    }
}