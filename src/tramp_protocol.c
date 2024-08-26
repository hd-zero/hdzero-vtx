#include "tramp_protocol.h"
#include "dm6300.h"
#include "global.h"
#include "hardware.h"
#include "msp_displayport.h"
#include "print.h"

uint8_t tr_tx_busy = 0;
uint8_t tramp_lock = 0;

#ifdef USE_TRAMP
static trampReceiveState_e trampReceiveState = S_WAIT_LEN;
static uint8_t tbuf[16];
static uint8_t rbuf[16];
static uint8_t r_ptr;

// Calculate tramp protocol checksum of provided buffer
static uint8_t tramp_checksum(uint8_t *buf) {
    uint8_t crc = 0;
    uint8_t i;

    for (i = 1; i < 14; i++) {
        crc += buf[i];
    }

    return crc;
}

void tramp_reset_receive(void) {
    trampReceiveState = S_WAIT_LEN;
    r_ptr = 0;
}

void trampResponse(void) {
    uint8_t i;

    tbuf[14] = tramp_checksum(tbuf);
    tbuf[15] = 0x00;

    tr_tx_busy = 1;

    for (i = 0; i < 16; i++) {
        tr_tx(tbuf[i]);
    }

    WAIT(1);
    tr_tx_busy = 0;
    tr_read(); // fix me!
}

static void set_freq(uint16_t freq) {
    uint8_t ch = DM6300_GetChannelByFreq(freq);

    if (ch != 0xff) {
        if (ch > 9 && lowband_lock) {
            ;
        } else {
            RF_FREQ = ch;
            if (dm6300_init_done) {
                DM6300_SetChannel(RF_FREQ);
            }
        }
    }
}

static uint16_t get_power(void) {
    uint16_t power = 0;

    switch (RF_POWER) {
    case 0:
        power = 25;
        break;
    case 1:
        power = 200;
        break;
    case 2:
        power = 0;
        break;
    default:
        power = 0;
        break;
    }

    return power;
}

static void set_power(uint16_t power) {
    switch (power) {
    case 0:
        RF_POWER = POWER_MAX + 2;
        break;
    case 1:
        RF_POWER = POWER_MAX + 1;
        break;

    case 14: // dBm
    case 25: // mw
        RF_POWER = 0;
        break;

    case 23: // dBm
    case 100:
    case 200:
    case 350:
    case 1000:
        RF_POWER = 1;
        break;

    default:
        break;
    }
    if (RF_POWER == POWER_MAX + 2) {
        // enter 0mw
        cur_pwr = RF_POWER;
        vtx_pit = PIT_0MW;

        WriteReg(0, 0x8F, 0x10); // reset RF_chip
        dm6300_init_done = 0;
        temp_err = 1;
    } else {

        if (dm6300_init_done) {
            DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);

        } else {
            Init_6300RF(RF_FREQ, RF_POWER);
            DM6300_AUXADC_Calib();
            dm6300_init_done = 1;
        }
        cur_pwr = RF_POWER;
        if (RF_POWER == POWER_MAX + 1)
            vtx_pit = PIT_P1MW;
    }
}

// Process response and return code if valid else 0
static uint8_t tramp_reply(void) {
    const uint8_t respCode = rbuf[1];
    uint16_t freq = DM6300_GetFreqByChannel(RF_FREQ);
    uint16_t power = get_power();
    const uint8_t locked = 0;
    uint8_t pitmode = 0;

    memset(tbuf, 0x00, 16);
    switch (respCode) {
    case 'r': {
        tbuf[0] = 0x0f;
        tbuf[1] = 'r';
        tbuf[2] = FREQ_L1 & 0xff;
        tbuf[3] = FREQ_L1 >> 8;
        tbuf[4] = FREQ_R8 & 0xff;
        tbuf[5] = FREQ_R8 >> 8;
        tbuf[6] = 200 & 0xff;
        tbuf[7] = 200 >> 8;

        trampResponse();
        return respCode;
    }

    case 'v': {
        tbuf[0] = 0x0f;
        tbuf[1] = 'v';
        tbuf[2] = freq & 0xff;
        tbuf[3] = freq >> 8;
        tbuf[4] = power & 0xff;
        tbuf[5] = power >> 8;
        tbuf[6] = locked;
        tbuf[7] = pitmode;
        tbuf[8] = power & 0xff;
        tbuf[9] = power >> 8;

        trampResponse();
        return respCode;
    }

    case 'F': {
        freq = ((uint16_t)rbuf[2] & 0xff) | ((uint16_t)rbuf[3] << 8);
        set_freq(freq);
        return respCode;
    }

    case 'P': {
        power = ((uint16_t)rbuf[2] & 0xff) | ((uint16_t)rbuf[3] << 8);
        set_power(power);
        return respCode;
    }

    case 's': {
        tbuf[0] = 0x0f;
        tbuf[1] = 's';
        tbuf[6] = 0x01;
        tbuf[7] = 0x00;

        trampResponse();
        return respCode;
    }

    default:
        break;
    }
    return 0;
}
// returns completed response code or 0

void tramp_receive(void) {
    uint8_t rdata = 0;
    while (tr_ready()) {
        rdata = tr_read();
        rbuf[r_ptr++] = rdata;

        switch (trampReceiveState) {
        case S_WAIT_LEN: {
            if (rdata == 0x0F) {
                // Found header byte, advance to wait for code
                trampReceiveState = S_WAIT_CODE;
            } else {
                // Unexpected header, reset state machine
                tramp_reset_receive();
            }
            break;
        }
        case S_WAIT_CODE: {

            trampReceiveState = S_DATA;
            break;
        }
        case S_DATA: {
            if (r_ptr == 16) {
                // Buffer is full, calculate checksum
                if ((rbuf[14] == tramp_checksum(rbuf)) && (rbuf[15] == 0)) {
                    tramp_lock = 1;
                    tramp_reply();
                }

                // Reset state machine ready for next response
                tramp_reset_receive();
            }
            break;
        }
        default: {
            // Invalid state, reset state machine
            tramp_reset_receive();
            break;
        }
        }
    }
}

void tramp_init(void) {
    uint16_t time_ms = 3000;
    RF_POWER = POWER_MAX + 2;
    while (time_ms--) {
        timer_task();
        tramp_receive();
    }
    if (!tramp_lock)
        RF_POWER = I2C_Read8(ADDR_EEPROM, EEP_ADDR_RF_POWER);
}
#endif