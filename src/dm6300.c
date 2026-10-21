#include "dm6300.h"

#include "common.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "i2c_device.h"
#include "monitor.h"
#include "print.h"
#include "rf.h"
#include "spi.h"

typedef struct {
    uint16_t addr;
    uint32_t dat;
} dm6300_reg_value_t;

static int16_t auxadc_offset = 0;
static uint32_t init6300_fcnt = 0;
static uint32_t init6300_fnum[FREQ_NUM_EXTERNAL] = {0};

uint32_t dcoc_ih = 0x075F0000;
uint32_t dcoc_qh = 0x075F0000;

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

static void DM6300_write_reg_map(uint8_t trans, const dm6300_reg_value_t *reg_map, uint8_t size) {
    uint8_t i = 0;
    for (i = 0; i < size; i++) {
        if (trans == 3)
            SPI_Write3(reg_map[i].addr, reg_map[i].dat);
        else
            SPI_Write6(reg_map[i].addr, reg_map[i].dat);
    }
}

static void DM6300_write_reg_map3(const dm6300_reg_value_t *reg_map, uint8_t size) {
    DM6300_write_reg_map(3, reg_map, size);
}
static void DM6300_write_reg_map6(const dm6300_reg_value_t *reg_map, uint8_t size) {
    DM6300_write_reg_map(6, reg_map, size);
}

#define REG_MAP_COUNT(regs)  sizeof(regs) / sizeof(dm6300_reg_value_t)
#define WRITE_REG_MAP3(regs) DM6300_write_reg_map3(regs, REG_MAP_COUNT(regs))
#define WRITE_REG_MAP6(regs) DM6300_write_reg_map6(regs, REG_MAP_COUNT(regs))

void DM6300_Set_Page18() {
    SPI_Write6(0xFF0, 0x00000018);
}
void DM6300_Set_Page19() {
    SPI_Write6(0xFF0, 0x00000019);
}

dm6300_reg_value_t dm6300_set_channel_regs[] = {
    {0x2B0, 0x00077777},
    {0x030, 0x00000013},
    {0x034, 0x00000013},
    {0x038, 0x00000370},
    {0x03C, 0x00000410},
    {0x040, 0x00000000},
    {0x044, 0x0D640735},
    {0x048, 0x01017F03},
    {0x04C, 0x021288A2},
    {0x050, 0x00FFCF33},
    {0x054, 0x1F3C3440},
    {0x028, 0x00008000}, // [11] // 0x00008000 + (init6300_fcnt & 0xFF)
    {0x020, 0x00000000}, // [12] // init6300_fnum[ch]
    {0x01C, 0x00000002},
    {0x018, 0x00000001}, // WAIT(1},
    {0x018, 0x00000000}, // WAIT(1},
    {0x028, 0x00000000}, // [16] // 0x00008000 + (init6300_fcnt & 0xFF)
    {0x020, 0x00000000}, // [17] // init6300_fnum[ch]},
    {0x01C, 0x00000003},
    {0x018, 0x00000001}, // WAIT(1},
    {0x018, 0x00000000}, // WAIT(1},
    {0x050, 0x00FFCFB3},
    {0x004, 0x00000000}, // [22] // tab[1][ch]
    {0x008, 0x00000000}, // [23] // tab[2][ch]
    {0x000, 0x00000000}, // WAIT(1},
    {0x000, 0x00000003},
    {0x050, 0x000333B3},
    {0x040, 0x07070002},
    {0x030, 0x00000010},
};

void DM6300_SetChannel(uint8_t ch) {
    if (ch >= FREQ_NUM)
        ch = 0;

    dm6300_set_channel_regs[11].dat = 0x00008000 + (init6300_fcnt & 0xFF);
    dm6300_set_channel_regs[12].dat = init6300_fnum[ch];

    dm6300_set_channel_regs[16].dat = 0x00008000 + (init6300_fcnt & 0xFF);
    dm6300_set_channel_regs[17].dat = init6300_fnum[ch];

    dm6300_set_channel_regs[22].dat = tab[1][ch];
    dm6300_set_channel_regs[23].dat = tab[2][ch];

    DM6300_Set_Page18();
    WRITE_REG_MAP3(dm6300_set_channel_regs);
}

