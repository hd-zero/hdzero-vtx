#include "monitor.h"

#include "camera.h"
#include "common.h"
#include "dm6300.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "i2c_device.h"
#include "print.h"
#include "sfr_ext.h"
#include "spi.h"
#include "uart.h"

///////////////////////////////////////////////
// Global variables for this module only.
// DO NOT TRY to use it outside the scope
XDATA_SEG uint8_t incnt = 0;           // in char count
XDATA_SEG uint8_t monstr[MAX_CMD_LEN]; // buffer for input string
XDATA_SEG uint8_t *argv[7];            // command line arguments
XDATA_SEG uint8_t argc = 0;            // command line cnt
XDATA_SEG uint8_t last_argc = 0;

XDATA_SEG uint8_t comment = 0;
BIT_TYPE echo = 1;
BIT_TYPE verbose = 1;

#ifdef _DEBUG_MODE
void MonHelp(void) {
    debugf("\r\nUsage: ");
    debugf("\r\n   w   addr  wdat   : Write reg_map register");
    debugf("\r\n   r   addr         : Read  reg_map register");
    debugf("\r\n   w2  addr  wdat   : Write page 2 reg_map register");
    debugf("\r\n   r2  addr         : Read  page 2 reg_map register");
    debugf("\r\n   ww  addr  wdat   : Write ad936x register");
    debugf("\r\n   rr  addr         : Read  ad936x register");
    debugf("\r\n   pat chan  pwr    : chan: 0~7, pwr: 0-2 [chan=-1, pat off]");
    debugf("\r\n   c                : Init DM6300");
    debugf("\r\n   /                : Repeat last command");
    debugf("\r\n   ; anything       : Comment");
    debugf("\r\n   v                : verbose mode on/off");
    debugf("\r\n   ch  freq  pwr    : change freq and power");
    debugf("\r\n   ew  wdat         : Write rf_tab[freq][pwr] with wdat");
    debugf("\r\n   er               : Read  rf_tab[freq][pwr]");
    debugf("\r\n   ea               : rf_tab[freq][pwr]++");
    debugf("\r\n   es               : rf_tab[freq][pwr]--");
    debugf("\r\n   h                : Help");
    debugf("\r\n");
}
#endif

#ifdef _DEBUG_MODE
uint8_t MonGetCommand(void) {
    uint8_t i, ch;
    uint8_t ret = 0;

    if (!Mon_ready())
        return 0;
    ch = Mon_rx();

    //----- if comment, echo back and ignore -----
    if (comment) {
        if (ch == '\r' || ch == 0x1b)
            comment = 0; // 0x1b = esc
        else {
            Mon_tx(ch);
            return 0;
        }
    } else if (ch == ';') {
        comment = 1;
        Mon_tx(ch);
        return 0;
    }

    //=====================================
    switch (ch) {

    case 0x1b: // ESC
        argc = 0;
        incnt = 0;
        comment = 0;
        Prompt();
        return 0;

    //--- end of string
    case '\r':

        if (incnt == 0) {
            Prompt();
            break;
        }

        monstr[incnt++] = '\0';
        argc = 0;
        for (i = 0; i < incnt; i++)
            if (monstr[i] != ' ')
                break;

        if (!monstr[i]) {
            incnt = 0;
            comment = 0;
            Prompt();
            return 0;
        }

        argv[0] = &monstr[i];
        for (; i < incnt; i++) {
            if (monstr[i] == ' ' || monstr[i] == '\0') {
                monstr[i] = '\0';
                // debugf("(%s) ",  argv[argc]);
                i++;
                while (monstr[i] == ' ')
                    i++;
                argc++;
                if (monstr[i]) {
                    argv[argc] = &monstr[i];
                }
            }
        }

        ret = 1;
        last_argc = argc;
        incnt = 0;

        break;

    //--- back space
    case 0x08:
        if (incnt) {
            incnt--;
            Mon_tx(ch);
            Mon_tx(' ');
            Mon_tx(ch);
        }
        break;

    //--- repeat command
    case '/':
        argc = last_argc;
        ret = 1;
        break;

    default:
        if (incnt < MAX_CMD_LEN) {
            Mon_tx(ch);
            monstr[incnt++] = ch;
        }
        break;
    }

    if (ret) {
        comment = 0;
        last_argc = argc;
        return ret;
    } else {
        return ret;
    }
}
#endif

#ifdef _DEBUG_MODE
void MonEE(uint8_t op, uint8_t d) {
    uint8_t val;
    uint8_t addr;

    addr = RF_FREQ * (POWER_MAX + 1) + RF_POWER;
    val = I2C_Read8(ADDR_EEPROM, addr);

    switch (op) {
    case 0: // ew
        I2C_Write8_Wait(10, ADDR_EEPROM, addr, d);
        break;

    case 1: // er
        break;

    case 2: // ea
        if (val != 0xFF)
            val++;
        I2C_Write8_Wait(10, ADDR_EEPROM, addr, val);
        break;

    case 3: // es
        if (val != 0)
            val--;
        I2C_Write8_Wait(10, ADDR_EEPROM, addr, val);
        break;
    }

    val = I2C_Read8_Wait(10, ADDR_EEPROM, addr);
    table_power[RF_FREQ][RF_POWER] = val;
    debugf("\r\nRF TAB[%d][%d] = %x", (uint16_t)RF_FREQ, (uint16_t)RF_POWER, val);

    DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
}
#endif

