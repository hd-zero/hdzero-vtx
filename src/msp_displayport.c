#include "msp_displayport.h"

#include "camera.h"
#include "common.h"
#include "dm6300.h"
#include "global.h"
#include "hardware.h"
#include "isr.h"
#include "lifetime.h"
#include "print.h"
#include "smartaudio_protocol.h"
#include "spi.h"
#include "uart.h"
#include "version.h"

uint8_t osd_buf[HD_VMAX][HD_HMAX];
uint8_t loc_buf[HD_VMAX][7];
uint8_t page_extend_buf[HD_VMAX][7];
uint8_t tx_buf[TXBUF_SIZE]; // buffer for sending data to VRX
uint8_t dptxbuf[256];
uint8_t dptx_rptr, dptx_wptr;

uint8_t fc_lock = 0;
// BIT_TYPE[0] msp_displayport
// BIT_TYPE[1] VTX_serial
uint8_t disp_mode; // DISPLAY_OSD | DISPLAY_CMS;
uint8_t osd_ready;

uint8_t fc_variant[4] = {'B', 'T', 'F', 'L'};
uint8_t fontType = 0x00;
uint8_t resolution = SD_3016;
uint8_t resolution_last = HD_5018;

uint8_t msp_rx_buf[64]; // from FC responding status|variant|rc commands
uint8_t disarmed = 1;

uint8_t vtx_channel;
uint8_t vtx_power;
uint8_t vtx_lp;
uint8_t vtx_pit;
uint8_t vtx_pit_save = PIT_OFF;
uint8_t vtx_offset = 0;
uint8_t first_arm = 0;

uint8_t fc_band_rx = 0;
uint8_t fc_channel_rx = 0;
uint8_t fc_pwr_rx = 0;
uint8_t fc_pit_rx = 0;
uint8_t fc_lp_rx = 0;

uint8_t pit_mode_cfg_done = 0;
uint8_t lp_mode_cfg_done = 0;
uint8_t g_IS_ARMED_last = 0;

uint8_t lq_cnt = 0;

uint8_t cms_state = CMS_OSD;

uint8_t msp_tx_cnt = 0xff;

uint8_t msp_rcv = 0;
uint16_t tick_8hz = 0;
uint16_t msp_rcv_tick_8hz = 0;

uint8_t msp_rbuf[64];

uint8_t mspVtxLock = 0;

uint8_t crc8tab[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9};

uint8_t osd_menu_offset = 0;

#ifdef USE_MSP

uint8_t msp_cmp_fc_variant(const char *variant) {
    uint8_t i;
    for (i = 0; i < sizeof(fc_variant); i++) {
        if (fc_variant[i] != variant[i]) {
            return 0;
        }
    }
    return 1;
}

void msp_task() {
    uint8_t len;
    static uint8_t tx_row = 0;
    static uint8_t t1 = 0;
    static uint8_t vmax = SD_VMAX;

#ifdef _DEBUG_DISPLAYPORT
    if (RS0_ERR) {
        RS0_ERR = 0;
        _outchar('$'); // RS0 buffer full
    }
#endif
    DP_tx_task();

    // decide by osd_frame size/rate and dptx rate
    if (msp_read_one_frame()) {
        if (resolution == HD_5018) {
            tx_row = HD_VMAX >> 1;
            vmax = HD_VMAX;
        } else {
            tx_row = SD_VMAX;
            vmax = SD_VMAX;
        }
    }

    if (osd_ready) {
        // send osd
        len = get_tx_data_osd(t1);
#ifdef _DEBUG_DISPLAYPORT
// debugf("\n\r%x ", (uint16_t)t1);
#endif
        insert_tx_buf(len);

        t1++;
        if (t1 >= vmax)
            t1 = 0;
    }

    // send param to FC -- 8HZ
    // send param to VRX -- 8HZ
    if (timer_8hz) {
        len = get_tx_data_5680();
        insert_tx_buf(len);
        //_outchar('0');
        if (dispF_cnt < DISPF_TIME)
            dispF_cnt++;

        if (msp_tx_cnt <= 8)
            msp_tx_cnt++;
        else
            msp_cmd_tx();
        //_outchar('3');
    }

    // set_vtx
    set_vtx_param();
}

uint8_t msp_read_one_frame() {
    static uint8_t state = MSP_HEADER_START;
    static uint8_t cur_cmd = CUR_OTHERS;
    static uint8_t length, osd_len;
    static uint8_t ptr = 0; // write ptr of msp_rx_buf
    static uint8_t crc = 0;
    static uint16_t cmd_u16 = 0;
    static uint16_t len_u16 = 0;

    uint8_t i, ret, full_frame, rx;

    ret = 0;
    full_frame = 0;
    for (i = 0; i < 16; i++) {
        if ((!CMS_ready()) || full_frame)
            return ret;
        rx = CMS_rx();

        switch (state) {
        case MSP_HEADER_START:
            if (rx == MSP_HEADER_START_BYTE) {
                ptr = 0;
                state = MSP_HEADER_M;
            }
#ifdef _DEBUG_DISPLAYPORT
            else
                _outchar('&');
#endif
            break;

        case MSP_HEADER_M:
            if (rx == MSP_HEADER_M_BYTE) {
                state = MSP_PACKAGE_REPLAY1;
            } else if (rx == MSP_HEADER_M2_BYTE) {
                state = MSP_PACKAGE_REPLAY2;
            } else {
                state = MSP_HEADER_START;
            }
            break;

        case MSP_PACKAGE_REPLAY1: // 0x3E
            if (rx == MSP_PACKAGE_REPLAY_BYTE) {
                state = MSP_LENGTH;
            } else {
                state = MSP_HEADER_START;
            }
            break;

        case MSP_LENGTH:
            crc = rx;
            state = MSP_CMD;
            length = rx;
            osd_len = rx - 4;
            break;

        case MSP_CMD:
            crc ^= rx;
            if (length == 0) {
                cur_cmd = CUR_OTHERS;
                state = MSP_CRC1;
            } else {
                if (rx == MSP_CMD_DISPLAYPORT_BYTE) {
                    cur_cmd = CUR_DISPLAYPORT;
                } else if (rx == MSP_CMD_RC_BYTE) {
                    cur_cmd = CUR_RC;
                } else if (rx == MSP_CMD_STATUS_BYTE) {
                    cur_cmd = CUR_STATUS;
                } else if (rx == MSP_CMD_FC_VARIANT) {
                    cur_cmd = CUR_FC_VARIANT;
                } else if (rx == MSP_CMD_VTX_CONFIG) {
                    cur_cmd = CUR_VTX_CONFIG;
                }
                state = MSP_RX1;
            }
            // debugf("\r\n%x ",(uint16_t)rx);
            break;

        case MSP_RX1:
            crc ^= rx;
            msp_rx_buf[ptr++] = rx;
            ptr &= 63;
            length--;
            if (length == 0)
                state = MSP_CRC1;
            break;

        case MSP_CRC1:
            if (rx == crc) {
                if (cur_cmd == CUR_STATUS)
                    parse_status();
                else if (cur_cmd == CUR_RC)
                    parse_rc();
                else if (cur_cmd == CUR_FC_VARIANT)
                    parse_variant();
                else if (cur_cmd == CUR_VTX_CONFIG)
                    parse_vtx_config();
                else if (cur_cmd == CUR_DISPLAYPORT)
                    ret = parse_displayport(osd_len);
                full_frame = 1;
#ifdef INIT_VTX_TABLE
                if (fc_lock & FC_VTX_CONFIG_LOCK && !(fc_lock & FC_INIT_VTX_TABLE_LOCK) && fc_lock & FC_VARIANT_LOCK) {
                    fc_lock |= FC_INIT_VTX_TABLE_LOCK;
                    if (msp_cmp_fc_variant("BTFL") || msp_cmp_fc_variant("QUIC")) {
                        InitVtxTable();
                    }
                }
#endif
            }
#ifdef _DEBUG_DISPLAYPORT
            else
                _outchar('^');
#endif
            state = MSP_HEADER_START;
            break;

        case MSP_PACKAGE_REPLAY2: // 0x3E
            if (rx == MSP_PACKAGE_REPLAY_BYTE) {
                state = MSP_ZERO;
            } else {
                state = MSP_HEADER_START;
            }
            break;

        case MSP_ZERO:         // 0x00
            crc = crc8tab[rx]; // 0 ^ rx = rx
            if (rx == 0x00) {
                state = MSP_CMD_L;
            } else {
                state = MSP_HEADER_START;
            }
            break;

        case MSP_CMD_L:
            crc = crc8tab[crc ^ rx];
            cmd_u16 = rx;
            state = MSP_CMD_H;
            break;

        case MSP_CMD_H:
            crc = crc8tab[crc ^ rx];
            cmd_u16 += ((uint16_t)rx << 8);
            state = MSP_LEN_L;
            break;

        case MSP_LEN_L:
            crc = crc8tab[crc ^ rx];
            len_u16 = rx;
            state = MSP_LEN_H;
            break;

        case MSP_LEN_H:
            crc = crc8tab[crc ^ rx];
            len_u16 += ((uint16_t)rx << 8);
            ptr = 0;
            state = MSP_RX2;
            break;

        case MSP_RX2:
            crc = crc8tab[crc ^ rx];
            msp_rx_buf[ptr++] = rx;
            ptr &= 63;
            len_u16--;
            if (len_u16 == 0)
                state = MSP_CRC2;
            break;

        case MSP_CRC2:
            if (crc == rx)
                parseMspVtx_V2(cmd_u16);
            state = MSP_HEADER_START;
            break;

        default:
            state = MSP_HEADER_START;
            break;
        } // switch(state)
    }     // i
    return ret;
}