#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
#define PIT_POWER 0x18 // 2dbm
#else
#define PIT_POWER 0x26
#endif
void DM6300_SetPower(uint8_t pwr, uint8_t freq, uint8_t offset) {
#ifdef HDZERO_FREESTYLE_V1
    uint16_t a_tab[4] = {0x204, 0x11F, 0x21F, 0x31F};
#elif defined HDZERO_FREESTYLE_V2
    uint16_t a_tab[4] = {0x21F, 0x21F, 0x31F, 0x31F};
#else
    uint16_t a_tab[2] = {0x21F, 0x41F};
#endif
    int16_t p;

    if (freq >= FREQ_NUM)
        freq = 0;
    DM6300_Set_Page18();

    if (pwr == POWER_MAX + 1) {
        SPI_Write3(0x330, 0x21F);
        SPI_Write3(0xD1C, PIT_POWER);
    } else {
        SPI_Write3(0x330, a_tab[pwr]);

#ifndef _RF_CALIB
        if (RF_POWER == 0 && pwr == 0) {
            p = table_power[freq][pwr] + offset - 2;

            if (OFFSET_25MW <= 10)
                p += offset_25mw_real();
            else if (p < (OFFSET_25MW - 10))
                p = 0;
            else
                p = p + 10 - offset_25mw_real();
        } else
#endif
            p = table_power[freq][pwr] + offset - 2;

        if (p > 255)
            p = 255;
        else if (p < 0)
            p = 0;
        SPI_Write3(0xD1C, (uint8_t)p);
    }
}
#if (0)
void DM6300_SetSingleTone(uint8_t enable) {
    DM6300_Set_Page19();
    SPI_Write3(0x08C, enable ? 0x18040000 : 0);
    SPI_Write3(0x0C0, enable ? 0x00000002 : 0);
    SPI_Write3(0x0BC, enable ? 0x5ED097B4 : 0);
}
#endif

void DM6300_InitAUXADC() {
    uint32_t dat;
    int16_t dat1, dat2, dat3;

    DM6300_Set_Page18();
    SPI_Read3(0x254, &dat);
    dat |= 0x200;
    SPI_Write3(0x254, dat);

    DM6300_Set_Page19();
    SPI_Write3(0x17C, 0x19);

    DM6300_Set_Page18();
    SPI_Write3(0x2A0, 0xC05B55FE);
    DM6300_Set_Page19();
#ifndef HDZERO_RACE_V2
    WAIT(1);
#endif
    SPI_Read3(0x17C, &dat);
    dat1 = ((int32_t)dat) >> 20;

    DM6300_Set_Page18();
    SPI_Write3(0x2A0, 0x305B55FE);
    DM6300_Set_Page19();
#ifndef HDZERO_RACE_V2
    WAIT(1);
#endif
    SPI_Read3(0x17C, &dat);
    dat2 = ((int32_t)dat) >> 20;

    DM6300_Set_Page18();
    SPI_Write3(0x2A0, 0xA05B51FE);
    DM6300_Set_Page19();
#ifndef HDZERO_RACE_V2
    WAIT(1);
#endif
    SPI_Read3(0x17C, &dat);
    dat3 = ((int32_t)dat) >> 20;

    auxadc_offset = dat3 - ((dat1 + dat2) >> 1);
#ifndef HDZERO_RACE_V2
    if (auxadc_offset < 0x420)
        auxadc_offset = 0x420;
#endif
}

void DM6300_AUXADC_Calib() {
    WriteReg(0x8F, 0x01);
    DM6300_InitAUXADC();
    WriteReg(0x8F, 0x11);
}

int16_t DM6300_GetTemp() {
    static uint8_t init = 1;
    uint32_t dat;
    int16_t temp;

    if (init) {
        init = 0;
        DM6300_Set_Page18();
        SPI_Write3(0x2C0, 0x00000100);
        SPI_Write3(0x2A0, 0xA04005FE);
    }

    DM6300_Set_Page19();
    WAIT(1);
    SPI_Read3(0x17C, &dat);
    temp = (((int32_t)dat) >> 20) + auxadc_offset;

    return temp;
}

