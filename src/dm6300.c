#include "dm6300.h"

#include "common.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "i2c_device.h"
#include "monitor.h"
#include "print.h"
#include "spi.h"

typedef struct {
    uint8_t trans;
    uint16_t addr;
    uint32_t dat;
} dm6300_reg_value_t;

int16_t auxadc_offset = 0;
uint32_t init6300_fcnt = 0;
uint32_t init6300_fnum[FREQ_NUM_EXTERNAL] = {0};

uint32_t dcoc_ih = 0x075F0000;
uint32_t dcoc_qh = 0x075F0000;

uint8_t dm6300_init_done = 0;
uint8_t dm6300_lost = 0;
#if defined HDZERO_FREESTYLE_V1
uint8_t table_power[FREQ_NUM_EXTERNAL][POWER_MAX + 1] = {
    // race band
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x60, 0x60},
    {0x72, 0x6d, 0x60, 0x60},
    {0x74, 0x70, 0x62, 0x5c},
    {0x78, 0x74, 0x64, 0x5b},
    {0x7a, 0x77, 0x64, 0x5b},
    {0x7a, 0x77, 0x64, 0x5b},
    // e band
    {0x70, 0x68, 0x5c, 0x60}, // E1
    // fatshark band
    {0x70, 0x68, 0x60, 0x60}, // F1
    {0x72, 0x6d, 0x60, 0x60}, // F2
    {0x74, 0x70, 0x62, 0x5c}, // F4
    // low band
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
};
#elif defined HDZERO_FREESTYLE_V2
uint8_t table_power[FREQ_NUM_EXTERNAL][POWER_MAX + 1] = {
    // race band
    {0x30, 0x50, 0x50, 0x58},
    {0x30, 0x50, 0x50, 0x58},
    {0x30, 0x50, 0x50, 0x58},
    {0x30, 0x50, 0x50, 0x58},
    {0x30, 0x50, 0x50, 0x58},
    {0x30, 0x50, 0x50, 0x58},
    {0x30, 0x50, 0x50, 0x58},
    {0x30, 0x50, 0x50, 0x58},
    // e band
    {0x70, 0x68, 0x5c, 0x60}, // E1
    // fatshark band
    {0x70, 0x68, 0x60, 0x60}, // F1
    {0x72, 0x6d, 0x60, 0x60}, // F2
    {0x74, 0x70, 0x62, 0x5c}, // F4
    // low band
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
    {0x70, 0x68, 0x5c, 0x60},
};
#else
uint8_t table_power[FREQ_NUM_EXTERNAL][POWER_MAX + 1] = {
    // race band
    {0x50, 0x60},
    {0x50, 0x60},
    {0x50, 0x60},
    {0x50, 0x60},
    {0x50, 0x60},
    {0x50, 0x60},
    {0x50, 0x60},
    {0x50, 0x60},
    // e band
    {0x79, 0x83}, // E1
    // fatshark band
    {0x75, 0x80}, // F1
    {0x73, 0x7E}, // F2
    {0x72, 0x7C}, // F4
    // low band
    {0x79, 0x83},
    {0x79, 0x83},
    {0x79, 0x83},
    {0x79, 0x83},
    {0x79, 0x83},
    {0x79, 0x83},
    {0x79, 0x83},
    {0x79, 0x83},
};
#endif

const uint32_t tab[3][FREQ_NUM_EXTERNAL] = {
    {
        // race band
        0x3867,
        0x379D,
        0x3924,
        0x3982,
        0x39E1,
        0x3A3F,
        0x3A9E,
        0x3AFC,

        // E band
        0x38DF, // E1

        // fatshark bank
        0x3938, // F1
        0x3840, // F2
        0x38A4, // F4

        // low band
        0x3574,
        0x35D2,
        0x3631,
        0x368F,
        0x36ED,
        0x374C,
        0x37AA,
        0x3809,
    },
    {
        // race band
        0x93,
        0x94,
        0x95,
        0x96,
        0x97,
        0x98,
        0x99,
        0x9a,

        // E band
        0x94, // E1

        // fatshark bank
        0x95, // F1
        0x96, // F2
        0x97, // F4

        // low band
        0x8B,
        0x8C,
        0x8D,
        0x8E,
        0x8F,
        0x90,
        0x91,
        0x92,
    },
    {
        // race band
        0xB00000,
        0x9D5555,
        0x8AAAAB,
        0x780000,
        0x655555,
        0x52AAAB,
        0x400000,
        0x2D5555,
        // E band
        0x122AAAB, // E1

        // fatshark bank
        0xF55555, // F1
        0x000000, // F2
        0x155555, // F4

        // low band
        0x1455555,
        0x132AAAB,
        0x1200000,
        0x10D5555,
        0xFAAAAB,
        0xE80000,
        0xD55555,
        0xC2AAAB,
    },
};

const uint32_t freq_tab[FREQ_NUM_EXTERNAL] = {
    // race band
    113200,
    113900,
    114700,
    115400,
    116100,
    116780,
    117560,
    118280,

    // e band
    114100, // E1

    // fatshark bank
    114800, // F1
    115200, // F2
    116000, // F4

    // low band
    107240,
    107980,
    108720,
    109640,
    110200,
    110940,
    111680,
    112420,
};

const uint16_t frequencies[] = {
    FREQ_R1,
    FREQ_R2,
    FREQ_R3,
    FREQ_R4,
    FREQ_R5,
    FREQ_R6,
    FREQ_R7,
    FREQ_R8,
    FREQ_E1,
    FREQ_F1,
    FREQ_F2,
    FREQ_F4,
    FREQ_L1,
    FREQ_L2,
    FREQ_L3,
    FREQ_L4,
    FREQ_L5,
    FREQ_L6,
    FREQ_L7,
    FREQ_L8,
};

void DM6300_write_reg_map(const dm6300_reg_value_t *reg_map, uint8_t size) {
    uint8_t i = 0;
    for (i = 0; i < size; i++) {
        // debugf("\r\nDM6300_write_reg_map: %bx %x %lx", reg_map[i].trans, reg_map[i].addr, reg_map[i].dat);
        SPI_Write(reg_map[i].trans, reg_map[i].addr, reg_map[i].dat);
    }
}

#define REG_MAP_COUNT(regs) sizeof(regs) / sizeof(dm6300_reg_value_t)
#define WRITE_REG_MAP(regs) DM6300_write_reg_map(regs, REG_MAP_COUNT(regs))