void clear_screen() {
    memset(osd_buf, 0x20, sizeof(osd_buf));
    memset(loc_buf, 0x00, sizeof(loc_buf));
    memset(page_extend_buf, 0x00, sizeof(page_extend_buf));
}

void write_string(uint8_t rx, uint8_t row, uint8_t col, uint8_t page_extend) {
    if (disp_mode == DISPLAY_OSD) {
        if (resolution == HD_5018) {
            if (row < HD_VMAX && col < HD_HMAX) {
                osd_buf[row][col] = rx;
                if (page_extend)
                    page_extend_buf[row][col >> 3] |= (1 << (col & 0x07));
                else
                    page_extend_buf[row][col >> 3] &= (0xff - (1 << (col & 0x07)));
            }
        } else if (resolution == SD_3016 || resolution == HD_3016) {
            if (row < SD_VMAX && col < SD_HMAX) {
                osd_buf[row][col] = rx;
                if (page_extend)
                    page_extend_buf[row][col >> 3] |= (1 << (col & 0x07));
                else
                    page_extend_buf[row][col >> 3] &= (0xff - (1 << (col & 0x07)));
            }
        }
    }
}

void mark_loc(uint8_t row, uint8_t col) {
    loc_buf[row][col >> 3] |= (1 << (col & 0x07));
}

void init_tx_buf() {
    uint8_t i;
    for (i = 0; i < TXBUF_SIZE; i++)
        tx_buf[i] = 0;
}

void fc_init() {
    disp_mode = DISPLAY_OSD;
    dptx_wptr = dptx_rptr = 0;

    osd_ready = 0;
    clear_screen();
    init_tx_buf();
    // vtx_menu_init();

    if (resolution == HD_5018)
        osd_menu_offset = 8;
    else
        osd_menu_offset = 0;
}

uint8_t get_tx_data_5680() // prepare data to VRX
{
#ifdef USE_TEMPERATURE_SENSOR
    uint8_t temp;
#endif

    tx_buf[0] = DP_HEADER0;
    tx_buf[1] = DP_HEADER1;
    tx_buf[2] = 0xff;
    // len
    tx_buf[3] = 15;

    // camType
    if (video_format == VDO_FMT_720P50)
        tx_buf[4] = 0x66;
    else if (video_format == VDO_FMT_720P60)
        tx_buf[4] = 0x99;
    else if (video_format == VDO_FMT_720P60_NEW)
        tx_buf[4] = 0xAA;
    else if (video_format == VDO_FMT_720P30)
        tx_buf[4] = 0xCC;
    else if (video_format == VDO_FMT_720X540_90)
        tx_buf[4] = 0xEE;
    else
        tx_buf[4] = 0x99;

    // fcType
    if (msp_cmp_fc_variant("QUIC")) {
        // HACK!
        // TODO: remove once another way of selecting font on the VRX is available
        tx_buf[5] = 'A';
        tx_buf[6] = 'R';
        tx_buf[7] = 'D';
        tx_buf[8] = 'U';
    } else {
        tx_buf[5] = fc_variant[0];
        tx_buf[6] = fc_variant[1];
        tx_buf[7] = fc_variant[2];
        tx_buf[8] = fc_variant[3];
    }

    // counter for link quality
    tx_buf[9] = lq_cnt++;

// VTX temp and overhot
#ifdef USE_TEMPERATURE_SENSOR
    temp = pwr_offset >> 1;
    if (temp > 8)
        temp = 8;
    tx_buf[10] = 0x80 | (heat_protect << 6) | temp;
#else
    tx_buf[10] = 0;
#endif

    tx_buf[11] = fontType; // fontType

    tx_buf[12] = 0x00; // deprecated

    tx_buf[13] = VTX_ID;

    tx_buf[14] = fc_lock & 0x03;

    tx_buf[15] = (camRatio == 0) ? 0xaa : 0x55;

    tx_buf[16] = VTX_VERSION_MAJOR;
    tx_buf[17] = VTX_VERSION_MINOR;
    tx_buf[18] = VTX_VERSION_PATCH_LEVEL;

    return 20;
}

