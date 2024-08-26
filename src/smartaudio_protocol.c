#include "smartaudio_protocol.h"
#include "common.h"
#include "dm6300.h"
#include "global.h"
#include "hardware.h"
#include "i2c_device.h"
#include "isr.h"
#include "monitor.h"
#include "msp_displayport.h"
#include "print.h"
#include "rom.h"
#include "sfr_ext.h"
#include "uart.h"

uint8_t SA_lock = 0;

uint8_t pwr_init = 0; // 0:POWER_MAX+2
uint8_t ch_init = 0;  // 0~9
uint8_t ch_bf = 0;

#if defined USE_SMARTAUDIO_SW || defined USE_SMARTAUDIO_HW
uint8_t SA_is_0 = 1; // detect pre hreder 0x00
uint8_t SA_config = 0;

uint8_t sa_rbuf[8];
uint8_t SA_dbm = 14;
uint8_t SA_dbm_last;
uint8_t mode_o = 0x04;
// Bit0: 1=set freq 0=set ch    0
// Bit1: pitmode active         0
// Bit2: PIR active             1
// Bit3: POR active             0
// Bit4: 1 = Unclocked VTX      1

uint8_t mode_p = 0x05;
// Bit0: PIR active             1
// Bit1: POR active             0
// Bit2: pitmode active         0
// Bit3: 1 = Unclocked VTX      1

uint8_t freq_new_h;
uint8_t freq_new_l;

uint8_t sa_status = SA_ST_IDLE;
uint32_t sa_start_ms = 0;

uint8_t dbm_to_pwr(uint8_t dbm) {
    if (dbm == 0)
        return POWER_MAX + 2;
    else if (dbm == 14) // 25mw
        return 0;
    else if (dbm == 23) // 200mw
        return 1;
#if (POWER_MAX > 1)
    else if (dbm == 27) // 500mw
        return 2;
#endif
#if (POWER_MAX > 2)
    else if (dbm == 30) // 1W
        return 3;
#endif
    else
        return 0;
}

uint8_t pwr_to_dbm(uint8_t pwr) {
    if (pwr == 0) // 25mw
        return 14;
    else if (pwr == 1) // 200mw
        return 23;
#if (POWER_MAX > 1)
    else if (pwr == 2) // 500mw
        return 27;
#endif
#if (POWER_MAX > 2)
    else if (pwr == 3) // 1W
        return 30;
#endif
    else if (pwr == POWER_MAX + 2)
        return 0;
    else
        return 14;
}

void SA_Response(uint8_t cmd) {
    uint8_t i, crc, tx_len;
    uint8_t tbuf[20];
    switch (cmd) {
    case SA_GET_SETTINGS:
        tbuf[0] = 0xAA;
        tbuf[1] = 0x55;
        tbuf[2] = 0x11;               // version V2.1
        tbuf[3] = 0x0A + POWER_MAX;   // length
        tbuf[4] = ch_bf;              // channel
        tbuf[5] = dbm_to_pwr(SA_dbm); // power level
        tbuf[6] = mode_o;             // operation mode
        tbuf[7] = freq_new_h;         // cur_freq_h
        tbuf[8] = freq_new_l;         // cur_freq_l
        tbuf[9] = SA_dbm;             // power dbm
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
        if (powerLock) {
            tbuf[10] = 1 + 1; // amount of power level
            for (i = 0; i <= 1; i++)
                tbuf[11 + i] = pwr_to_dbm(i);
            tbuf[12 + 1] = 0;
        } else
#endif
            tbuf[10] = POWER_MAX + 1; // amount of power level
        for (i = 0; i <= POWER_MAX; i++)
            tbuf[11 + i] = pwr_to_dbm(i);
        tbuf[12 + POWER_MAX] = 0;

        tx_len = tbuf[3] + 4;
        break;

    case SA_SET_PWR:
        tbuf[0] = 0xAA;
        tbuf[1] = 0x55;
        tbuf[2] = 2;
        tbuf[3] = 3;
        tbuf[4] = SA_dbm;
        tbuf[5] = 1; // reserved
        tx_len = 7;
        break;

    case SA_SET_CH:
        tbuf[0] = 0xaa;
        tbuf[1] = 0x55;
        tbuf[2] = 3;
        tbuf[3] = 3;
        tbuf[4] = ch_bf;
        tbuf[5] = 1; // reserved
        tx_len = 7;
        break;

    case SA_SET_FREQ:
        tbuf[0] = 0xaa;
        tbuf[1] = 0x55;
        tbuf[2] = 4;
        tbuf[3] = 4;
        tbuf[4] = freq_new_h;
        tbuf[5] = freq_new_l;
        tbuf[6] = 0x01; // reserved
        tx_len = 8;
        break;

    case SA_SET_MODE:
        tbuf[0] = 0xaa;
        tbuf[1] = 0x55;
        tbuf[2] = 5;
        tbuf[3] = 3;
        tbuf[4] = mode_p;
        tbuf[5] = 0x01; // reserved
        tx_len = 7;
        break;

    default:
        return;
    } // switch(cmd)

    // calculate CRC
    crc = 0;
    for (i = 2; i < tx_len - 1; i++)
        crc = crc8tab[crc ^ tbuf[i]];

    tbuf[tx_len - 1] = crc;
    SUART_tx(tbuf, tx_len);
}