dm6300_reg_value_t dm6300_set_channel_regs[] = {
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x2B0, 0x00077777},
    {0x3, 0x030, 0x00000013},
    {0x3, 0x034, 0x00000013},
    {0x3, 0x038, 0x00000370},
    {0x3, 0x03C, 0x00000410},
    {0x3, 0x040, 0x00000000},
    {0x3, 0x044, 0x0D640735},
    {0x3, 0x048, 0x01017F03},
    {0x3, 0x04C, 0x021288A2},
    {0x3, 0x050, 0x00FFCF33},
    {0x3, 0x054, 0x1F3C3440},
    {0x3, 0x028, 0x00008000}, // [12] // 0x00008000 + (init6300_fcnt & 0xFF)
    {0x3, 0x020, 0x00000000}, // [13] // init6300_fnum[ch]
    {0x3, 0x01C, 0x00000002},
    {0x3, 0x018, 0x00000001}, // WAIT(1},
    {0x3, 0x018, 0x00000000}, // WAIT(1},
    {0x3, 0x028, 0x00000000}, // [17] // 0x00008000 + (init6300_fcnt & 0xFF)
    {0x3, 0x020, 0x00000000}, // [18] // init6300_fnum[ch]},
    {0x3, 0x01C, 0x00000003},
    {0x3, 0x018, 0x00000001}, // WAIT(1},
    {0x3, 0x018, 0x00000000}, // WAIT(1},
    {0x3, 0x050, 0x00FFCFB3},
    {0x3, 0x004, 0x00000000}, // [23] // tab[1][ch]
    {0x3, 0x008, 0x00000000}, // [24] // tab[2][ch]
    {0x3, 0x000, 0x00000000}, // WAIT(1},
    {0x3, 0x000, 0x00000003},
    {0x3, 0x050, 0x000333B3},
    {0x3, 0x040, 0x07070002},
    {0x3, 0x030, 0x00000010},
};

void DM6300_SetChannel(uint8_t ch) {
#ifdef _DEBUG_MODE
    debugf("\r\nset ch:%x", (uint16_t)ch);
#endif

    if (ch >= FREQ_NUM)
        ch = 0;

    dm6300_set_channel_regs[12].dat = 0x00008000 + (init6300_fcnt & 0xFF);
    dm6300_set_channel_regs[13].dat = init6300_fnum[ch];

    dm6300_set_channel_regs[17].dat = 0x00008000 + (init6300_fcnt & 0xFF);
    dm6300_set_channel_regs[18].dat = init6300_fnum[ch];

    dm6300_set_channel_regs[23].dat = tab[1][ch];
    dm6300_set_channel_regs[24].dat = tab[2][ch];

    WRITE_REG_MAP(dm6300_set_channel_regs);
}

uint8_t DM6300_GetChannelByFreq(uint16_t const freq) {
    uint8_t iter;
    for (iter = 0; iter < ARRAY_SIZE(frequencies); iter++) {
        if (frequencies[iter] == freq) {
            return iter;
        }
    }
    return INVALID_CHANNEL;
}

uint16_t DM6300_GetFreqByChannel(uint8_t const ch) {
    if (ch < ARRAY_SIZE(frequencies)) {
        return frequencies[ch];
    }
    return 0;
}

void DM6300_SetPower(uint8_t pwr, uint8_t freq, uint8_t offset) {
#ifdef HDZERO_FREESTYLE_V1
    uint16_t a_tab[4] = {0x204, 0x11F, 0x21F, 0x31F};
#elif defined HDZERO_FREESTYLE_V2
    uint16_t a_tab[4] = {0x21F, 0x21F, 0x31F, 0x31F};
#else
    uint16_t a_tab[2] = {0x21F, 0x41F};
#endif
    int16_t p;
#ifdef _DEBUG_MODE
    debugf("\r\nDM6300 set power:%x, offset:%x", (uint16_t)pwr, (uint16_t)offset);
#endif
    if (freq >= FREQ_NUM)
        freq = 0;
    SPI_Write(0x6, 0xFF0, 0x00000018);

    if (pwr == POWER_MAX + 1) {
        SPI_Write(0x3, 0x330, 0x21F);
        SPI_Write(0x3, 0xD1C, PIT_POWER);
    } else {
        SPI_Write(0x3, 0x330, a_tab[pwr]);

#ifndef _RF_CALIB
        if (RF_POWER == 0 && pwr == 0) {
            p = table_power[freq][pwr] + offset - 2;

            if (OFFSET_25MW <= 10)
                p += OFFSET_25MW;
            else if (p < (OFFSET_25MW - 10))
                p = 0;
            else
                p = p + 10 - OFFSET_25MW;
        } else
#endif
            p = table_power[freq][pwr] + offset - 2;

        if (p > 255)
            p = 255;
        else if (p < 0)
            p = 0;
        SPI_Write(0x3, 0xD1C, (uint8_t)p);
    }
}

void DM6300_SetSingleTone(uint8_t enable) {
    SPI_Write(0x6, 0xFF0, 0x00000019);

    if (enable) {
        SPI_Write(0x3, 0x08C, 0x18040000);
        SPI_Write(0x3, 0x0C0, 2);
        SPI_Write(0x3, 0x0BC, 0x5ED097B4);
    } else {
        SPI_Write(0x3, 0x08C, 0);
        SPI_Write(0x3, 0x0C0, 0);
        SPI_Write(0x3, 0x0BC, 0);
    }
}

void DM6300_RFTest() {
    uint16_t i;
    for (i = 0; i < 100; i++)
        SPI_Write(0x6, 0xFF0, 0x00000018);
}

void DM6300_InitAUXADC() {
    uint32_t dat;
    int16_t dat1, dat2, dat3;

    SPI_Write(0x6, 0xFF0, 0x00000018);
    SPI_Read(0x3, 0x254, &dat);
#ifdef _DEBUG_DM6300
    debugf("\r\nDM6300 0x254 = %x%x.\r\n", (uint16_t)((dat >> 16) & 0xFFFF), (uint16_t)(dat & 0xFFFF));
#endif
    dat |= 0x200;
#ifdef _DEBUG_DM6300
    debugf("\r\nDM6300 0x254 = %x%x.\r\n", (uint16_t)((dat >> 16) & 0xFFFF), (uint16_t)(dat & 0xFFFF));
#endif
    SPI_Write(0x3, 0x254, dat);

    SPI_Write(0x6, 0xFF0, 0x00000019);
    SPI_Write(0x3, 0x17C, 0x19);

    SPI_Write(0x6, 0xFF0, 0x00000018);
    SPI_Write(0x3, 0x2A0, 0xC05B55FE);
    SPI_Write(0x6, 0xFF0, 0x00000019);
#ifndef HDZERO_RACE_V2
    WAIT(1);
#endif
    SPI_Read(0x3, 0x17C, &dat);
    dat1 = ((int32_t)dat) >> 20;

    SPI_Write(0x6, 0xFF0, 0x00000018);
    SPI_Write(0x3, 0x2A0, 0x305B55FE);
    SPI_Write(0x6, 0xFF0, 0x00000019);
#ifndef HDZERO_RACE_V2
    WAIT(1);
#endif
    SPI_Read(0x3, 0x17C, &dat);
    dat2 = ((int32_t)dat) >> 20;

    SPI_Write(0x6, 0xFF0, 0x00000018);
    SPI_Write(0x3, 0x2A0, 0xA05B51FE);
    SPI_Write(0x6, 0xFF0, 0x00000019);
#ifndef HDZERO_RACE_V2
    WAIT(1);
#endif
    SPI_Read(0x3, 0x17C, &dat);
    dat3 = ((int32_t)dat) >> 20;

    auxadc_offset = dat3 - ((dat1 + dat2) >> 1);
#ifndef HDZERO_RACE_V2
    if (auxadc_offset < 0x420)
        auxadc_offset = 0x420;
#endif
#ifdef _DEBUG_DM6300
    debugf("\r\nDM6300 AUXADC Calib done. data1=%x, data2=%x, data3=%x, offset=%x", dat1, dat2, dat3, auxadc_offset);
#endif
}