uint8_t get_tx_data_osd(uint8_t index) // prepare osd+data to VTX
{
    // package struct:
    /*
    {
        uint8_t header0;
        uint8_t header1;
        uint8_t row_number;     // include resolution
        uint8_t length;
        uint8_t *mask;          // not ' ' flag. len:hd(7) sd(4)
        uint8_t *loc_flag;      // only for sd. len:4
        uint8_t *string;
        uint8_t *page;          // length decide by len(string)
        uint8_t crc0;
        uint8_t crc1;

    }
    */
    uint8_t mask[7] = {0};
    uint8_t i, t1;
    uint8_t ptr;
    uint8_t hmax;
    uint8_t len_mask;
    uint8_t page[7] = {0};
    uint8_t page_byte = 0;
    uint8_t num = 0;

    if (resolution == HD_5018) {
        hmax = HD_HMAX;
        len_mask = 7;
        ptr = 11;
    } else {
        ptr = 12;
        len_mask = 4;
        hmax = SD_HMAX;
    }

    // string
    for (i = 0; i < hmax; i++) {
        t1 = osd_buf[index][i];
        if (t1 != ' ') {
            mask[i >> 3] |= (1 << (i & 0x07));
            tx_buf[ptr] = t1;

            page[num >> 3] |= ((page_extend_buf[index][i >> 3] >> (i & 0x07)) & 0x01) << (num & 0x07);
            ptr++;
            num++;
        }
    }

    // page
    page_byte = (num >> 3) + ((num & 0x07) != 0);
    for (i = 0; i < page_byte; i++) {
        if (disp_mode == DISPLAY_OSD)
            tx_buf[ptr++] = page[i];
        else
            tx_buf[ptr++] = 0;
    }

    tx_buf[0] = DP_HEADER0;
    tx_buf[1] = DP_HEADER1;
    tx_buf[2] = (resolution << 5) | index;
    tx_buf[3] = ptr - 4; // len

    // 0x20 flag
    for (i = 0; i < len_mask; i++) {
        tx_buf[4 + i] = mask[i];
    }

    // location_flag
    if (resolution == SD_3016) {
        if (disp_mode == DISPLAY_OSD) {
            tx_buf[8] = loc_buf[index][0];
            tx_buf[9] = loc_buf[index][1];
            tx_buf[10] = loc_buf[index][2];
            tx_buf[11] = loc_buf[index][3];
        } else {
            tx_buf[8] = 0x00; // 0x04;
            tx_buf[9] = 0x00;
            tx_buf[10] = 0x00; // 0x04;
            tx_buf[11] = 0x00;
        }
    }

    return (uint8_t)(ptr + 1);
}

void insert_tx_byte(uint8_t c) {
    dptxbuf[dptx_wptr++] = c;
#ifdef _DEBUG_DISPLAYPORT
    if (dptx_wptr == dptx_rptr) // dptxbuf full
        _outchar('*');
#endif
}

#ifdef SDCC
void DP_SEND_27M(uint8_t c) {
    // found by trial and error, are on the conservative side
    // could use refinement down the line
    uint16_t __ticks = 300;
    do {
        __ticks--;
    } while (__ticks);

    DP_tx(c);
}
void DP_SEND_20M(uint8_t c) {
    // found by trial and error, are on the conservative side
    // could use refinement down the line
    uint16_t __ticks = 450;
    do {
        __ticks--;
    } while (__ticks);

    DP_tx(c);
}
#else
#define DP_SEND_27M(c)                  \
    {                                   \
        uint8_t _i_;                    \
        for (_i_ = 0; _i_ < 200; _i_++) \
            ;                           \
        DP_tx(c);                       \
    }
#define DP_SEND_20M(c)                  \
    {                                   \
        uint8_t _i_;                    \
        for (_i_ = 0; _i_ < 200; _i_++) \
            ;                           \
        for (_i_ = 0; _i_ < 100; _i_++) \
            ;                           \
        DP_tx(c);                       \
    }
#endif

void DP_tx_task() {
    uint8_t i;
    for (i = 0; i < 32; i++) {
        if (dptx_wptr != dptx_rptr) {
#if (1)
            if (RF_BW == BW_20M) {
                DP_SEND_20M(dptxbuf[dptx_rptr++]);
            } else {
                DP_SEND_27M(dptxbuf[dptx_rptr++]);
            }
#else
            _outchar(dptxbuf[dptx_rptr++]);
#endif
        } else
            break;
    }
}

void insert_tx_buf(uint8_t len) {
    uint8_t i;
    uint8_t crc0, crc1;

    crc0 = 0;
    crc1 = 0;
    for (i = 0; i < len - 1; i++) {
        crc0 ^= tx_buf[i];
        crc1 = crc8tab[crc1 ^ tx_buf[i]];
        insert_tx_byte(tx_buf[i]);
    }
    insert_tx_byte(crc0);
    insert_tx_byte(crc1);
}

void msp_send_header(uint8_t dl) {
    if (dl) {
        WAIT(20);
    }
    //_outchar('F');
    CMS_tx(0x24);
    //_outchar('G');
    CMS_tx(0x4d);
    //_outchar('H');
    CMS_tx(0x3c);
    //_outchar('I');
}

void msp_cmd_tx() // send 3 commands to FC
{
    uint8_t i, j;
    uint8_t msp_cmd[4] = {
        0x02, // msp_fc_variant
        0x65, // msp_status
        0x69, // msp_rc
        0x58  // msp_vtx_config
    };

    if (fc_lock & FC_VTX_CONFIG_LOCK)
        j = 3;
    else
        j = 4;

    //_outchar('1');
    for (i = 0; i < j; i++) {
        //_outchar('A');
        msp_send_header(0);
        //_outchar('B');
        CMS_tx(0x00);
        //_outchar('C');
        CMS_tx(msp_cmd[i]);
        //_outchar('D');
        CMS_tx(msp_cmd[i]);
        //_outchar('E');
    }
    //_outchar('2');
}

void msp_eeprom_write() {
    msp_send_header(1);
    CMS_tx(0x00);
    CMS_tx(250);
    CMS_tx(250);
}
void msp_set_vtx_config(uint8_t power, uint8_t save) {
    uint8_t crc = 0;
    uint8_t channel = 0;
    uint8_t band;

    if (RF_FREQ < 8) {
        band = 5;
        channel = RF_FREQ;
    } else {
        band = 4;
        if (RF_FREQ == 8)
            channel = 1;
        else if (RF_FREQ == 9)
            channel = 3;
    }
    msp_send_header(0);
    CMS_tx(0x0f);
    crc ^= 0x0f; // len
    CMS_tx(0x59);
    crc ^= 0x59; // cmd
    CMS_tx(0x00);
    crc ^= 0x00; // freq_h
    CMS_tx(0x00);
    crc ^= 0x00; // freq_l
    CMS_tx(power + 1);
    crc ^= (power + 1); // power_level
    CMS_tx((PIT_MODE & 1));
    crc ^= (PIT_MODE & 1); // pitmode
    CMS_tx(LP_MODE);
    crc ^= LP_MODE; // lp_mode
    CMS_tx(0x00);
    crc ^= 0x00; // pit_freq_h
    CMS_tx(0x00);
    crc ^= 0x00; // pit_freq_l
    CMS_tx(band);
    crc ^= band; // band
    CMS_tx(channel + 1);
    crc ^= (channel + 1); // channel
    CMS_tx(0x00);
    crc ^= 0x00; // freq_h
    CMS_tx(0x00);
    crc ^= 0x00; // freq_l
    CMS_tx(0x06);
    crc ^= 0x06; // band number
    CMS_tx(0x08);
    crc ^= 0x08; // channel number
#ifdef HDZERO_FREESTYLE
    if (powerLock) {
        CMS_tx(3);
        crc ^= (3); // power number
    } else {
        CMS_tx(5);
        crc ^= (5); // power number
    }
#else
    CMS_tx(POWER_MAX + 2);
    crc ^= (POWER_MAX + 2); // power number
#endif
    CMS_tx(0x00);
    crc ^= 0x00; // disable table
    CMS_tx(crc);

#ifdef _DEBUG_MODE
    debugf("\r\nmsp_set_vtx_config:F%x,P%x,M:%x", (uint16_t)RF_FREQ, (uint16_t)power, (uint16_t)PIT_MODE);
#endif

    if (save)
        msp_eeprom_write();
}

void parse_status() {
    if (!(fc_lock & FC_STATUS_LOCK))
        fc_lock |= FC_STATUS_LOCK;

    g_IS_ARMED = (msp_rx_buf[6] & 0x01);
    g_IS_PARALYZE = (msp_rx_buf[9] & 0x80);
    disarmed = !g_IS_ARMED;
    // debugf("\n\rstatus:%x",msp_rx_buf[6] & 0x01);
}