void Monitor(void) {
#ifdef _DEBUG_MODE
    if (!MonGetCommand())
        return;

    if (!stricmp(argv[0], "w"))
        MonWrite(0);
    else if (!stricmp(argv[0], "r"))
        MonRead(0);
    else if (!stricmp(argv[0], "w2"))
        MonWrite(1);
    else if (!stricmp(argv[0], "r2"))
        MonRead(1);
    else if (!stricmp(argv[0], "ww"))
        MonWrite(2);
    else if (!stricmp(argv[0], "rr"))
        MonRead(2);
    else if (!stricmp(argv[0], "pat"))
        chg_vtx();
    else if (!stricmp(argv[0], "c"))
        Init_6300RF(RF_FREQ, RF_POWER);
    else if (!stricmp(argv[0], "rfrst")) {
        WriteReg(0, 0x8F, 0x00);
        WriteReg(0, 0x8F, 0x11);
    } else if (!stricmp(argv[0], "rf1")) {
        // WriteReg(0, 0x8F, 0x00);
        // WriteReg(0, 0x8F, 0x11);
        DM6300_init1();
    } else if (!stricmp(argv[0], "rf2"))
        DM6300_init2(0);
    else if (!stricmp(argv[0], "rf3"))
        DM6300_init3(RF_FREQ);
    else if (!stricmp(argv[0], "rf4"))
        DM6300_init4();
    else if (!stricmp(argv[0], "rf5"))
        DM6300_init5();
    else if (!stricmp(argv[0], "rf6"))
        DM6300_init6(0);
    else if (!stricmp(argv[0], "rf7"))
        DM6300_init7(0);
    else if (!stricmp(argv[0], "efuse1")) {
        DM6300_EFUSE1();
        SPI_Write(0x6, 0xFF0, 0x00000018);
    } else if (!stricmp(argv[0], "efuse2")) {
        DM6300_EFUSE2();
        SPI_Write(0x6, 0xFF0, 0x00000018);
    } else if (!stricmp(argv[0], "rftest"))
        DM6300_RFTest();
    else if (!stricmp(argv[0], "bbon"))
        WriteReg(0, 0x8F, 0x11);
    else if (!stricmp(argv[0], "bboff"))
        WriteReg(0, 0x8F, 0x01);
    // else if ( !stricmp( argv[0], "m0" ) )
    // DM6300_M0();
    else if (!stricmp(argv[0], "ch")) {
        if (argc == 3 && Asc2Bin(argv[1]) <= FREQ_MAX && Asc2Bin(argv[2]) <= POWER_MAX) {
            RF_FREQ = Asc2Bin(argv[1]);
            RF_POWER = Asc2Bin(argv[2]);
            DM6300_SetChannel(RF_FREQ);
            DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
        }
    } else if (!stricmp(argv[0], "ew"))
        MonEE(0, Asc2Bin(argv[1]));
    else if (!stricmp(argv[0], "er"))
        MonEE(1, 0);
    else if (!stricmp(argv[0], "ea"))
        MonEE(2, 0);
    else if (!stricmp(argv[0], "es"))
        MonEE(3, 0);
    else if (!stricmp(argv[0], "dc")) {
        if (argc == 5) {
            I2C_Write8_Wait(10, ADDR_EEPROM, 0x88, Asc2Bin(argv[1]));
            I2C_Write8_Wait(10, ADDR_EEPROM, 0x89, Asc2Bin(argv[2]));
            I2C_Write8_Wait(10, ADDR_EEPROM, 0x8a, Asc2Bin(argv[3]));
            I2C_Write8_Wait(10, ADDR_EEPROM, 0x8b, Asc2Bin(argv[4]));
            // debugf("\r\nWrite in eeprom, 0x88=%x,0x89=%x,0x8a=%x,0x8b=%x",
            // Asc2Bin(argv[1]),Asc2Bin(argv[2]),Asc2Bin(argv[3]),Asc2Bin(argv[4]));
        } else
            debugf("   --> missing parameter!");
    } else if (!stricmp(argv[0], "iq")) {
        if (argc == 5) {
            I2C_Write8_Wait(10, ADDR_EEPROM, 0x8c, Asc2Bin(argv[1]));
            I2C_Write8_Wait(10, ADDR_EEPROM, 0x8d, Asc2Bin(argv[2]));
            I2C_Write8_Wait(10, ADDR_EEPROM, 0x8e, Asc2Bin(argv[3]));
            I2C_Write8_Wait(10, ADDR_EEPROM, 0x8f, Asc2Bin(argv[4]));
            debugf("\r\nWrite in eeprom, 0x88=%x,0x89=%x,0x8a=%x,0x8b=%x",
                   (uint16_t)Asc2Bin(argv[1]), (uint16_t)Asc2Bin(argv[2]), (uint16_t)Asc2Bin(argv[3]), (uint16_t)Asc2Bin(argv[4]));
        } else
            debugf("   --> missing parameter!");
    } else if (!stricmp(argv[0], "v")) {
        verbose = !verbose;
        if (verbose)
            debugf("\r\nVerbose on");
        else
            debugf("\r\nVerbose off");
    } else if (!stricmp(argv[0], "h"))
        MonHelp();
    else
        debugf("\r\nInvalid Command...");

    Prompt();
#endif
}