CODE_SEG const dm6300_reg_value_t dm6300_init1_regs[] = {
    {0xFFC, 0x00000000},
    {0xFFC, 0x00000001},
    {0x7FC, 0x00000000},
    {0xF1C, 0x00000001},
    {0xF20, 0x0000FCD0},
    {0xF04, 0x00004741},
    {0xF08, 0x00000083},
    {0xF08, 0x000000C3},
    {0xF40, 0x00000003},
    {0xF40, 0x00000001},
    {0xFF0, 0x00000018},
    {0xFFC, 0x00000000},
    {0xFFC, 0x00000001},
};

void DM6300_init1() {
    DM6300_Set_Page18();
    WRITE_REG_MAP6(dm6300_init1_regs);
}

CODE_SEG const dm6300_reg_value_t dm6300_init2_regs_bw17[] = {
    {0x2B0, 0x00077777},
    {0x230, 0x00000000},
    {0x234, 0x10000000},
    {0x238, 0x000000BF},
    {0x23C, 0x55530610},
    {0x240, 0x3FFC0047},
    {0x244, 0x00188A13},
    {0x248, 0x00000000},
    {0x24C, 0x0A121707},
    {0x250, 0x017F0001},
    {0x228, 0x0000807C},
    {0x220, 0x0000292C},
    {0x21C, 0x00000002},
    {0x218, 0x00000001},
    {0x218, 0x00000000},
    {0x228, 0x0000807C},
    {0x220, 0x0000292C},
    {0x21C, 0x00000003},
    {0x218, 0x00000001},
    {0x218, 0x00000000},
    {0x244, 0x00188A17},
    {0x204, 0x0000002A},
    {0x208, 0x00400000},
    {0x200, 0x00000000},
    {0x200, 0x00000003},
    {0x240, 0x00030041},
    {0x248, 0x00000404},
    {0x258, 0x00010003},
    {0x254, 0x00057A17},
};
CODE_SEG const dm6300_reg_value_t dm6300_init2_regs_bw27[] = {
    // 02_BBPLL_3456
    {0x2AC, 0x00000300},
    {0x2B0, 0x00077777},
    {0x230, 0x00000000},
    {0x234, 0x10000000},
    {0x238, 0x000000BF},
    {0x23C, 0x17530610}, // 0x15840410  0x15530610
    {0x240, 0x3FFC0047}, // 0x0001C047  0x3FFC0047
    {0x244, 0x00188A13}, // 0x00188A17
    {0x248, 0x00000000},
    {0x24C, 0x0A161707}, // 0x08021707
    {0x250, 0x017F0001},
    {0x218, 0x00000000},
    {0x228, 0x00008083},
    {0x220, 0x00002E0E}, // 0x00002666
    {0x21C, 0x00000002},
    {0x218, 0x00000001},
    {0x218, 0x00000000},
    {0x21C, 0x00000003},
    {0x218, 0x00000001},
    {0x218, 0x00000000},
    {0x244, 0x00188A17},
    {0x204, 0x0000002D}, // 0x0000004c
    {0x208, 0x00000000}, // 0x00666666
    {0x200, 0x00000000},
    {0x200, 0x00000003},
    {0x240, 0x00030041}, // 0x0003C045
    {0x248, 0x00000404},
    {0x250, 0x0F7F0001},
    {0x254, 0x00007813}, // 0x00007813
    {0x258, 0x00010002},
};

void DM6300_init2(BWType_e sel) {
    DM6300_Set_Page18();
    if (sel)
        WRITE_REG_MAP3(dm6300_init2_regs_bw17);
    else
        WRITE_REG_MAP3(dm6300_init2_regs_bw27);
}