void parse_variant() {
    uint8_t i;

    if (!(fc_lock & FC_VARIANT_LOCK))
        fc_lock |= FC_VARIANT_LOCK;

    for (i = 0; i < 4; i++)
        fc_variant[i] = msp_rx_buf[i];
}

void parse_rc() {
    uint16_t roll, pitch, yaw, throttle;
    // static uint16_t roll_d,pitch_d,yaw_d,throttle_d;

    if (!(fc_lock & FC_RC_LOCK))
        fc_lock |= FC_RC_LOCK;

    roll = (msp_rx_buf[1] << 8) | msp_rx_buf[0];
    pitch = (msp_rx_buf[3] << 8) | msp_rx_buf[2];
    yaw = (msp_rx_buf[5] << 8) | msp_rx_buf[4];
    throttle = (msp_rx_buf[7] << 8) | msp_rx_buf[6];

    update_cms_menu(roll, pitch, yaw, throttle);
    /*
    roll_d = roll;
    pitch_d = pitch;
    yaw_d = yaw;
    throttle_d = throttle;
    */
}
void parse_vtx_config() {
    uint8_t nxt_ch = 0;
    /*
    nxt_pwr:
        0: 25mW
        1:200mW
        .....
        POWER_MAX+1: 0mw
    */
    uint8_t nxt_pwr = 0;
    uint8_t pit_update = 0;
    uint8_t needSaveEEP = 0;

    if (!(fc_lock & FC_VTX_CONFIG_LOCK))
        fc_lock |= FC_VTX_CONFIG_LOCK;

    if (!msp_cmp_fc_variant("BTFL") && !msp_cmp_fc_variant("EMUF") && !msp_cmp_fc_variant("QUIC")) {
        return;
    }

    fc_pwr_rx = msp_rx_buf[3] - 1;
    if (fc_pwr_rx > POWER_MAX + 2)
        fc_pwr_rx = 0;
}

void parseMspVtx_V2(uint16_t cmd_u16) {
    uint8_t nxt_ch = 0;
    uint8_t nxt_pwr = 0;
    uint8_t pit_update = 0;
    uint8_t needSaveEEP = 0;
    static uint8_t last_pwr = 255;
    static uint8_t last_lp = 255;
    static uint8_t last_pit = 255;

    if (cmd_u16 != MSP_CMD_VTX_CONFIG)
        return;

    if (!(fc_lock & FC_VTX_CONFIG_LOCK))
        return;

    fc_band_rx = msp_rx_buf[1];
    fc_channel_rx = msp_rx_buf[2];
    fc_pwr_rx = msp_rx_buf[3];
    fc_pit_rx = msp_rx_buf[4];
    fc_lp_rx = msp_rx_buf[8];

    pwr_lmt_done = 1;

#ifdef _DEBUG_MODE
    debugf("\r\nparseMspVtx_V2");
    debugf("\r\n    fc_vtx_dev:    %x", (uint16_t)msp_rx_buf[0]);
    debugf("\r\n    fc_band_rx:    %x", (uint16_t)msp_rx_buf[1]);
    debugf("\r\n    fc_channel_rx: %x", (uint16_t)msp_rx_buf[2]);
    debugf("\r\n    fc_pwr_rx:     %x", (uint16_t)msp_rx_buf[3]);
    debugf("\r\n    fc_pit_rx :    %x", (uint16_t)msp_rx_buf[4]);
    debugf("\r\n    fc_vtx_status: %x", (uint16_t)msp_rx_buf[7]);
    debugf("\r\n    fc_lp_rx:      %x", (uint16_t)msp_rx_buf[8]);
    debugf("\r\n    fc_bands:      %x", (uint16_t)msp_rx_buf[12]);
    debugf("\r\n    fc_channels:   %x", (uint16_t)msp_rx_buf[13]);
    debugf("\r\n    fc_powerLevels %x", (uint16_t)msp_rx_buf[14]);
#endif

    if (SA_lock)
        return;

    mspVtxLock |= 1;

    // update LP_MODE
    if (fc_lp_rx != last_lp) {
        last_lp = fc_lp_rx;
        if (fc_lp_rx < 2) {
            LP_MODE = fc_lp_rx;
        }
        needSaveEEP = 1;
    }
    // update channel
    if (fc_band_rx == 5) // race band
        nxt_ch = fc_channel_rx - 1;
    else if (fc_band_rx == 4) { // fatshark band
        if (fc_channel_rx == 2)
            nxt_ch = 8;
        else if (fc_channel_rx == 4)
            nxt_ch = 9;
    }
    if (RF_FREQ != nxt_ch) {
        vtx_channel = nxt_ch;
        RF_FREQ = nxt_ch;
        if (dm6300_init_done)
            DM6300_SetChannel(RF_FREQ);
        needSaveEEP = 1;
    }

    // update pit
    nxt_pwr = fc_pwr_rx - 1;

    if ((nxt_pwr != POWER_MAX + 1) && (!dm6300_init_done)) {
        Init_6300RF(RF_FREQ, RF_POWER);
        DM6300_AUXADC_Calib();
    }

    PIT_MODE = fc_pit_rx & 1;
#ifdef _DEBUG_MODE
    debugf("\r\nPIT_MODE = %x", (uint16_t)PIT_MODE);
#endif
    if (fc_pit_rx != last_pit) {
        if (PIT_MODE) {
            DM6300_SetPower(POWER_MAX + 1, RF_FREQ, pwr_offset);
            cur_pwr = POWER_MAX + 1;
            vtx_pit_save = PIT_MODE;
        } else {
#ifndef VIDEO_PAT
#ifdef HDZERO_FREESTYLE
            if ((RF_POWER == 3) && (!g_IS_ARMED))
                pwr_lmt_done = 0;
            else
#endif
#endif
                if (nxt_pwr == POWER_MAX + 1) {
                WriteReg(0, 0x8F, 0x10);
                dm6300_init_done = 0;
                cur_pwr = POWER_MAX + 2;
                vtx_pit_save = PIT_0MW;
                temp_err = 1;
            } else {
                DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                cur_pwr = RF_POWER;
                vtx_pit_save = PIT_MODE;
            }
        }
        last_pit = fc_pit_rx;
        needSaveEEP = 1;
    }

    // update power
    if (last_pwr != nxt_pwr) {
        if (last_pwr == POWER_MAX + 1) {
            // Exit 0mW
            if (cur_pwr == POWER_MAX + 2) {
                if (PIT_MODE)
                    Init_6300RF(RF_FREQ, POWER_MAX + 1);
                else
                    Init_6300RF(RF_FREQ, RF_POWER);
                DM6300_AUXADC_Calib();
                needSaveEEP = 1;
            }
            vtx_pit_save = PIT_MODE;
        } else if (nxt_pwr == POWER_MAX + 1) {
            // Enter 0mW
            if (cur_pwr != POWER_MAX + 2) {
                WriteReg(0, 0x8F, 0x10);
                dm6300_init_done = 0;
                cur_pwr = POWER_MAX + 2;
                vtx_pit_save = PIT_0MW;
                temp_err = 1;
            }
        } else if (nxt_pwr <= POWER_MAX) {
            RF_POWER = nxt_pwr;
            if (PIT_MODE)
                RF_POWER = POWER_MAX + 1;
            if (dm6300_init_done) {
                if (cur_pwr != RF_POWER) {
#ifndef VIDEO_PAT
#ifdef HDZERO_FREESTYLE
                    if ((RF_POWER == 3) && (!g_IS_ARMED))
                        pwr_lmt_done = 0;
                    else
#endif
#endif
                    {
                        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                        cur_pwr = RF_POWER;
                    }
                    needSaveEEP = 1;
                }
            } else {
                Init_6300RF(RF_FREQ, RF_POWER);
                DM6300_AUXADC_Calib();
            }
            vtx_pit_save = PIT_MODE;
        }
        last_pwr = nxt_pwr;
    }

    if (needSaveEEP)
        Setting_Save();

#ifdef _DEBUG_MODE
    debugf("\r\nparseMspVtx_V2 pwr:%x, pit:%x", (uint16_t)nxt_pwr, (uint16_t)fc_pit_rx);
#endif
}