void DM6300_AUXADC_Calib() {
    WriteReg(0, 0x8F, 0x01);
    DM6300_InitAUXADC();
    WriteReg(0, 0x8F, 0x11);
    dm6300_init_done = 1;
}
/*void DM6300_CalibRF()
{
    uint8_t i, j;
    uint32_t dh, dl;
    int16_t vol;

    DM6300_SetSingleTone(1);

    SPI_Write(0x6, 0xFF0, 0x00000018);
    SPI_Write(0x3, 0x2A0, 0xA05B45FE);

    for (i=0;i<8;i++){
        DM6300_SetChannel(i);
        DM6300_SetPower(0, i, 0);
        debugf("\r\nPA voltage detect: channel = %d,voltage =",(uint16_t)i);
        for(j=0;j<8;j++){
            SPI_Write(0x6, 0xFF0, 0x00000019);
            SPI_Read (0x3, 0x17C, &dl);
            vol = (((int32_t)dl) >> 20) + auxadc_offset;
            debugf("%x ", vol);
        }
        //WAIT(1000);
    }

    DM6300_SetSingleTone(0);
}*/

int16_t DM6300_GetTemp() {
    static uint8_t init = 1;
    uint32_t dat;
    int16_t temp;

    if (init) {
        init = 0;
        SPI_Write(0x6, 0xFF0, 0x00000018);
        SPI_Write(0x3, 0x2C0, 0x00000100);
        SPI_Write(0x3, 0x2A0, 0xA04005FE);
    }

    SPI_Write(0x6, 0xFF0, 0x00000019);
    WAIT(1);
    SPI_Read(0x3, 0x17C, &dat);
    temp = (((int32_t)dat) >> 20) + auxadc_offset;

    return temp;
}

CODE_SEG const dm6300_reg_value_t dm6300_init1_regs[] = {
    {0x6, 0xFFC, 0x00000000},
    {0x6, 0xFFC, 0x00000001},
    {0x6, 0x7FC, 0x00000000},
    {0x6, 0xF1C, 0x00000001},
    {0x6, 0xF20, 0x0000FCD0},
    {0x6, 0xF04, 0x00004741},
    {0x6, 0xF08, 0x00000083},
    {0x6, 0xF08, 0x000000C3},
    {0x6, 0xF40, 0x00000003},
    {0x6, 0xF40, 0x00000001},
    {0x6, 0xFF0, 0x00000018},
    {0x6, 0xFFC, 0x00000000},
    {0x6, 0xFFC, 0x00000001},
};

void DM6300_init1() {
    WRITE_REG_MAP(dm6300_init1_regs);
}

CODE_SEG const dm6300_reg_value_t dm6300_init2_regs_bw17[] = {
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x2B0, 0x00077777},
    {0x3, 0x230, 0x00000000},
    {0x3, 0x234, 0x10000000},
    {0x3, 0x238, 0x000000BF},
    {0x3, 0x23C, 0x55530610},
    {0x3, 0x240, 0x3FFC0047},
    {0x3, 0x244, 0x00188A13},
    {0x3, 0x248, 0x00000000},
    {0x3, 0x24C, 0x0A121707},
    {0x3, 0x250, 0x017F0001},
    {0x3, 0x228, 0x0000807C},
    {0x3, 0x220, 0x0000292C},
    {0x3, 0x21C, 0x00000002},
    {0x3, 0x218, 0x00000001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x228, 0x0000807C},
    {0x3, 0x220, 0x0000292C},
    {0x3, 0x21C, 0x00000003},
    {0x3, 0x218, 0x00000001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x244, 0x00188A17},
    {0x3, 0x204, 0x0000002A},
    {0x3, 0x208, 0x00400000},
    {0x3, 0x200, 0x00000000},
    {0x3, 0x200, 0x00000003},
    {0x3, 0x240, 0x00030041},
    {0x3, 0x248, 0x00000404},
    {0x3, 0x258, 0x00010003},
    {0x3, 0x254, 0x00057A17}};
CODE_SEG const dm6300_reg_value_t dm6300_init2_regs_bw27[] = {
    // 02_BBPLL_3456
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x2AC, 0x00000300},
    {0x3, 0x2B0, 0x00077777},
    {0x3, 0x230, 0x00000000},
    {0x3, 0x234, 0x10000000},
    {0x3, 0x238, 0x000000BF},
    {0x3, 0x23C, 0x17530610}, // 0x15840410  0x15530610
    {0x3, 0x240, 0x3FFC0047}, // 0x0001C047  0x3FFC0047
    {0x3, 0x244, 0x00188A13}, // 0x00188A17
    {0x3, 0x248, 0x00000000},
    {0x3, 0x24C, 0x0A161707}, // 0x08021707
    {0x3, 0x250, 0x017F0001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x228, 0x00008083},
    {0x3, 0x220, 0x00002E0E}, // 0x00002666
    {0x3, 0x21C, 0x00000002},
    {0x3, 0x218, 0x00000001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x21C, 0x00000003},
    {0x3, 0x218, 0x00000001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x244, 0x00188A17},
    {0x3, 0x204, 0x0000002D}, // 0x0000004c
    {0x3, 0x208, 0x00000000}, // 0x00666666
    {0x3, 0x200, 0x00000000},
    {0x3, 0x200, 0x00000003},
    {0x3, 0x240, 0x00030041}, // 0x0003C045
    {0x3, 0x248, 0x00000404},
    {0x3, 0x250, 0x0F7F0001},
    {0x3, 0x254, 0x00007813}, // 0x00007813
    {0x3, 0x258, 0x00010002},
};

void DM6300_init2(BWType_e sel) {
    if (sel) {
        WRITE_REG_MAP(dm6300_init2_regs_bw17);
    } else {
        WRITE_REG_MAP(dm6300_init2_regs_bw27);
    }
}