dm6300_reg_value_t dm6300_init3_regs[] = {
    // 03_RFPLL_CA1_TX_10G
    {0x2B0, 0x00077777},
    {0x030, 0x00000013},
    {0x034, 0x00000013},
    {0x038, 0x00000370},
    {0x03C, 0x00000410},
    {0x040, 0x00000000},
    {0x044, 0x0D640735},
    {0x048, 0x01017F03},
    {0x04C, 0x021288A2},
    {0x050, 0x00FFCF33},
    {0x054, 0x1F3C3440},
    {0x028, 0x00000000}, // [11] // 0x00008000 + (init6300_fcnt & 0xFF)
    {0x020, 0x00000000}, // [12] // init6300_fnum[ch]}, // tab[0][ch] WAIT(1},
    {0x01C, 0x00000002},
    {0x018, 0x00000001},
    {0x018, 0x00000000},
    {0x028, 0x00000000}, // [16] // 0x00008000 + (init6300_fcnt & 0xFF)},
    {0x020, 0x00000000}, // [17] // init6300_fnum[ch]}, // tab[0][ch]
    {0x01C, 0x00000003},
    {0x018, 0x00000001},
    {0x018, 0x00000000},
    {0x050, 0x00FFCFB3},
    {0x004, 0x00000000}, // [22] // tab[1][ch]},
    {0x008, 0x00000000}, // [23] // tab[2][ch]},
    {0x000, 0x00000000},
    {0x000, 0x00000003},
    {0x050, 0x000333B3},
    {0x040, 0x07070002},
    {0x030, 0x00000010},
};

void DM6300_init3(uint8_t ch) {
    dm6300_init3_regs[11].dat = 0x00008000 + (init6300_fcnt & 0xFF);
    dm6300_init3_regs[12].dat = init6300_fnum[ch]; // tab[0][ch] WAIT(1);

    dm6300_init3_regs[16].dat = 0x00008000 + (init6300_fcnt & 0xFF);
    dm6300_init3_regs[17].dat = init6300_fnum[ch]; // tab[0][ch]

    dm6300_init3_regs[22].dat = tab[1][ch];
    dm6300_init3_regs[23].dat = tab[2][ch];

    DM6300_Set_Page18();
    WRITE_REG_MAP3(dm6300_init3_regs);
}

CODE_SEG const dm6300_reg_value_t dm6300_init4_regs[] = {
    // 04_TX_CA1_RF
    {0x31C, 0x00000030}, // 0x00000030
    {0x300, 0xC000281B},
    {0x304, 0x0CC00006},
    {0x308, 0x00000000},
    {0x30C, 0x00000000},
    {0x310, 0xFFFFFF14},
    {0x314, 0x4B7FFFE0},
    {0x318, 0xFFFFFFFF},
    {0x320, 0x00008000},
    {0x324, 0x0000004C},
    {0x328, 0x00000000},
    {0x32C, 0x00000000},
    {0x330, 0x00000000}, // remove init DC high
    {0x334, 0x00000000},
    {0x31C, 0x00000010},
};
void DM6300_init4() {
    DM6300_Set_Page18();
    WRITE_REG_MAP3(dm6300_init4_regs);
}

CODE_SEG const dm6300_reg_value_t dm6300_init5_regs_bm17[] = {
    // 05_tx_cal_DAC_BBF
    {0x778, 0x80000A80},
    {0x77C, 0x0D000000},
    {0x780, 0x80000A80},
    {0x784, 0x0D000000},
    {0x788, 0x0FFFFFFF},
    {0x78C, 0x00300FD6},
    {0x384, 0x000B00B0},
    {0x38C, 0x000B00B0},
    {0x390, 0x042F5444},
    {0x394, 0x2E788108},
    {0x398, 0x2E788128},
    {0x39C, 0x2C80C0C8},
    {0x3A0, 0x00003636},
    {0x3A4, 0x04150404},
    {0x3A8, 0x00000000},
    {0x3AC, 0x00000012},
    {0x3B0, 0x00000000},
    {0x3D8, 0x0000000A},
    {0x380, 0x0B0F8080},
    {0x388, 0x0B0F8280},
};
CODE_SEG const dm6300_reg_value_t dm6300_init5_regs_bm27[] = {
    // 05_tx_cal_DAC_BBF
    {0x778, 0x80000A80}, // 0x80000A80 0x00400680
    {0x77C, 0x0D000000}, // 0x0D000000 0x0D800680
    {0x780, 0x80000A80}, // 0x80000A80 0x00400680
    {0x784, 0x0D000000}, // 0x0D000000 0x0D800680
    {0x788, 0x0FFFFFFF},
    {0x78C, 0x00300FD6}, // 0x00300FD6 0x80080FD5
    {0x384, 0x00075075},
    {0x38C, 0x00075075},
    {0x390, 0x042F5444},
    {0x394, 0x2E788108},
    {0x398, 0x2E788128},
    {0x39C, 0x2C80C0C8},
    {0x3A0, 0x00006868},
    {0x3A4, 0x04150404},
    {0x3A8, 0x00000000},
    {0x3AC, 0x00000000},
    {0x3B0, 0x00000000},
    {0x3D8, 0x0000000A},
    {0x380, 0x075F8000},
    {0x388, 0x075F8000},
};