uint8_t parse_displayport(uint8_t len) {
    uint8_t row = 0, col = 0;
    uint8_t state_osd = MSP_OSD_SUBCMD;
    uint8_t ptr = 0;
    uint8_t i = 0;
    uint8_t page_extend = 0;

    while (i != 64) {
        switch (state_osd) {
        case MSP_OSD_SUBCMD:
            if (msp_rx_buf[0] == SUBCMD_HEARTBEAT)
                return 0;
            else if (msp_rx_buf[0] == SUBCMD_RELEASE)
                return 0;
            else if (msp_rx_buf[0] == SUBCMD_CLEAR) {
                if (disp_mode == DISPLAY_OSD)
                    clear_screen();
                osd_ready = 0;
#ifdef _DEBUG_DISPLAYPORT
                _outchar('\r');
                _outchar('\n');
                _outchar('C');
#endif
                return 0;
            } else if (msp_rx_buf[0] == SUBCMD_WRITE) {
#ifdef _DEBUG_DISPLAYPORT
                _outchar('W');
#endif
                osd_ready = 0;
                state_osd = MSP_OSD_LOC;
            } else if (msp_rx_buf[0] == SUBCMD_DRAW) {
                osd_ready = 1;
#ifdef _DEBUG_DISPLAYPORT
                _outchar('D');
                _outchar(' ');
#endif
                if (!(fc_lock & FC_OSD_LOCK)) {
                    Flicker_LED(8);
                    fc_lock |= FC_OSD_LOCK;
                }
                return 1;
            } else if (msp_rx_buf[0] == SUBCMD_CONFIG) {
                fontType = msp_rx_buf[1];
                resolution = msp_rx_buf[2];

                if (resolution != resolution_last)
                    fc_init();
                resolution_last = resolution;
                return 0;
            } else {
                return 0;
            }
            break;
        case MSP_OSD_LOC:
            row = msp_rx_buf[1];
            col = msp_rx_buf[2];
            if (resolution == HD_3016) {
                row -= 1;
                col -= 10;
            }
            mark_loc(row, col);
            state_osd = MSP_OSD_ATTR;
            break;
        case MSP_OSD_ATTR:
            if (len == 0)
                return 0;
            else {
                page_extend = msp_rx_buf[3] & 0x01;
                state_osd = MSP_OSD_WRITE;
            }
            break;
        case MSP_OSD_WRITE:
            for (ptr = 0; ptr < len; ptr++) {
                write_string(msp_rx_buf[4 + ptr], row, col + ptr, page_extend);
            }
            return 0;
        default:
            break;
        } // switch
        i++;
    } // while
    return 0;
}