dm6300_reg_value_t dm6300_init3_regs[] = {
    // 03_RFPLL_CA1_TX_10G
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x2B0, 0x00077777},
    {0x3, 0x030, 0x00000013},
    {0x3, 0x034, 0x00000013},
    {0x3, 0x038, 0x00000370},
    {0x3, 0x03C, 0x00000410},
    {0x3, 0x040, 0x00000000},
    {0x3, 0x044, 0x0D640735},
    {0x3, 0x048, 0x01017F03},
    {0x3, 0x04C, 0x021288A2},
    {0x3, 0x050, 0x00FFCF33},
    {0x3, 0x054, 0x1F3C3440},
    // {0x3, 0x028, 0x00008030},
    {0x3, 0x028, 0x00000000}, // [12] // 0x00008000 + (init6300_fcnt & 0xFF)
    // {0x3, 0x020, 0x00003746},
    {0x3, 0x020, 0x00000000}, // [13] // init6300_fnum[ch]}, // tab[0][ch] WAIT(1},
    {0x3, 0x01C, 0x00000002},
    {0x3, 0x018, 0x00000001},
    {0x3, 0x018, 0x00000000},
    // {0x3, 0x028, 0x00008030},
    {0x3, 0x028, 0x00000000}, // [17] // 0x00008000 + (init6300_fcnt & 0xFF)},
    // {0x3, 0x020, 0x00003746},
    {0x3, 0x020, 0x00000000}, // [18] // init6300_fnum[ch]}, // tab[0][ch]
    {0x3, 0x01C, 0x00000003},
    {0x3, 0x018, 0x00000001},
    {0x3, 0x018, 0x00000000},
    {0x3, 0x050, 0x00FFCFB3},
    // {0x3, 0x004, 0x00000093},
    // {0x3, 0x008, 0x00CAAAAB},
    {0x3, 0x004, 0x00000000}, // [23] // tab[1][ch]},
    {0x3, 0x008, 0x00000000}, // [24] // tab[2][ch]},
    {0x3, 0x000, 0x00000000},
    {0x3, 0x000, 0x00000003},
    {0x3, 0x050, 0x000333B3},
    {0x3, 0x040, 0x07070002},
    {0x3, 0x030, 0x00000010}};

void DM6300_init3(uint8_t ch) {
    dm6300_init3_regs[12].dat = 0x00008000 + (init6300_fcnt & 0xFF);
    dm6300_init3_regs[13].dat = init6300_fnum[ch]; // tab[0][ch] WAIT(1);

    dm6300_init3_regs[17].dat = 0x00008000 + (init6300_fcnt & 0xFF);
    dm6300_init3_regs[18].dat = init6300_fnum[ch]; // tab[0][ch]

    dm6300_init3_regs[23].dat = tab[1][ch];
    dm6300_init3_regs[24].dat = tab[2][ch];

    WRITE_REG_MAP(dm6300_init3_regs);
}

CODE_SEG const dm6300_reg_value_t dm6300_init4_regs[] = {
    // 04_TX_CA1_RF
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x31C, 0x00000030}, // 0x00000030
    {0x3, 0x300, 0xC000281B},
    {0x3, 0x304, 0x0CC00006},
    {0x3, 0x308, 0x00000000},
    {0x3, 0x30C, 0x00000000},
    {0x3, 0x310, 0xFFFFFF14},
    {0x3, 0x314, 0x4B7FFFE0},
    {0x3, 0x318, 0xFFFFFFFF},
    {0x3, 0x320, 0x00008000},
    {0x3, 0x324, 0x0000004C},
    {0x3, 0x328, 0x00000000},
    {0x3, 0x32C, 0x00000000},
    {0x3, 0x330, 0x00000000}, // remove init DC high
    {0x3, 0x334, 0x00000000},
    {0x3, 0x31C, 0x00000010},
};

void DM6300_init4() {
    WRITE_REG_MAP(dm6300_init4_regs);
}

CODE_SEG const dm6300_reg_value_t dm6300_init5_regs_bm17[] = {
    // 05_tx_cal_DAC_BBF
    {0x6, 0xFF0, 0x00000019},
    {0x3, 0x194, 0x0001FFFF},
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x778, 0x80000A80},
    {0x3, 0x77C, 0x0D000000},
    {0x3, 0x780, 0x80000A80},
    {0x3, 0x784, 0x0D000000},
    {0x3, 0x788, 0x0FFFFFFF},
    {0x3, 0x78C, 0x00300FD6},
    {0x3, 0x384, 0x000B00B0},
    {0x3, 0x38C, 0x000B00B0},
    {0x3, 0x390, 0x042F5444},
    {0x3, 0x394, 0x2E788108},
    {0x3, 0x398, 0x2E788128},
    {0x3, 0x39C, 0x2C80C0C8},
    {0x3, 0x3A0, 0x00003636},
    {0x3, 0x3A4, 0x04150404},
    {0x3, 0x3A8, 0x00000000},
    {0x3, 0x3AC, 0x00000012},
    {0x3, 0x3B0, 0x00000000},
    {0x3, 0x3D8, 0x0000000A},
    {0x3, 0x380, 0x0B0F8080},
    {0x3, 0x388, 0x0B0F8280}};

CODE_SEG const dm6300_reg_value_t dm6300_init5_regs_bm27[] = {
    // 05_tx_cal_DAC_BBF
    {0x6, 0xFF0, 0x00000019},
    {0x3, 0x194, 0x0001FFFF},
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x778, 0x80000A80}, // 0x80000A80 0x00400680
    {0x3, 0x77C, 0x0D000000}, // 0x0D000000 0x0D800680
    {0x3, 0x780, 0x80000A80}, // 0x80000A80 0x00400680
    {0x3, 0x784, 0x0D000000}, // 0x0D000000 0x0D800680
    {0x3, 0x788, 0x0FFFFFFF},
    {0x3, 0x78C, 0x00300FD6}, // 0x00300FD6 0x80080FD5
    {0x3, 0x384, 0x00075075},
    {0x3, 0x38C, 0x00075075},
    {0x3, 0x390, 0x042F5444},
    {0x3, 0x394, 0x2E788108},
    {0x3, 0x398, 0x2E788128},
    {0x3, 0x39C, 0x2C80C0C8},
    {0x3, 0x3A0, 0x00006868},
    {0x3, 0x3A4, 0x04150404},
    {0x3, 0x3A8, 0x00000000},
    {0x3, 0x3AC, 0x00000000},
    {0x3, 0x3B0, 0x00000000},
    {0x3, 0x3D8, 0x0000000A},
    {0x3, 0x380, 0x075F8000},
    {0x3, 0x388, 0x075F8000},
};

void DM6300_init5(uint8_t sel) {
    if (sel)
        WRITE_REG_MAP(dm6300_init5_regs_bm17);
    else
        WRITE_REG_MAP(dm6300_init5_regs_bm27);
}

dm6300_reg_value_t dm6300_init6_regs[] = {
    {0x6, 0xFF0, 0x00000019},
    {0x3, 0x0E4, 0x00000002},
    {0x3, 0x0E8, 0x0000000D},
    {0x3, 0x08c, 0x00000000},

    {0x6, 0xFF0, 0x00000018},
    {0x3, 0xd04, 0x020019D9}, // 0x0x020000ED
    {0x3, 0xd08, 0x00000000},
    {0x3, 0xd1c, 0x00000000}, // remove init DC high
    {0x3, 0xd00, 0x00000003},

    {0x6, 0xFF0, 0x00000019},

    {0x3, 0x080, 0x00000000}, // [10]

    // if (sel)
    //     SPI_Write(0x3, 0x080, 0x16318C0C);
    // else
    //     SPI_Write(0x3, 0x080, 0x1084210C);

    {0x3, 0x020, 0x0000000C},
    {0x3, 0x018, 0x04F16040},
    {0x3, 0x088, 0x00000085},

    {0x6, 0xFF0, 0x00000018},
    {0x3, 0xD08, 0x03200300},
    {0x3, 0xD0C, 0x00000000},
};

