#include "runcam.h"
#include "camera.h"

uint8_t runcam_detect_type(void) {
    uint8_t type = 0;

    if (!RUNCAM_Write(RUNCAM_MICRO_V1, 0x50, 0x0452484E))
        type = RUNCAM_MICRO_V1;
    else if (!RUNCAM_Write(RUNCAM_MICRO_V2, 0x50, 0x0452484E))
        type = RUNCAM_MICRO_V2;
    else if (!RUNCAM_Write(RUNCAM_MICRO_V3, 0x50, 0x04484848))
        type = RUNCAM_MICRO_V3;
    else
        type = 0;

    return type;
}