void SA_Update(uint8_t cmd) {
    switch (cmd) {

    case SA_SET_PWR:
        if (!(sa_rbuf[0] >> 7))
            return;
        SA_dbm = sa_rbuf[0] & 0x7f;

        if (SA_dbm_last != SA_dbm) { // need to update power
            if (SA_dbm == 0) {       // Enter 0mW
                cur_pwr = POWER_MAX + 2;
                PIT_MODE = 0;
                vtx_pit = PIT_0MW;

                if (last_SA_lock && seconds < WAIT_SA_CONFIG) {
                    pwr_init = cur_pwr;
                } else {
                    WriteReg(0, 0x8F, 0x10); // reset RF_chip
                    dm6300_init_done = 0;
                    temp_err = 1;
                }
            } else if (SA_dbm_last == 0) { // Exit 0mW
                cur_pwr = dbm_to_pwr(SA_dbm);
                PIT_MODE = 0;
                RF_POWER = cur_pwr;

                if (last_SA_lock && seconds < WAIT_SA_CONFIG)
                    pwr_init = cur_pwr;
                else {
                    Init_6300RF(RF_FREQ, RF_POWER);
                    PIT_MODE = 0;

                    DM6300_AUXADC_Calib();
                }
            } else {
                cur_pwr = dbm_to_pwr(SA_dbm);
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
                if (powerLock)
                    cur_pwr &= 0x01;
#endif
                RF_POWER = cur_pwr;
                if (last_SA_lock && seconds < WAIT_SA_CONFIG)
                    pwr_init = cur_pwr;
                else {
#ifndef VIDEO_PAT
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
                    if ((RF_POWER == 3) && (!g_IS_ARMED))
                        pwr_lmt_done = 0;
                    else
#endif
#endif
                        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                }

                Setting_Save();
            }
            SA_dbm_last = SA_dbm;
        }
        break;

    case SA_SET_CH: {
        uint8_t const channel = bfChannel_to_channel(sa_rbuf[0]);
        if (channel < INVALID_CHANNEL) {
            ch_bf = sa_rbuf[0];
            if (channel != RF_FREQ) {
                RF_FREQ = channel;
                if (last_SA_lock && (seconds < WAIT_SA_CONFIG))
                    ch_init = channel;
                else
                    DM6300_SetChannel(channel);
                Setting_Save();
            }
        }
        break;
    }

    case SA_SET_FREQ: {
        uint16_t const freq = ((uint16_t)sa_rbuf[0] << 8) + sa_rbuf[1];
        uint8_t const channel = DM6300_GetChannelByFreq(freq);
        if (channel < INVALID_CHANNEL) {
            ch_bf = channel_to_bfChannel(channel);
            if (channel != RF_FREQ) {
                RF_FREQ = channel;
                if (last_SA_lock && (seconds < WAIT_SA_CONFIG))
                    ch_init = channel;
                else
                    DM6300_SetChannel(channel);
                Setting_Save();
            }
        }
        break;
    }

    case SA_SET_MODE:
        if (mode_p != sa_rbuf[0]) {
            mode_p = sa_rbuf[0] & 0x07;

            if (mode_p & 0x04) { // deactive pitmode
                PIT_MODE = 0;
                mode_o &= 0x1d;
            } else { // active pitmode
                PIT_MODE = 2;
                mode_o |= 0x02;
            }
            if (cur_pwr == (POWER_MAX + 2))
                return;

            if (PIT_MODE) {
                if (last_SA_lock && seconds < WAIT_SA_CONFIG)
                    pwr_init = cur_pwr;
                else if (dbm_to_pwr(SA_dbm) == (POWER_MAX + 2)) {
                    cur_pwr = POWER_MAX + 2;
                    PIT_MODE = 0;
                    vtx_pit = PIT_0MW;
                    WriteReg(0, 0x8F, 0x10); // reset RF_chip
                    dm6300_init_done = 0;
                    temp_err = 1;
                } else {
                    DM6300_SetPower(POWER_MAX + 1, RF_FREQ, 0);
                    cur_pwr = POWER_MAX + 1;
                }
            } else {
                if (last_SA_lock && seconds < WAIT_SA_CONFIG)
                    pwr_init = cur_pwr;
                else if (dbm_to_pwr(SA_dbm) == (POWER_MAX + 2)) {
                    cur_pwr = POWER_MAX + 2;
                    PIT_MODE = 0;
                    vtx_pit = PIT_0MW;
                    WriteReg(0, 0x8F, 0x10); // reset RF_chip
                    dm6300_init_done = 0;
                    temp_err = 1;
                } else {
#ifndef VIDEO_PAT
#if defined HDZERO_FREESTYLE_V1 || HDZERO_FREESTYLE_V2
                    if ((RF_POWER == 3) && (!g_IS_ARMED)) {
                        pwr_lmt_done = 0;
                        cur_pwr = 3;
                    } else
#endif
#endif
                    {
                        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                        cur_pwr = RF_POWER;
                    }
                }
            }

            Setting_Save();
        }
        break;

    default:
        break;
    }
}
#if defined USE_SMARTAUDIO_SW
uint8_t SA_task(void) {
    static uint8_t st = 0, SA = 0xFF;

    if (st == 0) { // monitor SA pin
        SA = (SA << 1) | SUART_PORT;
        if (SA == 0xfe) {
            SA_config = 1;
            SA_is_0 = 1;
            IE = 0xC2; // UART1 & Timer0 enabled, UART0 disabled
            st = 1;
        }
    } else {
        if (SA_Process()) {
            SA_config = 0;
            IE = 0xD2; // UART1 & Timer0 enabled, UART0 0 enable
            st = 0;
            SA = 0xFF;
        }
    }
    return st;
}
#elif defined USE_SMARTAUDIO_HW
uint8_t SA_task(void) {
    return 1 - SA_Process();
}
uint8_t SA_timeout(void) {
    if (timer_ms10x - sa_start_ms > 5000) {
        uart_set_baudrate(BAUDRATE);
        sa_status = SA_ST_IDLE;
        return 1;
    }
    return 0;
}
#endif