void DM6300_init6(BWType_e sel) {
    dm6300_init6_regs[10].dat = sel ? 0x16318C0C : 0x1084210C;
    WRITE_REG_MAP(dm6300_init6_regs);
}

CODE_SEG const dm6300_reg_value_t dm6300_init7_regs_bw17[] = {
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0xC00, 0x00040004},
    {0x3, 0xC04, 0xFFF70001},
    {0x3, 0xC08, 0xFFDDFFE9},
    {0x3, 0xC0C, 0xFFE7FFDB},
    {0x3, 0xC10, 0x0017FFFF},
    {0x3, 0xC14, 0x00120020},
    {0x3, 0xC18, 0xFFD9FFF4},
    {0x3, 0xC1C, 0xFFF0FFD5},
    {0x3, 0xC20, 0x003B001B},
    {0x3, 0xC24, 0x00070035},
    {0x3, 0xC28, 0xFFACFFCC},
    {0x3, 0xC2C, 0x000CFFC4},
    {0x3, 0xC30, 0x006E0057},
    {0x3, 0xC34, 0xFFD20039},
    {0x3, 0xC38, 0xFF79FF7C},
    {0x3, 0xC3C, 0x0064FFD9},
    {0x3, 0xC40, 0x009600BA},
    {0x3, 0xC44, 0xFF51FFFE},
    {0x3, 0xC48, 0xFF6EFF0B},
    {0x3, 0xC4C, 0x0111004A},
    {0x3, 0xC50, 0x0070012D},
    {0x3, 0xC54, 0xFE75FF44},
    {0x3, 0xC58, 0xFFE3FEA9},
    {0x3, 0xC5C, 0x021F0165},
    {0x3, 0xC60, 0xFF7F0164},
    {0x3, 0xC64, 0xFD2DFDA0},
    {0x3, 0xC68, 0x01A8FEC6},
    {0x3, 0xC6C, 0x03CA03F5},
    {0x3, 0xC70, 0xFBDF0098},
    {0x3, 0xC74, 0xFA42F8B7},
    {0x3, 0xC78, 0x0DC001CD},
    {0x3, 0xC7C, 0x21B419FC},
    {0x3, 0xC80, 0x19FC21B4},
    {0x3, 0xC84, 0x01CD0DC0},
    {0x3, 0xC88, 0xF8B7FA42},
    {0x3, 0xC8C, 0x0098FBDF},
    {0x3, 0xC90, 0x03F503CA},
    {0x3, 0xC94, 0xFEC601A8},
    {0x3, 0xC98, 0xFDA0FD2D},
    {0x3, 0xC9C, 0x0164FF7F},
    {0x3, 0xCA0, 0x0165021F},
    {0x3, 0xCA4, 0xFEA9FFE3},
    {0x3, 0xCA8, 0xFF44FE75},
    {0x3, 0xCAC, 0x012D0070},
    {0x3, 0xCB0, 0x004A0111},
    {0x3, 0xCB4, 0xFF0BFF6E},
    {0x3, 0xCB8, 0xFFFEFF51},
    {0x3, 0xCBC, 0x00BA0096},
    {0x3, 0xCC0, 0xFFD90064},
    {0x3, 0xCC4, 0xFF7CFF79},
    {0x3, 0xCC8, 0x0039FFD2},
    {0x3, 0xCCC, 0x0057006E},
    {0x3, 0xCD0, 0xFFC4000C},
    {0x3, 0xCD4, 0xFFCCFFAC},
    {0x3, 0xCD8, 0x00350007},
    {0x3, 0xCDC, 0x001B003B},
    {0x3, 0xCE0, 0xFFD5FFF0},
    {0x3, 0xCE4, 0xFFF4FFD9},
    {0x3, 0xCE8, 0x00200012},
    {0x3, 0xCEC, 0xFFFF0017},
    {0x3, 0xCF0, 0xFFDBFFE7},
    {0x3, 0xCF4, 0xFFE9FFDD},
    {0x3, 0xCF8, 0x0001FFF7},
    {0x3, 0xCFC, 0x00040004}};
CODE_SEG const dm6300_reg_value_t dm6300_init7_regs_bw27[] = {
    // 07_fir_128stap
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0xC00, 0x004E0041},
    {0x3, 0xC04, 0x0053005D},
    {0x3, 0xC08, 0xFFF7002D},
    {0x3, 0xC0C, 0xFFB1FFC7},
    {0x3, 0xC10, 0xFFF0FFC1},
    {0x3, 0xC14, 0x00450025},
    {0x3, 0xC18, 0x0009003B},
    {0x3, 0xC1C, 0xFF98FFC6},
    {0x3, 0xC20, 0xFFD2FF9A},
    {0x3, 0xC24, 0x00670024},
    {0x3, 0xC28, 0x00380071},
    {0x3, 0xC2C, 0xFF79FFD4},
    {0x3, 0xC30, 0xFF94FF5C},
    {0x3, 0xC34, 0x0085000B},
    {0x3, 0xC38, 0x008A00BC},
    {0x3, 0xC3C, 0xFF620000},
    {0x3, 0xC40, 0xFF2DFF09},
    {0x3, 0xC44, 0x0093FFCA},
    {0x3, 0xC48, 0x010D011B},
    {0x3, 0xC4C, 0xFF640060},
    {0x3, 0xC50, 0xFE88FE9F},
    {0x3, 0xC54, 0x007DFF42},
    {0x3, 0xC58, 0x01E70195},
    {0x3, 0xC5C, 0xFF9C0127},
    {0x3, 0xC60, 0xFD5BFE0B},
    {0x3, 0xC64, 0x000FFE15},
    {0x3, 0xC68, 0x03B9025D},
    {0x3, 0xC6C, 0x007B0323},
    {0x3, 0xC70, 0xF9E2FCC5},
    {0x3, 0xC74, 0xFDD2F9D3},
    {0x3, 0xC78, 0x0F7905A3},
    {0x3, 0xC7C, 0x1DE41880},
    {0x3, 0xC80, 0x18801DE4},
    {0x3, 0xC84, 0x05A30F79},
    {0x3, 0xC88, 0xF9D3FDD2},
    {0x3, 0xC8C, 0xFCC5F9E2},
    {0x3, 0xC90, 0x0323007B},
    {0x3, 0xC94, 0x025D03B9},
    {0x3, 0xC98, 0xFE15000F},
    {0x3, 0xC9C, 0xFE0BFD5B},
    {0x3, 0xCA0, 0x0127FF9C},
    {0x3, 0xCA4, 0x019501E7},
    {0x3, 0xCA8, 0xFF42007D},
    {0x3, 0xCAC, 0xFE9FFE88},
    {0x3, 0xCB0, 0x0060FF64},
    {0x3, 0xCB4, 0x011B010D},
    {0x3, 0xCB8, 0xFFCA0093},
    {0x3, 0xCBC, 0xFF09FF2D},
    {0x3, 0xCC0, 0x0000FF62},
    {0x3, 0xCC4, 0x00BC008A},
    {0x3, 0xCC8, 0x000B0085},
    {0x3, 0xCCC, 0xFF5CFF94},
    {0x3, 0xCD0, 0xFFD4FF79},
    {0x3, 0xCD4, 0x00710038},
    {0x3, 0xCD8, 0x00240067},
    {0x3, 0xCDC, 0xFF9AFFD2},
    {0x3, 0xCE0, 0xFFC6FF98},
    {0x3, 0xCE4, 0x003B0009},
    {0x3, 0xCE8, 0x00250045},
    {0x3, 0xCEC, 0xFFC1FFF0},
    {0x3, 0xCF0, 0xFFC7FFB1},
    {0x3, 0xCF4, 0x002DFFF7},
    {0x3, 0xCF8, 0x005D0053},
    {0x3, 0xCFC, 0x0041004E},
};