void DM6300_init5(uint8_t sel) {
    DM6300_Set_Page19();
    SPI_Write3(0x194, 0x0001FFFF);
    DM6300_Set_Page18();

    if (sel)
        WRITE_REG_MAP3(dm6300_init5_regs_bm17);
    else
        WRITE_REG_MAP3(dm6300_init5_regs_bm27);
}

dm6300_reg_value_t dm6300_init6_regs_0[] = {
    {0x0E4, 0x00000002},
    {0x0E8, 0x0000000D},
    {0x08c, 0x00000000},
};
dm6300_reg_value_t dm6300_init6_regs_1[] = {
    {0xd04, 0x020019D9}, // 0x0x020000ED
    {0xd08, 0x00000000},
    {0xd1c, 0x00000000}, // remove init DC high
    {0xd00, 0x00000003},
};
dm6300_reg_value_t dm6300_init6_regs_2[] = {
    {0x080, 0x00000000},
    {0x020, 0x0000000C},
    {0x018, 0x04F16040},
    {0x088, 0x00000085},
};
dm6300_reg_value_t dm6300_init6_regs_3[] = {
    {0xD08, 0x03200300},
    {0xD0C, 0x00000000},
};

void DM6300_init6(BWType_e sel) {
    dm6300_init6_regs_2[0].dat = sel ? 0x16318C0C : 0x1084210C;

    DM6300_Set_Page19();
    WRITE_REG_MAP3(dm6300_init6_regs_0);
    DM6300_Set_Page18();
    WRITE_REG_MAP3(dm6300_init6_regs_1);
    DM6300_Set_Page19();
    WRITE_REG_MAP3(dm6300_init6_regs_2);
    DM6300_Set_Page18();
    WRITE_REG_MAP3(dm6300_init6_regs_3);
}