uint8_t SA_Process() {
    static uint8_t rx = 0;
    static uint8_t status = SA_HEADER0;
    static uint8_t cmd = 0;
    static uint8_t crc = 0;
    static uint8_t len = 0;
    static uint8_t index = 0;
    uint8_t ret = 0;

    if (SUART_ready()) {
        rx = SUART_rx();
        switch (status) {
        case SA_HEADER0:
            if (rx == SA_HEADER0_BYTE) {
                crc = crc8tab[rx]; // 0 ^ rx = rx
                index = 0;
                status = SA_HEADER1;
            } else {
                ret = 1;
            }
            break;

        case SA_HEADER1:
            if (rx == SA_HEADER1_BYTE) {
                crc = crc8tab[crc ^ rx];
                status = SA_CMD;
            } else {
                status = SA_HEADER0;
                ret = 1;
            }
            break;

        case SA_CMD:
            if (rx & 1) {
                cmd = rx >> 1;
                crc = crc8tab[crc ^ rx];
                status = SA_LENGTH;
            } else {
                status = SA_HEADER0;
                ret = 1;
            }
            break;

        case SA_LENGTH:
            len = rx;
            if (len > 16) {
                status = SA_HEADER0;
                ret = 1;
            } else {
                crc = crc8tab[crc ^ rx];
                if (len == 0)
                    status = SA_CRC;
                else {
                    status = SA_PAYLOAD;
                    index = 0;
                }
            }
            break;

        case SA_PAYLOAD:
            crc = crc8tab[crc ^ rx];
            sa_rbuf[index++] = rx;
            len--;
            if (len == 0)
                status = SA_CRC;
            break;

        case SA_CRC:
            if (crc == rx) {
                SA_lock = 1;
                SA_Update(cmd);
                SA_Response(cmd);
            }
            ret = 1;
            status = SA_HEADER0;
            break;

        default:
            ret = 1;
            status = SA_HEADER0;
            break;
        }
    }

    return ret;
}

void SA_Init() {
    uint16_t freq;
    SA_dbm = pwr_to_dbm(RF_POWER);
    SA_dbm_last = SA_dbm;
    mode_o |= PIT_MODE;
    mode_p |= (PIT_MODE << 1);

    ch_init = RF_FREQ;
    pwr_init = RF_POWER;

    ch_bf = channel_to_bfChannel(ch_init);
    freq = DM6300_GetFreqByChannel(ch_init);
    freq_new_h = (uint8_t)(freq >> 8);
    freq_new_l = (uint8_t)(freq & 0xff);
}

#endif