void DM6300_init7(BWType_e sel) {
    if (sel) {
        WRITE_REG_MAP(dm6300_init7_regs_bw17);
    } else {
        WRITE_REG_MAP(dm6300_init7_regs_bw27);
    }
}

CODE_SEG const dm6300_reg_value_t dm6300_init_regs[] = {
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x2B0, 0x00077777},
    {0x3, 0x030, 0x00000013},
    {0x3, 0x034, 0x00000013},
    {0x3, 0x038, 0x00000370},
    {0x3, 0x03C, 0x00000410},
    {0x3, 0x040, 0x00000000},
    {0x3, 0x044, 0x0D640735},
    {0x3, 0x048, 0x01017F03},
    {0x3, 0x04C, 0x021288A2},
    {0x3, 0x050, 0x00FFCF33},
    {0x3, 0x054, 0x1F3C3440},
    {0x3, 0x028, 0x00008008},
    {0x3, 0x01C, 0x0000FFF6},
    {0x3, 0x018, 0x00000001},
    {0x3, 0x018, 0x00000000}};

uint8_t DM6300_detect(void) {
    uint32_t rdat = 0;
    SPI_Write(0x6, 0xFF0, 0x18);
    SPI_Read(0x6, 0xFF0, &rdat);

#ifdef _DEBUG_MODE
    if (rdat != 0x18) {
        debugf("\r\ndm6300 lost");
    } else {
        debugf("\r\ndm6300 alive");
    }
#endif
    return (rdat != 0x18) ? 1 : 0;
}

void DM6300_Init(uint8_t ch, BWType_e bw) {
    int i;
    uint32_t dat;

    // dm6300 detect
    dm6300_lost = DM6300_detect();
    // 01_INIT
    DM6300_init1();

#ifdef USE_EFUSE
    DM6300_EFUSE1();
#endif

    WRITE_REG_MAP(dm6300_init_regs);

    SPI_Read(0x3, 0x02C, &dat);
    init6300_fcnt = dat & 0x3FFF;
    init6300_fcnt = 0x20000 / init6300_fcnt - 3;
    for (i = 0; i < FREQ_NUM_EXTERNAL; i++)
        init6300_fnum[i] = freq_tab[i] * init6300_fcnt / 384;

    // 02_BBPLL_3456
    DM6300_init2(bw);

    // 03_RFPLL_CA1_TX_10G
    DM6300_init3(ch);

    // 04_TX_CA1_RF
    DM6300_init4();

    // 05_tx_cal_DAC_BBF
    DM6300_init5(bw);

    // 06_TX_CA1_RBDP_CMOS
    DM6300_init6(bw);

    // 07_fir_128stap
    DM6300_init7(bw);

#ifdef USE_EFUSE
    DM6300_EFUSE2();
#endif
    SPI_Write(0x6, 0xFF0, 0x00000018);
#ifdef _DEBUG_MODE
    debugf("\r\nDM6300 init done.");
#endif
}

////////////////////////////////////////////////////////////////////////////////
// efuse rd amd imp
#define EFUSE_NUM  12  // 12 macro
#define EFUSE_SIZE 128 // 128*8=1024bit

typedef struct _MACRO0 {
    unsigned char chip_name[8];
    unsigned char chip_ver[4];
    unsigned char chip_id[8];
    unsigned char chip_grade[4];
    unsigned char crc[4];
    unsigned char efuse_ver[4];
    unsigned char rsvd1[32];
    unsigned short mode;
    unsigned short band_num;
    unsigned char rsvd2[60];
} MACRO0_T;

typedef struct _MACRO1 {
    unsigned short ical;
    unsigned short rcal;
    unsigned long bandgap;
    unsigned long tempsensor;
    unsigned char rsvd[116];
} MACRO1_T;

typedef struct _RX_CAL {
    unsigned short freq_start;
    unsigned short freq_stop;
    unsigned long iqmismatch[3];
    unsigned long im2;
    unsigned char rsvd[12];
} RX_CAL_T;

typedef struct _TX_CAL {
    unsigned short freq_start;
    unsigned short freq_stop;
    unsigned long dcoc_i;
    unsigned long dcoc_q;

    unsigned long iqmismatch;

    unsigned long dcoc_i_dvm;
    unsigned long dcoc_q_dvm;

    unsigned char rsvd[8];
} TX_CAL_T;

typedef struct _MACRO2 {
    RX_CAL_T rx1;
    TX_CAL_T tx1;
    RX_CAL_T rx2;
    TX_CAL_T tx2;
} MACRO2_T;

typedef union _EFUSE {
    unsigned char dat[EFUSE_NUM][EFUSE_SIZE];
    struct
    {
        MACRO0_T m0;
        MACRO1_T m1;
        MACRO2_T m2[EFUSE_NUM - 2];
        unsigned short band;
        unsigned short err;
    } macro;
} EFUSE_T;

EFUSE_T efuse;