CODE_SEG const dm6300_reg_value_t dm6300_init7_regs_bw17[] = {
    {0xC00, 0x00040004},
    {0xC04, 0xFFF70001},
    {0xC08, 0xFFDDFFE9},
    {0xC0C, 0xFFE7FFDB},
    {0xC10, 0x0017FFFF},
    {0xC14, 0x00120020},
    {0xC18, 0xFFD9FFF4},
    {0xC1C, 0xFFF0FFD5},
    {0xC20, 0x003B001B},
    {0xC24, 0x00070035},
    {0xC28, 0xFFACFFCC},
    {0xC2C, 0x000CFFC4},
    {0xC30, 0x006E0057},
    {0xC34, 0xFFD20039},
    {0xC38, 0xFF79FF7C},
    {0xC3C, 0x0064FFD9},
    {0xC40, 0x009600BA},
    {0xC44, 0xFF51FFFE},
    {0xC48, 0xFF6EFF0B},
    {0xC4C, 0x0111004A},
    {0xC50, 0x0070012D},
    {0xC54, 0xFE75FF44},
    {0xC58, 0xFFE3FEA9},
    {0xC5C, 0x021F0165},
    {0xC60, 0xFF7F0164},
    {0xC64, 0xFD2DFDA0},
    {0xC68, 0x01A8FEC6},
    {0xC6C, 0x03CA03F5},
    {0xC70, 0xFBDF0098},
    {0xC74, 0xFA42F8B7},
    {0xC78, 0x0DC001CD},
    {0xC7C, 0x21B419FC},
    {0xC80, 0x19FC21B4},
    {0xC84, 0x01CD0DC0},
    {0xC88, 0xF8B7FA42},
    {0xC8C, 0x0098FBDF},
    {0xC90, 0x03F503CA},
    {0xC94, 0xFEC601A8},
    {0xC98, 0xFDA0FD2D},
    {0xC9C, 0x0164FF7F},
    {0xCA0, 0x0165021F},
    {0xCA4, 0xFEA9FFE3},
    {0xCA8, 0xFF44FE75},
    {0xCAC, 0x012D0070},
    {0xCB0, 0x004A0111},
    {0xCB4, 0xFF0BFF6E},
    {0xCB8, 0xFFFEFF51},
    {0xCBC, 0x00BA0096},
    {0xCC0, 0xFFD90064},
    {0xCC4, 0xFF7CFF79},
    {0xCC8, 0x0039FFD2},
    {0xCCC, 0x0057006E},
    {0xCD0, 0xFFC4000C},
    {0xCD4, 0xFFCCFFAC},
    {0xCD8, 0x00350007},
    {0xCDC, 0x001B003B},
    {0xCE0, 0xFFD5FFF0},
    {0xCE4, 0xFFF4FFD9},
    {0xCE8, 0x00200012},
    {0xCEC, 0xFFFF0017},
    {0xCF0, 0xFFDBFFE7},
    {0xCF4, 0xFFE9FFDD},
    {0xCF8, 0x0001FFF7},
    {0xCFC, 0x00040004},
};
CODE_SEG const dm6300_reg_value_t dm6300_init7_regs_bw27[] = {
    // 07_fir_128stap
    {0xC00, 0x004E0041},
    {0xC04, 0x0053005D},
    {0xC08, 0xFFF7002D},
    {0xC0C, 0xFFB1FFC7},
    {0xC10, 0xFFF0FFC1},
    {0xC14, 0x00450025},
    {0xC18, 0x0009003B},
    {0xC1C, 0xFF98FFC6},
    {0xC20, 0xFFD2FF9A},
    {0xC24, 0x00670024},
    {0xC28, 0x00380071},
    {0xC2C, 0xFF79FFD4},
    {0xC30, 0xFF94FF5C},
    {0xC34, 0x0085000B},
    {0xC38, 0x008A00BC},
    {0xC3C, 0xFF620000},
    {0xC40, 0xFF2DFF09},
    {0xC44, 0x0093FFCA},
    {0xC48, 0x010D011B},
    {0xC4C, 0xFF640060},
    {0xC50, 0xFE88FE9F},
    {0xC54, 0x007DFF42},
    {0xC58, 0x01E70195},
    {0xC5C, 0xFF9C0127},
    {0xC60, 0xFD5BFE0B},
    {0xC64, 0x000FFE15},
    {0xC68, 0x03B9025D},
    {0xC6C, 0x007B0323},
    {0xC70, 0xF9E2FCC5},
    {0xC74, 0xFDD2F9D3},
    {0xC78, 0x0F7905A3},
    {0xC7C, 0x1DE41880},
    {0xC80, 0x18801DE4},
    {0xC84, 0x05A30F79},
    {0xC88, 0xF9D3FDD2},
    {0xC8C, 0xFCC5F9E2},
    {0xC90, 0x0323007B},
    {0xC94, 0x025D03B9},
    {0xC98, 0xFE15000F},
    {0xC9C, 0xFE0BFD5B},
    {0xCA0, 0x0127FF9C},
    {0xCA4, 0x019501E7},
    {0xCA8, 0xFF42007D},
    {0xCAC, 0xFE9FFE88},
    {0xCB0, 0x0060FF64},
    {0xCB4, 0x011B010D},
    {0xCB8, 0xFFCA0093},
    {0xCBC, 0xFF09FF2D},
    {0xCC0, 0x0000FF62},
    {0xCC4, 0x00BC008A},
    {0xCC8, 0x000B0085},
    {0xCCC, 0xFF5CFF94},
    {0xCD0, 0xFFD4FF79},
    {0xCD4, 0x00710038},
    {0xCD8, 0x00240067},
    {0xCDC, 0xFF9AFFD2},
    {0xCE0, 0xFFC6FF98},
    {0xCE4, 0x003B0009},
    {0xCE8, 0x00250045},
    {0xCEC, 0xFFC1FFF0},
    {0xCF0, 0xFFC7FFB1},
    {0xCF4, 0x002DFFF7},
    {0xCF8, 0x005D0053},
    {0xCFC, 0x0041004E},
};