void update_cms_menu(uint16_t roll, uint16_t pitch, uint16_t yaw, uint16_t throttle) {
    /*
     *                throttle(油门) +                                          pitch(俯仰) +
     *
     *
     *   yaw(方向) -                      yaw(方向) +               roll(横滚) -                    roll(横滚) +
     *
     *
     *                throttle(油门) -                                          pitch(俯仰) -
     */
    static uint8_t vtx_state = 0;
    uint8_t mid = 0;
    static uint8_t last_mid = 1;
    static uint8_t cms_cnt;

    uint8_t VirtualBtn = BTN_INVALID;

    uint8_t IS_HI_yaw = IS_HI(yaw);
    uint8_t IS_LO_yaw = IS_LO(yaw);
    uint8_t IS_MID_yaw = IS_MID(yaw);

    uint8_t IS_HI_throttle = IS_HI(throttle);
    uint8_t IS_LO_throttle = IS_LO(throttle);
    // uint8_t IS_MID_throttle = IS_MID(throttle);

    uint8_t IS_HI_pitch = IS_HI(pitch);
    uint8_t IS_LO_pitch = IS_LO(pitch);
    uint8_t IS_MID_pitch = IS_MID(pitch);

    uint8_t IS_HI_roll = IS_HI(roll);
    uint8_t IS_LO_roll = IS_LO(roll);
    uint8_t IS_MID_roll = IS_MID(roll);

    if (!disarmed && (cms_state != CMS_OSD)) {
        fc_init();
        cms_state = CMS_OSD;
    }

    // btn control
    if (IS_MID_yaw && IS_MID_roll && IS_MID_pitch) {
        mid = 1;
        VirtualBtn = BTN_MID;
    } else {
        mid = 0;
        if (IS_HI_yaw && IS_MID_roll && IS_MID_pitch)
            VirtualBtn = BTN_ENTER;
        else if (IS_LO_yaw && IS_MID_roll && IS_MID_pitch)
            VirtualBtn = BTN_EXIT;
        else if (IS_MID_yaw && IS_MID_roll && IS_HI_pitch)
            VirtualBtn = BTN_UP;
        else if (IS_MID_yaw && IS_MID_roll && IS_LO_pitch)
            VirtualBtn = BTN_DOWN;
        else if (IS_MID_yaw && IS_LO_roll && IS_MID_pitch)
            VirtualBtn = BTN_LEFT;
        else if (IS_MID_yaw && IS_HI_roll && IS_MID_pitch)
            VirtualBtn = BTN_RIGHT;
        else
            VirtualBtn = BTN_INVALID;
    }

    switch (cms_state) {
    case CMS_OSD:
        if (disarmed) {
            if (IS_HI_yaw && IS_LO_throttle && IS_LO_roll && IS_LO_pitch) {
                if (cur_pwr == POWER_MAX + 2) {
                    cms_state = CMS_EXIT_0MW;
                    // debugf("\r\ncms_state(%x),cur_pwr(%x)",cms_state, cur_pwr);
                    cms_cnt = 0;
                    break;
                }
                /*if(!SA_lock)*/ {
                    cms_state = CMS_ENTER_VTX_MENU;
                    // debugf("\r\ncms_state(%x)",cms_state);
                    vtx_menu_init();
                    vtx_state = 0;
                }
            } else if (IS_LO_yaw && IS_LO_throttle && IS_HI_roll && IS_LO_pitch) {
                if (cur_pwr != POWER_MAX + 2) {
                    cms_state = CMS_ENTER_0MW;
                    // debugf("\r\ncms_state(%x)",cms_state);
                    cms_cnt = 0;
                }
            } else if (VirtualBtn == BTN_ENTER) {
                cms_state = CMS_ENTER_CAM;
                // debugf("\r\ncms_state(%x)",cms_state);
                cms_cnt = 0;
            } else
                cms_cnt = 0;
        }
        // debugf("\n\r CMS_OSD");
        break;

    case CMS_ENTER_0MW:
        if (IS_LO_yaw && IS_LO_throttle && IS_HI_roll && IS_LO_pitch)
            cms_cnt++;
        else {
            cms_state = CMS_OSD;
        }
        if (cms_cnt == 3) {
            vtx_channel = RF_FREQ;
            vtx_power = RF_POWER;
            vtx_lp = LP_MODE;
            PIT_MODE = PIT_0MW;
            vtx_pit = PIT_0MW;
            if (!SA_lock) {
                save_vtx_param();
            } else {
                msp_set_vtx_config(POWER_MAX + 1, 0); // enter 0mW for SA
                WriteReg(0, 0x8F, 0x10);
                dm6300_init_done = 0;
                cur_pwr = POWER_MAX + 2;
                temp_err = 1;
            }
        }
        break;

    case CMS_EXIT_0MW:
        if (vtx_pit == PIT_0MW) {
            vtx_channel = RF_FREQ;
            vtx_power = RF_POWER;
            vtx_lp = LP_MODE;
            vtx_pit = PIT_OFF;
            vtx_offset = OFFSET_25MW;
            if (SA_lock) {
                // PIT_MODE = 2;
                vtx_pit = PIT_P1MW;
                Init_6300RF(RF_FREQ, POWER_MAX + 1);
                cur_pwr = POWER_MAX + 1;
                DM6300_AUXADC_Calib();
#ifdef _DEBUG_DISPLAYPORT
                debugf("\r\nExit 0mW\r\n");
#endif
                // debugf("\r\n exit0");
            } else {
                save_vtx_param();
                pit_mode_cfg_done = 1; // avoid to config DM6300 again
                // SPI_Write(0x6, 0xFF0, 0x00000018);
                // SPI_Write(0x3, 0xd00, 0x00000003);
                Init_6300RF(RF_FREQ, RF_POWER);
                DM6300_AUXADC_Calib();
#ifdef _DEBUG_MODE
                debugf("\r\nExit 0mW\r\n");
#endif
                // debugf("\r\n exit0");
            }
        }

        if (!(IS_HI_yaw && IS_LO_throttle && IS_LO_roll && IS_LO_pitch))
            cms_state = CMS_OSD;
        break;

    case CMS_ENTER_VTX_MENU: {
        cms_state = CMS_VTX_MENU;
        vtx_menu_init();
        break;
    }

    case CMS_VTX_MENU: {
        if (disarmed) {
            if (last_mid) {
                switch (vtx_state) {
                // channel
                case 0:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 1;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 6;
                    else if (VirtualBtn == BTN_RIGHT) {
                        if (SA_lock == 0) {
                            vtx_channel++;
                            if (vtx_channel == FREQ_MAX_EXT + 1)
                                vtx_channel = 0;
                        }
                    } else if (VirtualBtn == BTN_LEFT) {
                        if (SA_lock == 0) {
                            vtx_channel--;
                            if (vtx_channel > FREQ_MAX_EXT + 1)
                                vtx_channel = FREQ_MAX_EXT;
                        }
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // power
                case 1:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 2;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 0;
                    else if (VirtualBtn == BTN_RIGHT) {
                        if (SA_lock == 0) {
                            vtx_power++;
#ifdef HDZERO_FREESTYLE
                            if (powerLock)
                                vtx_power &= 0x01;
#endif
                            if (vtx_power > POWER_MAX)
                                vtx_power = 0;
                        }
                    } else if (VirtualBtn == BTN_LEFT) {
                        if (SA_lock == 0) {
                            vtx_power--;
#ifdef HDZERO_FREESTYLE
                            if (powerLock)
                                vtx_power &= 0x01;
#endif
                            if (vtx_power > POWER_MAX)
                                vtx_power = POWER_MAX;
                        }
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // lp_mode
                case 2:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 3;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 1;
                    else if (VirtualBtn == BTN_LEFT) {
                        if (SA_lock == 0) {
                            vtx_lp--;
                            if (vtx_lp > 2)
                                vtx_lp = 2;
                        }
                    } else if (VirtualBtn == BTN_RIGHT) {
                        if (SA_lock == 0) {
                            vtx_lp++;
                            if (vtx_lp > 2)
                                vtx_lp = 0;
                        }
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // pit_mode
                case 3:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 4;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 2;
                    else if (VirtualBtn == BTN_RIGHT) {
                        if (SA_lock == 0) {
                            if (vtx_pit == PIT_0MW)
                                vtx_pit = PIT_OFF;
                            else
                                vtx_pit++;
                        }
                    } else if (VirtualBtn == BTN_LEFT) {
                        if (SA_lock == 0) {
                            if (vtx_pit == PIT_OFF)
                                vtx_pit = PIT_0MW;
                            else
                                vtx_pit--;
                        }
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // offset_25mw
                case 4:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 5;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 3;
                    else if (VirtualBtn == BTN_RIGHT) {
                        if (vtx_offset == 10)
                            vtx_offset = vtx_offset;
                        else if (vtx_offset == 11)
                            vtx_offset = 0;
                        else if (vtx_offset < 10)
                            vtx_offset++;
                        else
                            vtx_offset--;
                    } else if (VirtualBtn == BTN_LEFT) {
                        if (vtx_offset == 20)
                            vtx_offset = vtx_offset;
                        else if (vtx_offset == 0)
                            vtx_offset = 11;
                        else if (vtx_offset > 10)
                            vtx_offset++;
                        else
                            vtx_offset--;
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // exit
                case 5:
                    if (VirtualBtn == BTN_DOWN) {
                        vtx_state = 6;
                        update_vtx_menu_param(vtx_state);
                    } else if (VirtualBtn == BTN_UP) {
                        vtx_state = 4;
                        update_vtx_menu_param(vtx_state);
                    } else if (VirtualBtn == BTN_RIGHT) {
                        vtx_state = 0;
                        cms_state = CMS_OSD;
                        fc_init();
                        msp_tx_cnt = 0;
                    }
                    break;

                // save&exit
                case 6:
                    if (VirtualBtn == BTN_DOWN) {
                        vtx_state = 0;
                        update_vtx_menu_param(vtx_state);
                    } else if (VirtualBtn == BTN_UP) {
                        vtx_state = 5;
                        update_vtx_menu_param(vtx_state);
                    } else if (VirtualBtn == BTN_RIGHT) {
                        vtx_state = 0;
                        cms_state = CMS_OSD;
                        if (SA_lock) {
                            RF_FREQ = vtx_channel;
                            RF_POWER = vtx_power;
                            LP_MODE = vtx_lp;
                            PIT_MODE = vtx_pit;
                            vtx_pit_save = vtx_pit;
                            OFFSET_25MW = vtx_offset;
                            CFG_Back();
                            Setting_Save();
                        } else {
                            save_vtx_param();
                        }
                        fc_init();
                        msp_tx_cnt = 0;
                    }
                    break;
                default:
                    cms_state = CMS_OSD;
                    fc_init();
                    break;
                } // switch
            }     // if(last_mid)
            // last_mid = mid;
        } else {
            cms_state = CMS_OSD;
            fc_init();
        }
        break;
    }

    case CMS_ENTER_CAM: {
        if (VirtualBtn == BTN_ENTER)
            cms_cnt++;
        else
            cms_state = CMS_OSD;

        if (cms_cnt == 5) {
            cms_cnt = 0;
            disp_mode = DISPLAY_CMS;
            clear_screen();
            camera_menu_init();
            cms_state = CMS_CAM;
        }
        break;
    }

    case CMS_CAM: {
        // detect to exit
        if (VirtualBtn == BTN_EXIT)
            cms_cnt++;
        else
            cms_cnt = 0;

        if (camera_status_update(VirtualBtn) || (cms_cnt == 10)) {
            disp_mode = DISPLAY_OSD;
            cms_state = CMS_OSD;
            fc_init();
            msp_tx_cnt = 0;
        }

        break;
    }

    default: {
        cms_state = CMS_OSD;
        fc_init();
        break;
    }
    } // switch
    last_mid = mid;
}

void vtx_menu_init() {
    uint8_t i;
    uint8_t hourString[4];
    uint8_t minuteString[2];

    disp_mode = DISPLAY_CMS;
    clear_screen();

    strcpy(osd_buf[0] + osd_menu_offset + 2, "----VTX_MENU----");
    strcpy(osd_buf[2] + osd_menu_offset + 2, ">CHANNEL");
    strcpy(osd_buf[3] + osd_menu_offset + 3, "POWER");
    strcpy(osd_buf[4] + osd_menu_offset + 3, "LP_MODE");
    strcpy(osd_buf[5] + osd_menu_offset + 3, "PIT_MODE");
    strcpy(osd_buf[6] + osd_menu_offset + 3, "OFFSET_25MW");
    strcpy(osd_buf[7] + osd_menu_offset + 3, "EXIT");
    strcpy(osd_buf[8] + osd_menu_offset + 3, "SAVE&EXIT");
    strcpy(osd_buf[9] + osd_menu_offset + 2, "------INFO------");
    strcpy(osd_buf[10] + osd_menu_offset + 3, "VTX");
    strcpy(osd_buf[11] + osd_menu_offset + 3, "VER");
    strcpy(osd_buf[12] + osd_menu_offset + 3, "LIFETIME");

    for (i = 0; i < 5; i++) {
        osd_buf[2 + i][osd_menu_offset + 19] = '<';
        osd_buf[2 + i][osd_menu_offset + 26] = '>';
    }

    // draw variant
    strcpy(osd_buf[10] + osd_menu_offset + 13, VTX_NAME);

    // draw version
    strcpy(osd_buf[11] + osd_menu_offset + 13, VTX_VERSION_STRING);

    ParseLifeTime(hourString, minuteString);
    osd_buf[12][osd_menu_offset + 13] = hourString[0];
    osd_buf[12][osd_menu_offset + 14] = hourString[1];
    osd_buf[12][osd_menu_offset + 15] = hourString[2];
    osd_buf[12][osd_menu_offset + 16] = hourString[3];
    osd_buf[12][osd_menu_offset + 17] = 'H';
    osd_buf[12][osd_menu_offset + 18] = minuteString[0];
    osd_buf[12][osd_menu_offset + 19] = minuteString[1];
    osd_buf[12][osd_menu_offset + 20] = 'M';

    vtx_channel = RF_FREQ;
    vtx_power = RF_POWER;
    vtx_lp = LP_MODE;
    vtx_pit = PIT_MODE;
    vtx_offset = OFFSET_25MW;
    update_vtx_menu_param(0);
}

void update_vtx_menu_param(uint8_t vtx_state) {
    uint8_t i;
    uint8_t hourString[4];
    uint8_t minuteString[2];

    // state
    for (i = 0; i < 7; i++) {
        if (i == vtx_state)
            osd_buf[i + 2][osd_menu_offset + 2] = '>';
        else
            osd_buf[i + 2][osd_menu_offset + 2] = ' ';
    }

    // channel display
    if (vtx_channel < 8) {
        osd_buf[2][osd_menu_offset + 23] = 'R';
        osd_buf[2][osd_menu_offset + 24] = vtx_channel + '1';
    } else {
        osd_buf[2][osd_menu_offset + 23] = 'F';
        if (vtx_channel == 8)
            osd_buf[2][osd_menu_offset + 24] = '2';
        else if (vtx_channel == 9)
            osd_buf[2][osd_menu_offset + 24] = '4';
    }

    // power display
    switch (vtx_power) {
    case 0:
        strcpy(osd_buf[3] + osd_menu_offset + 20, "   25");
        break;
    case 1:
        strcpy(osd_buf[3] + osd_menu_offset + 20, "  200");
        break;
    case 2:
        strcpy(osd_buf[3] + osd_menu_offset + 20, "  500");
        break;
    case 3:
        strcpy(osd_buf[3] + osd_menu_offset + 20, "  MAX");
        break;
    default:
        strcpy(osd_buf[3] + osd_menu_offset + 20, "     ");
        break;
    }

    if (vtx_lp == 0)
        strcpy(osd_buf[4] + osd_menu_offset + 20, "  OFF");
    else if (vtx_lp == 1)
        strcpy(osd_buf[4] + osd_menu_offset + 20, "   ON");
    else if (vtx_lp == 2)
        strcpy(osd_buf[4] + osd_menu_offset + 20, "  1ST");

    if (vtx_pit == PIT_P1MW)
        strcpy(osd_buf[5] + osd_menu_offset + 20, " P1MW");
    else if (vtx_pit == PIT_0MW)
        strcpy(osd_buf[5] + osd_menu_offset + 20, "  0MW");
    else if (vtx_pit == PIT_OFF)
        strcpy(osd_buf[5] + osd_menu_offset + 20, "  OFF");

    if (vtx_offset < 10) {
        strcpy(osd_buf[6] + osd_menu_offset + 20, "     ");
        osd_buf[6][osd_menu_offset + 23] = '0' + vtx_offset;
    } else if (vtx_offset == 10)
        strcpy(osd_buf[6] + osd_menu_offset + 20, "   10");
    else if (vtx_offset < 20) {
        strcpy(osd_buf[6] + osd_menu_offset + 20, "   -");
        osd_buf[6][osd_menu_offset + 24] = '0' + (vtx_offset - 10);
    } else if (vtx_offset == 20)
        strcpy(osd_buf[6] + osd_menu_offset + 20, "  -10");

    ParseLifeTime(hourString, minuteString);
    osd_buf[12][osd_menu_offset + 16] = hourString[0];
    osd_buf[12][osd_menu_offset + 17] = hourString[1];
    osd_buf[12][osd_menu_offset + 18] = hourString[2];
    osd_buf[12][osd_menu_offset + 19] = hourString[3];
    osd_buf[12][osd_menu_offset + 20] = 'H';
    osd_buf[12][osd_menu_offset + 21] = minuteString[0];
    osd_buf[12][osd_menu_offset + 22] = minuteString[1];
    osd_buf[12][osd_menu_offset + 23] = 'M';
}

void save_vtx_param() {
    RF_FREQ = vtx_channel;
    RF_POWER = vtx_power;
    LP_MODE = vtx_lp;
    PIT_MODE = vtx_pit;
    vtx_pit_save = vtx_pit;
    OFFSET_25MW = vtx_offset;
    CFG_Back();
    Setting_Save();
    Imp_RF_Param();

    // init pitmode status and first_arm after setting_save
    pit_mode_cfg_done = 0;
    lp_mode_cfg_done = 0;
    first_arm = 0;

    if (!SA_lock) {
        if (vtx_pit == PIT_0MW)
            msp_set_vtx_config(POWER_MAX + 1, 0);
        else
            msp_set_vtx_config(RF_POWER, 1);
    }
}

void set_vtx_param() {
    // If fc is lost, auto armed
    /*
    if(seconds >= PWR_LMT_SEC){
        if(!fc_lock)
            g_IS_ARMED = 1;
    }
    */
    if (SA_lock)
        return;
    if (!rf_delay_init_done)
        return;

    if (!g_IS_ARMED) {
        // configurate pitmode when power-up or setting_vtx
        if (PIT_MODE) {
            if (!pit_mode_cfg_done) {
                if (vtx_pit_save == PIT_0MW) {
                    WriteReg(0, 0x8F, 0x10);
                    dm6300_init_done = 0;
// SPI_Write(0x6, 0xFF0, 0x00000018);
// SPI_Write(0x3, 0xd00, 0x00000000);
#ifdef _DEBUG_MODE
                    debugf("\r\nDM6300 0mW");
#endif
                    cur_pwr = POWER_MAX + 2;
                    temp_err = 1;
                } else // if(vtx_pit_save == PIT_P1MW)
                {
                    if (!dm6300_init_done) {
                        Init_6300RF(RF_FREQ, POWER_MAX + 1);
                        DM6300_AUXADC_Calib();
                    } else
                        DM6300_SetPower(POWER_MAX + 1, RF_FREQ, 0);
#ifdef _DEBUG_MODE
                    debugf("\r\nDM6300 P1mW");
#endif
                    cur_pwr = POWER_MAX + 1;
                }
                pit_mode_cfg_done = 1;
            }
        } else if (LP_MODE) {
            if (!lp_mode_cfg_done) {
                DM6300_SetPower(0, RF_FREQ, 0); // limit power to 25mW
                cur_pwr = 0;
#ifdef _DEBUG_MODE
                debugf("\n\rEnter LP_MODE");
#endif
                lp_mode_cfg_done = 1;
            }
        }
    }

    if (g_IS_ARMED && !g_IS_ARMED_last) {
        // Power_Auto
        if (vtx_pit_save == PIT_0MW) {
            // exit 0mW
            Init_6300RF(RF_FREQ, RF_POWER);
            DM6300_AUXADC_Calib();
            cur_pwr = RF_POWER;
            vtx_pit = PIT_OFF;
            vtx_pit_save = PIT_OFF;
        } else if (PIT_MODE || LP_MODE) {
// exit pitmode or lp_mode
#ifdef _DEBUG_MDOE
            debugf("\n\rExit PIT or LP");
#endif
#ifndef VIDEO_PAT
#ifdef HDZERO_FREESTYLE
            if (RF_POWER == 3 && !g_IS_ARMED)
                pwr_lmt_done = 0;
            else
#endif
#endif
            {
                DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                cur_pwr = RF_POWER;
            }
        } else if (heat_protect) {
#ifdef HDZERO_FREESTYLE
            WriteReg(0, 0x8F, 0x00);
            WriteReg(0, 0x8F, 0x01);
            DM6300_Init(RF_FREQ, RF_BW);
            DM6300_SetChannel(RF_FREQ);
            DM6300_SetPower(0, RF_FREQ, 0);
            WriteReg(0, 0x8F, 0x11);
#else
            DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
            cur_pwr = RF_POWER;
            heat_protect = 0;
#endif
        }

        first_arm = 1;
        PIT_MODE = PIT_OFF;
        Setting_Save();
        msp_set_vtx_config(RF_POWER, 1);
    } else if (!g_IS_ARMED && g_IS_ARMED_last) {
        if (LP_MODE == 1) {
            DM6300_SetPower(0, RF_FREQ, 0); // limit power to 25mW during disarmed
            cur_pwr = 0;
#ifdef _DEBUG_MDOE
            debugf("\n\rEnter LP_MODE");
#endif
        }
    }

    g_IS_ARMED_last = g_IS_ARMED;
}

#ifdef INIT_VTX_TABLE
CODE_SEG const uint8_t bf_vtx_band_table[6][31] = {
    /*BOSCAM_A*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x01, 0x08, 'B', 'O', 'S', 'C', 'A', 'M', '_', 'A', 'A', 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /*BOSCAM_B*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x02, 0x08, 'B', 'O', 'S', 'C', 'A', 'M', '_', 'B', 'B', 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /*BOSCAM_R*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x03, 0x08, 'B', 'O', 'S', 'C', 'A', 'M', '_', 'E', 'E', 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /*FATSHARK*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x04, 0x08, 'F', 'A', 'T', 'S', 'H', 'A', 'R', 'K', 'F', 0x01, 0x08, 0x00, 0x00, 0x80, 0x16, 0x00, 0x00, 0xa8, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /*RACEBAND*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x05, 0x08, 'R', 'A', 'C', 'E', 'B', 'A', 'N', 'D', 'R', 0x01, 0x08, 0x1a, 0x16, 0x3f, 0x16, 0x64, 0x16, 0x89, 0x16, 0xae, 0x16, 0xd3, 0x16, 0xf8, 0x16, 0x1d, 0x17},
    /*IMD6*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x06, 0x08, 'I', 'M', 'D', '6', ' ', ' ', ' ', ' ', 'I', 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};
CODE_SEG const uint8_t bf_vtx_power_table[5][9] = {
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x01, 0x0e, 0x00, 0x03, '2', '5', ' '}, // 25mW
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x02, 0x17, 0x00, 0x03, '2', '0', '0'}, // 200mW
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x03, 0x00, 0x00, 0x03, '0', ' ', ' '}, // 0mW
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x04, 0x00, 0x00, 0x03, '0', ' ', ' '}, // 0mW
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x05, 0x00, 0x00, 0x03, '0', ' ', ' '}, // 0mW
};
CODE_SEG const uint8_t bf_vtx_power_500mW[9] = {0x07, 0xe4, 0x03, 0x1b, 0x00, 0x03, '5', '0', '0'}; // 500mW
CODE_SEG const uint8_t bf_vtx_power_1W[9] = {0x07, 0xe4, 0x04, 0x1e, 0x00, 0x03, 'M', 'A', 'X'};    // MAX

void InitVtxTable() {
    uint8_t i, j;
    uint8_t crc;
    uint8_t const *power_table[5];

#ifdef _DEBUG_MODE
    debugf("\r\nInitVtxTable");
#endif

    // set band num, channel num and power level number
    LP_MODE = fc_lp_rx;
    msp_set_vtx_config(fc_pwr_rx, 0);

    // set band/channel
    for (i = 0; i < 6; i++) {
        msp_send_header(1);
        crc = 0;
        for (j = 0; j < 31; j++) {
            CMS_tx(bf_vtx_band_table[i][j]);
            crc ^= bf_vtx_band_table[i][j];
        }
        CMS_tx(crc);
    }

    // first two entries are always the same
    power_table[0] = bf_vtx_power_table[0];
    power_table[1] = bf_vtx_power_table[1];

#if defined HDZERO_FREESTYLE
    if (!powerLock) {
        // if we dont have power lock, enable 500mw and 1W
        power_table[2] = bf_vtx_power_500mW;
        power_table[3] = bf_vtx_power_1W;
    } else {
        // otherwise add empties
        power_table[2] = bf_vtx_power_table[2];
        power_table[3] = bf_vtx_power_table[3];
    }
#else
    power_table[2] = bf_vtx_power_table[2];
    power_table[3] = bf_vtx_power_table[3];
#endif

    // last entry is alway the same
    power_table[4] = bf_vtx_power_table[4];

    // send all of them
    for (i = 0; i < 5; i++) {
        msp_send_header(1);
        crc = 0;
        for (j = 0; j < 9; j++) {
            CMS_tx(bf_vtx_power_table[i][j]);
            crc ^= bf_vtx_power_table[i][j];
        }
        CMS_tx(crc);
    }

    msp_eeprom_write();
}
#endif

#else

void fc_init() {}
void msp_task() {}
void msp_set_vtx_config(uint8_t power, uint8_t save) {
    (void)power;
    (void)save;
}

#endif