void DM6300_EFUSE1() {
    int i, j;
    int efuse_rdy = 1;

    uint32_t dat;

    memset((char *)&efuse, 0, sizeof(EFUSE_T));

    // EFUSE_RST = 1;
    SPI_Write(0x6, 0xFF0, 0x00000019);
    SPI_Write(0x3, 0x0E0, 0x00000001);

    SPI_Write(0x6, 0xFF0, 0x00000018);

    for (j = 28; j < 32 /*EFUSE_SIZE*/; j++) // read macro 0
    {
        // EFUSE_CFG = (0<<11) | (j<<4) | 0x1;
        SPI_Write(0x3, 0x7D0, (j << 4) | 0x1);

        // while(EFUSE_RDY&1);
        while (efuse_rdy) {
            SPI_Read(0x3, 0x7D4, &dat);
            efuse_rdy = dat & 1;
        }
        efuse_rdy = 1;

        // efuse.data[0][j] = EFUSE_DAT;
        SPI_Read(0x3, 0x7D8, &dat);
        efuse.dat[0][j] = dat & 0xFF;
    }

    for (j = 66; j < 68 /*EFUSE_SIZE*/; j++) // read macro 0
    {
        // EFUSE_CFG = (0<<11) | (j<<4) | 0x1;
        SPI_Write(0x3, 0x7D0, (j << 4) | 0x1);

        // while(EFUSE_RDY&1);
        while (efuse_rdy) {
            SPI_Read(0x3, 0x7D4, &dat);
            efuse_rdy = dat & 1;
        }
        efuse_rdy = 1;

        // efuse.data[0][j] = EFUSE_DAT;
        SPI_Read(0x3, 0x7D8, &dat);
        efuse.dat[0][j] = dat & 0xFF;
    }

    for (j = 0; j < 12 /*EFUSE_SIZE*/; j++) // read macro 1
    {
        // EFUSE_CFG = (1<<11) | (j<<4) | 0x1;
        SPI_Write(0x3, 0x7D0, (1 << 11) | (j << 4) | 0x1);

        // while(EFUSE_RDY&1);
        while (efuse_rdy) {
            SPI_Read(0x3, 0x7D4, &dat);
            efuse_rdy = dat & 1;
        }
        efuse_rdy = 1;

        // efuse.data[1][j] = EFUSE_DAT;
        SPI_Read(0x3, 0x7D8, &dat);
        efuse.dat[1][j] = dat & 0xFF;
    }

// for(i=2; i<efuse.macro.m0.band_num+2; i++) // read macro 2~11
#ifdef KEIL_C51
    efuse.macro.m0.band_num = (efuse.macro.m0.band_num >> 8) | (efuse.macro.m0.band_num << 8);
#endif

    for (i = 2; i < efuse.macro.m0.band_num + 2; i++) // read macro 2~11
    {
        for (j = 32; j < 56 /*EFUSE_SIZE*/; j++) {
            // EFUSE_CFG = (i<<11) | (j<<4) | 0x1;
            SPI_Write(0x3, 0x7D0, (i << 11) | (j << 4) | 0x1);

            // while(EFUSE_RDY&1);
            while (efuse_rdy) {
                SPI_Read(0x3, 0x7D4, &dat);
                efuse_rdy = dat & 1;
            }
            efuse_rdy = 1;

            // efuse.data[i][j] = EFUSE_DAT;
            SPI_Read(0x3, 0x7D8, &dat);
            efuse.dat[i][j] = dat & 0xFF;

            if (j == 35) {
#ifdef KEIL_C51
                efuse.macro.m2[i - 2].tx1.freq_start = (efuse.macro.m2[i - 2].tx1.freq_start >> 8) | (efuse.macro.m2[i - 2].tx1.freq_start << 8);
                efuse.macro.m2[i - 2].tx1.freq_stop = (efuse.macro.m2[i - 2].tx1.freq_stop >> 8) | (efuse.macro.m2[i - 2].tx1.freq_stop << 8);
#endif
                if (efuse.macro.m2[i - 2].tx1.freq_start < 5000 || efuse.macro.m2[i - 2].tx1.freq_stop > 6000)
                    break;
            }
        }
    }

    // EFUSE_CFG = 0;
    SPI_Write(0x3, 0x7D0, 0x00000000);
    // EFUSE_RST = 0;
    SPI_Write(0x6, 0xFF0, 0x00000019);
    SPI_Write(0x3, 0x0E0, 0x00000000);

// application
#ifdef KEIL_C51
    efuse.macro.m1.bandgap = ((efuse.macro.m1.bandgap >> 24) & 0xFF) |
                             ((efuse.macro.m1.bandgap >> 8) & 0xFF00) |
                             ((efuse.macro.m1.bandgap << 8) & 0xFF0000) |
                             ((efuse.macro.m1.bandgap << 24) & 0xFF000000);
    efuse.macro.m1.ical = (efuse.macro.m1.ical >> 8) | (efuse.macro.m1.ical << 8);
    efuse.macro.m1.rcal = (efuse.macro.m1.rcal >> 8) | (efuse.macro.m1.rcal << 8);
#endif
    // debugf("\r\nband_num=%x", efuse.macro.m0.band_num);
    // debugf("\r\nbandgap=%lx", efuse.macro.m1.bandgap);
    // debugf("\r\nical=%x", efuse.macro.m1.ical);
    // debugf("\r\nrcal=%x", efuse.macro.m1.rcal);

    dat = ((efuse.macro.m1.ical & 0x1F) << 3) | (efuse.macro.m1.rcal & 0x7);
    // debugf("\r\nrh=%lx", rh);

    SPI_Write(0x6, 0xF14, efuse.macro.m1.bandgap);
    SPI_Write(0x6, 0xF18, dat);

    SPI_Write(0x6, 0xFF0, 0x00000018);
}

const dm6300_reg_value_t dm6300_regs_dm6300_efuse2_1[] = {
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x3AC, 0x00000012}};