#ifdef _DEBUG_MODE
void MonWrite(uint8_t mode) {
    uint16_t addr = 0;
    uint8_t value;

    uint8_t spi_trans = 0;
    uint16_t spi_addr = 0;
    uint32_t spi_data_L;

    uint16_t spi_data_H_h = 0;
    uint16_t spi_data_H_l = 0;
    uint16_t spi_data_L_h = 0;
    uint16_t spi_data_L_l = 0;

    if (mode < 2) {
        if (argc < 3) {
            debugf("   --> missing parameter!");
            return;
        }
        addr = Asc4Bin(argv[1]);
        value = Asc2Bin(argv[2]);

        WriteReg(mode, (uint8_t)addr, value);
    } else {
        if (argc < 4) {
            debugf("   --> missing parameter!");
            return;
        }
        spi_trans = Asc2Bin(argv[1]);
        spi_addr = Asc4Bin(argv[2]);
        // spi_data_H = Asc8Bin( argv[3] );
        spi_data_L = Asc8Bin(argv[3]);
        SPI_Write(spi_trans, spi_addr, spi_data_L);
    }

    if (echo) {
        if (mode < 2) {
            value = ReadReg(mode, (uint8_t)addr);
            debugf("\r\nRead %2xh: %2xh ", (uint16_t)addr, (uint16_t)value);
        } else {
            spi_data_L = 0;
            SPI_Read(spi_trans, spi_addr, &spi_data_L);

            spi_data_L_l = spi_data_L & 0xffff;
            spi_data_L_h = (spi_data_L >> 16) & 0xffff;

            debugf("\r\nRead %x: %x%x", (uint16_t)spi_addr,
                   (uint16_t)spi_data_L_h, (uint16_t)spi_data_L_l);
        }
    }
}

void MonRead(uint8_t mode) {
    uint16_t addr;
    uint8_t value;

    uint8_t spi_trans;
    uint16_t spi_addr;
    uint32_t spi_data_H = 0;
    uint32_t spi_data_L = 0;

    uint16_t spi_data_H_h = 0;
    uint16_t spi_data_H_l = 0;
    uint16_t spi_data_L_h = 0;
    uint16_t spi_data_L_l = 0;

    if (mode < 2) {
        if (argc < 2) {
            debugf("   --> missing parameter!");
            return;
        }
        addr = Asc4Bin(argv[1]);
        value = ReadReg(mode, (uint8_t)addr);
        debugf("\r\nRead %3xh: %2xh ", (uint16_t)addr, (uint16_t)value);
    } else {
        if (argc < 3) {
            debugf("   --> missing parameter!");
            return;
        }
        spi_trans = Asc2Bin(argv[1]);
        spi_addr = Asc4Bin(argv[2]);
        SPI_Read(spi_trans, spi_addr, &spi_data_L);

        spi_data_L_l = spi_data_L & 0xffff;
        spi_data_L_h = (spi_data_L >> 16) & 0xffff;

        debugf("\r\nRead %x: %x%x", (uint16_t)spi_addr,
               (uint16_t)spi_data_L_h, (uint16_t)spi_data_L_l);
    }
}

void chg_vtx(void) {
    uint8_t chan, pwr;
    uint8_t t0 = 0, t1 = 1, t2 = 2;

    if (argc < 2) {
        debugf("   --> missing parameter!");
        return;
    }

    chan = Asc2Bin(argv[1]);
    pwr = Asc2Bin(argv[2]);

    if (!stricmp(argv[1], "-1")) { // Camera
        WriteReg(0, 0x50, 0x00);
        DM6300_SetChannel(RF_FREQ);
        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
        debugf("\r\nVTX Cam, channel = %d,power = %d,FPS = %d", (uint16_t)RF_FREQ, (uint16_t)RF_POWER, (uint16_t)video_format);
    } else { // pattern
        Set_720P60(0);
        video_format = VDO_FMT_720P60;
        WriteReg(0, 0x50, 0x01);

        RF_POWER = pwr;
        RF_FREQ = chan;
        DM6300_SetChannel(RF_FREQ);
        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
        debugf("\r\nVTX Pattern, channel = %d,power = %d,FPS = 1", (uint16_t)RF_FREQ, (uint16_t)RF_POWER);
    }
}
#endif