void DM6300_init7(BWType_e sel) {
    DM6300_Set_Page18();
    if (sel)
        WRITE_REG_MAP3(dm6300_init7_regs_bw17);
    else
        WRITE_REG_MAP3(dm6300_init7_regs_bw27);
}

uint8_t DM6300_is_alive(void) {
    uint32_t rdat = 0;
    DM6300_Set_Page18();
    SPI_Read6(0xFF0, &rdat);
    return rdat == 0x18;
}

CODE_SEG const dm6300_reg_value_t dm6300_init_regs[] = {
    {0x2B0, 0x00077777},
    {0x030, 0x00000013},
    {0x034, 0x00000013},
    {0x038, 0x00000370},
    {0x03C, 0x00000410},
    {0x040, 0x00000000},
    {0x044, 0x0D640735},
    {0x048, 0x01017F03},
    {0x04C, 0x021288A2},
    {0x050, 0x00FFCF33},
    {0x054, 0x1F3C3440},
    {0x028, 0x00008008},
    {0x01C, 0x0000FFF6},
    {0x018, 0x00000001},
    {0x018, 0x00000000},
};

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
    DM6300_Set_Page19();
    SPI_Write3(0x0E0, 0x00000001);

    DM6300_Set_Page18();

    for (j = 28; j < 32 /*EFUSE_SIZE*/; j++) // read macro 0
    {
        // EFUSE_CFG = (0<<11) | (j<<4) | 0x1;
        SPI_Write3(0x7D0, (j << 4) | 0x1);

        // while(EFUSE_RDY&1);
        while (efuse_rdy) {
            SPI_Read3(0x7D4, &dat);
            efuse_rdy = dat & 1;
        }
        efuse_rdy = 1;

        // efuse.data[0][j] = EFUSE_DAT;
        SPI_Read3(0x7D8, &dat);
        efuse.dat[0][j] = dat & 0xFF;
    }

    for (j = 66; j < 68 /*EFUSE_SIZE*/; j++) // read macro 0
    {
        // EFUSE_CFG = (0<<11) | (j<<4) | 0x1;
        SPI_Write3(0x7D0, (j << 4) | 0x1);

        // while(EFUSE_RDY&1);
        while (efuse_rdy) {
            SPI_Read3(0x7D4, &dat);
            efuse_rdy = dat & 1;
        }
        efuse_rdy = 1;

        // efuse.data[0][j] = EFUSE_DAT;
        SPI_Read3(0x7D8, &dat);
        efuse.dat[0][j] = dat & 0xFF;
    }

    for (j = 0; j < 12 /*EFUSE_SIZE*/; j++) // read macro 1
    {
        // EFUSE_CFG = (1<<11) | (j<<4) | 0x1;
        SPI_Write3(0x7D0, (1 << 11) | (j << 4) | 0x1);

        // while(EFUSE_RDY&1);
        while (efuse_rdy) {
            SPI_Read3(0x7D4, &dat);
            efuse_rdy = dat & 1;
        }
        efuse_rdy = 1;

        // efuse.data[1][j] = EFUSE_DAT;
        SPI_Read3(0x7D8, &dat);
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
            SPI_Write3(0x7D0, (i << 11) | (j << 4) | 0x1);

            // while(EFUSE_RDY&1);
            while (efuse_rdy) {
                SPI_Read3(0x7D4, &dat);
                efuse_rdy = dat & 1;
            }
            efuse_rdy = 1;

            // efuse.data[i][j] = EFUSE_DAT;
            SPI_Read3(0x7D8, &dat);
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
    SPI_Write3(0x7D0, 0x00000000);
    // EFUSE_RST = 0;
    DM6300_Set_Page19();
    SPI_Write3(0x0E0, 0x00000000);

// application
#ifdef KEIL_C51
    efuse.macro.m1.bandgap = ((efuse.macro.m1.bandgap >> 24) & 0xFF) |
                             ((efuse.macro.m1.bandgap >> 8) & 0xFF00) |
                             ((efuse.macro.m1.bandgap << 8) & 0xFF0000) |
                             ((efuse.macro.m1.bandgap << 24) & 0xFF000000);
    efuse.macro.m1.ical = (efuse.macro.m1.ical >> 8) | (efuse.macro.m1.ical << 8);
    efuse.macro.m1.rcal = (efuse.macro.m1.rcal >> 8) | (efuse.macro.m1.rcal << 8);
#endif

    dat = ((efuse.macro.m1.ical & 0x1F) << 3) | (efuse.macro.m1.rcal & 0x7);

    SPI_Write6(0xF14, efuse.macro.m1.bandgap);
    SPI_Write6(0xF18, dat);

    DM6300_Set_Page18();
}

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
    // version[1];  //version[1] M.N---M
    // version[3];  //version[3] M.N---N

    DM6300_Set_Page18();
    SPI_Write3(0x3AC, 0x00000012);

    for (i = 0; i < efuse.macro.m0.band_num; i++) // find match macro 5.8G
                                                  // for(i=0; i<FREQ_NUM_EXTERNAL; i++) // find match macro 5.8G
    {
        // efuse.macro.m2[i].tx1.freq_start = (efuse.macro.m2[i].tx1.freq_start >> 8) | (efuse.macro.m2[i].tx1.freq_start << 8);
        // efuse.macro.m2[i].tx1.freq_stop = (efuse.macro.m2[i].tx1.freq_stop >> 8) | (efuse.macro.m2[i].tx1.freq_stop << 8);

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
            // if((version[1]>'2') | ((version[1]>='2') && (version[3]>'1'))){ //version > 2.1
            if ((version[1] == '2') && (version[3] == '2')) { // version = 2.2
                ef_data = efuse.macro.m2[i].tx1.dcoc_i;
                ef_data = (ef_data & 0xFFFF8000) | ((((((ef_data & 0x00007f7f) + 0x00000202) >> 2) & 0x00001f1f)) | 0x00000080);
                efuse.macro.m2[i].tx1.dcoc_i = ef_data;

                ef_data = efuse.macro.m2[i].tx1.dcoc_q;
                ef_data = (ef_data & 0xFFFF8000) | ((((((ef_data & 0x00007f7f) + 0x00000202) >> 2) & 0x00001f1f)) | 0x00000080);
                efuse.macro.m2[i].tx1.dcoc_q = ef_data;
            }

            SPI_Write3(0xD08, efuse.macro.m2[i].tx1.iqmismatch);
            SPI_Write3(0x380, efuse.macro.m2[i].tx1.dcoc_i);
            SPI_Write3(0x388, efuse.macro.m2[i].tx1.dcoc_q);

            dcoc_ih = efuse.macro.m2[i].tx1.dcoc_i & 0xFFFF0000;
            dcoc_qh = efuse.macro.m2[i].tx1.dcoc_q & 0xFFFF0000;

            if (EE_VALID) {
                rdat = I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_EN);
                if ((rdat & 0xFF) == 0) {
                    DM6300_Set_Page18();

                    rdat = I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_IH);
                    rdat <<= 8;
                    rdat |= I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_IL);
                    rdat |= dcoc_ih;
                    SPI_Write3(0x380, rdat);

                    rdat = I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_QH);
                    rdat <<= 8;
                    rdat |= I2C_Read8_Wait(10, ADDR_EEPROM, EEP_ADDR_DCOC_QL);
                    rdat |= dcoc_qh;
                    SPI_Write3(0x388, rdat);
                }
            }
            break;
        }
    }
}

void DM6300_Init(uint8_t ch, BWType_e bw) {
    int i;
    uint32_t dat;

    // 01_INIT
    DM6300_init1();

#ifdef USE_EFUSE
    DM6300_EFUSE1();
#endif

    DM6300_Set_Page18();
    WRITE_REG_MAP3(dm6300_init_regs);

    SPI_Read3(0x02C, &dat);
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
    DM6300_Set_Page18();
}