void DM6300_EFUSE2() {
    int i;
    // uint32_t d0,d1,d2,d3,d4,d5,d6,d7;
    // uint32_t wdat;
    uint32_t ef_data;
    uint8_t version[5];
    uint32_t rdat;

    version[4] = 0;
    for (i = 0; i < 4; i++) {
        version[i] = efuse.macro.m0.efuse_ver[i];
    }
#ifdef _DEBUG_DM6300
    debugf("\r\n version = %s", version);
#endif
    // version[1];  //version[1] M.N---M
    // version[3];  //version[3] M.N---N

    WRITE_REG_MAP(dm6300_regs_dm6300_efuse2_1);

    for (i = 0; i < efuse.macro.m0.band_num; i++) // find match macro 5.8G
                                                  // for(i=0; i<FREQ_NUM_EXTERNAL; i++) // find match macro 5.8G
    {
// efuse.macro.m2[i].tx1.freq_start = (efuse.macro.m2[i].tx1.freq_start >> 8) | (efuse.macro.m2[i].tx1.freq_start << 8);
// efuse.macro.m2[i].tx1.freq_stop = (efuse.macro.m2[i].tx1.freq_stop >> 8) | (efuse.macro.m2[i].tx1.freq_stop << 8);
#ifdef _DEBUG_DM6300
        debugf("\r\n start=%x, stop=%x", efuse.macro.m2[i].tx1.freq_start, efuse.macro.m2[i].tx1.freq_stop);
#endif

        if (efuse.macro.m2[i].tx1.freq_start >= 5000 && efuse.macro.m2[i].tx1.freq_stop <= 6000) {
            //*((volatile unsigned int *)0x200D08) = efuse.macro.m2[i].tx1.iqmismatch;
            //*((volatile unsigned int *)0x200380) = efuse.macro.m2[i].tx1.dcoc_i;
            //*((volatile unsigned int *)0x200388) = efuse.macro.m2[i].tx1.dcoc_q;

            if ((version[1] == '2') && (version[3] == '3')) { // version = 2.3
                efuse.macro.m2[i].tx1.dcoc_i = efuse.macro.m2[i].tx1.dcoc_i_dvm;
                efuse.macro.m2[i].tx1.dcoc_q = efuse.macro.m2[i].tx1.dcoc_q_dvm;
            } else {
                efuse.macro.m2[i].tx1.dcoc_i = efuse.macro.m2[i].tx1.dcoc_i;
                efuse.macro.m2[i].tx1.dcoc_q = efuse.macro.m2[i].tx1.dcoc_q;
            }
#ifdef KEIL_C51
            efuse.macro.m2[i].tx1.iqmismatch = ((efuse.macro.m2[i].tx1.iqmismatch >> 24) & 0xFF) |
                                               ((efuse.macro.m2[i].tx1.iqmismatch >> 8) & 0xFF00) |
                                               ((efuse.macro.m2[i].tx1.iqmismatch << 8) & 0xFF0000) |
                                               ((efuse.macro.m2[i].tx1.iqmismatch << 24) & 0xFF000000);

            efuse.macro.m2[i].tx1.dcoc_i = ((efuse.macro.m2[i].tx1.dcoc_i >> 24) & 0xFF) |
                                           ((efuse.macro.m2[i].tx1.dcoc_i >> 8) & 0xFF00) |
                                           ((efuse.macro.m2[i].tx1.dcoc_i << 8) & 0xFF0000) |
                                           ((efuse.macro.m2[i].tx1.dcoc_i << 24) & 0xFF000000);

            efuse.macro.m2[i].tx1.dcoc_q = ((efuse.macro.m2[i].tx1.dcoc_q >> 24) & 0xFF) |
                                           ((efuse.macro.m2[i].tx1.dcoc_q >> 8) & 0xFF00) |
                                           ((efuse.macro.m2[i].tx1.dcoc_q << 8) & 0xFF0000) |
                                           ((efuse.macro.m2[i].tx1.dcoc_q << 24) & 0xFF000000);
#endif
#ifdef _DEBUG_DM6300
            debugf("\r\niqmismatch_old=%lx", efuse.macro.m2[i].tx1.iqmismatch);
            debugf("\r\ndcoc_i_old=%lx", efuse.macro.m2[i].tx1.dcoc_i);
            debugf("\r\ndcoc_q_old=%lx", efuse.macro.m2[i].tx1.dcoc_q);

            // change dc_i/dc_q
            debugf("\r\n version[1] = %c", (uint16_t)version[1]);
            debugf("\r\n version[3] = %c", (uint16_t)version[3]);
#endif
            // if((version[1]>'2') | ((version[1]>='2') && (version[3]>'1'))){ //version > 2.1
            if ((version[1] == '2') && (version[3] == '2')) { // version = 2.2
                ef_data = efuse.macro.m2[i].tx1.dcoc_i;
                ef_data = (ef_data & 0xFFFF8000) | ((((((ef_data & 0x00007f7f) + 0x00000202) >> 2) & 0x00001f1f)) | 0x00000080);
                efuse.macro.m2[i].tx1.dcoc_i = ef_data;

                ef_data = efuse.macro.m2[i].tx1.dcoc_q;
                ef_data = (ef_data & 0xFFFF8000) | ((((((ef_data & 0x00007f7f) + 0x00000202) >> 2) & 0x00001f1f)) | 0x00000080);
                efuse.macro.m2[i].tx1.dcoc_q = ef_data;
            }

            SPI_Write(0x3, 0xD08, efuse.macro.m2[i].tx1.iqmismatch);
            SPI_Write(0x3, 0x380, efuse.macro.m2[i].tx1.dcoc_i);
            SPI_Write(0x3, 0x388, efuse.macro.m2[i].tx1.dcoc_q);

            debugf("\r\niqmismatch=%lx", efuse.macro.m2[i].tx1.iqmismatch);
            debugf("\r\ndcoc_i=%lx", efuse.macro.m2[i].tx1.dcoc_i);
            debugf("\r\ndcoc_q=%lx", efuse.macro.m2[i].tx1.dcoc_q);

            dcoc_ih = efuse.macro.m2[i].tx1.dcoc_i & 0xFFFF0000;
            dcoc_qh = efuse.macro.m2[i].tx1.dcoc_q & 0xFFFF0000;

            if (EE_VALID) {
                rdat = I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_EN);
                if ((rdat & 0xFF) == 0) {
#ifdef _DEBUG_DM6300
                    debugf("\r\nDCOC read from EEPROM:");
#endif
                    SPI_Write(0x6, 0xFF0, 0x00000018);

                    rdat = I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_IH);
                    rdat <<= 8;
                    rdat |= I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_IL);
                    rdat |= dcoc_ih;
                    SPI_Write(0x3, 0x380, rdat);
#ifdef _DEBUG_DM6300
                    debugf("\r\ndcoc_i=%lx", rdat);
#endif

                    rdat = I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_QH);
                    rdat <<= 8;
                    rdat |= I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_QL);
                    rdat |= dcoc_qh;
                    SPI_Write(0x3, 0x388, rdat);
#ifdef _DEBUG_DM6300
                    debugf("\r\ndcoc_q=%lx", rdat);
#endif
                }
            }

            /*if(EE_VALID){
                d0 = I2C_Read8_Wait(10, ADDR_EEPROM, 0xa8);
                d1 = I2C_Read8_Wait(10, ADDR_EEPROM, 0xa9);
                d2 = I2C_Read8_Wait(10, ADDR_EEPROM, 0xaa);
                d3 = I2C_Read8_Wait(10, ADDR_EEPROM, 0xab);
                d4 = I2C_Read8_Wait(10, ADDR_EEPROM, 0xac);
                d5 = I2C_Read8_Wait(10, ADDR_EEPROM, 0xad);
                d6 = I2C_Read8_Wait(10, ADDR_EEPROM, 0xae);
                d7 = I2C_Read8_Wait(10, ADDR_EEPROM, 0xaf);

                debugf("\r\nd0=%lx,d1=%lx,d2=%lx,d3=%lx", d0,d1,d2,d3);
                debugf("\r\nd4=%lx,d5=%lx,d6=%lx,d7=%lx", d4,d5,d6,d7);

                wdat = 0x075F0000;
                wdat = wdat | (d0<<8) | d1;
                SPI_Write(0x3, 0x380, wdat);
                debugf("\r\nreg380=%lx", wdat);
                wdat = 0x075F0000;
                wdat = wdat | (d2<<8) | d3;
                SPI_Write(0x3, 0x388, wdat);
                debugf("\r\nreg388=%lx", wdat);
                wdat = (d4 << 24) | (d5 << 16) | (d6 << 8) | d7;
                SPI_Write(0x3, 0xD08, wdat);
                debugf("\r\nregD08=%lx", wdat);
            }*/

            break;
        }
    }
}
