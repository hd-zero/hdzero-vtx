#include "isr.h"
#include "global.h"
#include "uart.h"
#include "print.h"
#include "monitor.h"
#include "sfr_ext.h"
#include "hardware.h"
#include "i2c_device.h"
#include "rom.h"
#include "smartaudio_protocol.h"
#include "msp_displayport.h"
#include "dm6300.h"

uint8_t SA_lock = 0;
uint8_t SA_config = 0;
uint8_t SA_is_0 = 1;  // detect pre hreder 0x00

uint8_t pwr_init = 0; //0:POWER_MAX+2
uint8_t ch_init = 0;//0~9
uint8_t ch_bf = 0;

#ifdef USE_SMARTAUDIO

uint8_t sa_rbuf[8];
uint8_t SA_dbm = 14;
uint8_t SA_dbm_last;
uint8_t ch = 0;
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
    
uint8_t freq_new_h = 0x16;
uint8_t freq_new_l = 0x1a;
uint16_t freq = 0x161a;

uint8_t dbm_to_pwr(uint8_t dbm)
{
    if(dbm == 0)
        return POWER_MAX+2;
    else if(dbm == 14)      //25mw
        return 0;
    else if(dbm == 23)      //200mw
        return 1;
    #if (POWER_MAX > 1)
    else if(dbm == 27)      //500mw
        return 2;
    #endif
    #if (POWER_MAX > 2)
    else if(dbm == 30)      //1W
        return 3;
    #endif
    else
        return 0;
}

uint8_t pwr_to_dbm(uint8_t pwr)
{
    if(pwr == 0)                //25mw
        return 14;
    else if(pwr == 1)           //200mw
        return 23;
    #if (POWER_MAX > 1)
    else if(pwr == 2)           //500mw
        return 27;
    #endif
    #if (POWER_MAX > 2)
    else if(pwr == 3)           //1W
        return 30;
    #endif
    else if(pwr == POWER_MAX+2)
        return 0;
    else
        return 14;
}
void SA_Response(uint8_t cmd)
{
    uint8_t i,crc,tx_len;
    uint8_t tbuf[20];
    switch (cmd)
    {
    case SA_GET_SETTINGS:
        tbuf[0] = 0xAA;
        tbuf[1] = 0x55;
        tbuf[2] = 0x11;                 // version V2.1
        tbuf[3] = 0x0A+POWER_MAX;       // length
        tbuf[4] = ch_bf;                // channel
        tbuf[5] = dbm_to_pwr(SA_dbm);   // power level
        tbuf[6] = mode_o;               // operation mode
        tbuf[7] = freq_new_h;           // cur_freq_h
        tbuf[8] = freq_new_l;           // cur_freq_l
        tbuf[9] = SA_dbm;               // power dbm
        #ifdef VTX_L
        if(powerLock){
            tbuf[10] = 1 + 1;       // amount of power level
            for(i=0;i<=1;i++)
                tbuf[11+i] = pwr_to_dbm(i);
            tbuf[12+1] = 0;
        }else
        #endif
        tbuf[10] = POWER_MAX + 1;       // amount of power level
        for(i=0;i<=POWER_MAX;i++)
            tbuf[11+i] = pwr_to_dbm(i);
        tbuf[12+POWER_MAX] = 0;
    
        tx_len = tbuf[3] + 4;
        break;

    case SA_SET_PWR:
        tbuf[0] = 0xAA;
        tbuf[1] = 0x55;
        tbuf[2] = 2;
        tbuf[3] = 3;
        tbuf[4] = SA_dbm;
        tbuf[5] = 1; //reserved
        tx_len  = 7;
        break;

    case SA_SET_CH:
        tbuf[0] = 0xaa;
        tbuf[1] = 0x55;
        tbuf[2] = 3;
        tbuf[3] = 3;
        tbuf[4] = ch_bf;
        tbuf[5] = 1; //reserved
        tx_len  = 7;
        break;

    case SA_SET_FREQ:
        tbuf[0] = 0xaa;
        tbuf[1] = 0x55;
        tbuf[2] = 4;
        tbuf[3] = 4;
        tbuf[4] = freq_new_h;
        tbuf[5] = freq_new_l;
        tbuf[6] = 0x01; //reserved
        tx_len = 8;
        break;

    case SA_SET_MODE:
        tbuf[0] = 0xaa;
        tbuf[1] = 0x55;
        tbuf[2] = 5;
        tbuf[3] = 3;
        tbuf[4] = mode_p;
        tbuf[5] = 0x01; //reserved
        tx_len = 7;
        break;
        
    default: return;
    }//switch(cmd)
   
    //calculate CRC
    crc = 0;
    for(i=2;i<tx_len-1;i++)
        crc = crc8tab[crc ^ tbuf[i]];

    tbuf[tx_len-1] = crc;
    SUART_tx(tbuf,tx_len);
}


void SA_Update(uint8_t cmd)
{
    
    
    #ifdef DBG_SMARTAUDIO
    if(SUART_ready()){
        _outchar('^');
        return;
    }
    #endif
    switch (cmd) {
    case SA_GET_SETTINGS:
        #ifdef DBG_SMARTAUDIO
        _outchar('G');
        #endif
        break;

    case SA_SET_PWR:
        if(!(sa_rbuf[0] >> 7))
            return;
        SA_dbm = sa_rbuf[0] & 0x7f;
        
        #ifdef DBG_SMARTAUDIO
        Printf("dbm:%bx", SA_dbm);
        #endif
        
        if(SA_dbm_last != SA_dbm) {  // need to update power
            if(SA_dbm == 0) {     // Enter 0mW
                cur_pwr = POWER_MAX + 2;
                PIT_MODE = 0;
                vtx_pit = PIT_0MW;
                
                if(last_SA_lock && seconds < WAIT_SA_CONFIG){
                    pwr_init = cur_pwr;
                }else{
                    #ifdef _DEBUG_SA
                    Printf("\n\rEnter 0mW");
                    #endif
                    WriteReg(0, 0x8F, 0x10); // reset RF_chip
                    temp_err = 1;
                }
            }else if(SA_dbm_last == 0) { // Exit 0mW
                cur_pwr = dbm_to_pwr(SA_dbm);
                PIT_MODE = 0;
                RF_POWER = cur_pwr;
                
                if(last_SA_lock && seconds < WAIT_SA_CONFIG)
                    pwr_init = cur_pwr;
                else{
                    #ifdef _DEBUG_MDOE
                    Printf("\n\rExit 0mW");
                    #endif
                    
                    Init_6300RF(RF_FREQ, RF_POWER);
                    PIT_MODE = 0;
                    
                    DM6300_AUXADC_Calib();
                }
            }else{
                cur_pwr = dbm_to_pwr(SA_dbm);
                #ifdef VTX_L
                if(powerLock)
                    cur_pwr &= 0x01;
                #endif
                RF_POWER = cur_pwr;
                if(last_SA_lock && seconds < WAIT_SA_CONFIG)
                    pwr_init = cur_pwr;
                else{
            		#ifndef VIDEO_PAT
            		#ifdef VTX_L
                    if((RF_POWER == 3) && (!g_IS_ARMED))
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

    case SA_SET_CH:
        ch_bf = sa_rbuf[0];

        if(ch_bf == 25)
            ch = 8;
        else if(ch_bf == 27)
            ch= 9;
        else if(ch_bf >= 32 && ch_bf < 40)
            ch = ch_bf - 32;
        
        #ifdef DBG_SMARTAUDIO
        _outchar('C');
        Printf("%bx",ch);
        #endif
        
        if (ch != RF_FREQ) {
            RF_FREQ = ch;
            if(last_SA_lock && (seconds < WAIT_SA_CONFIG))
                ch_init = RF_FREQ;
            else
                DM6300_SetChannel(RF_FREQ);
            Setting_Save();
        }
        
        break;
    case SA_SET_FREQ:
        freq_new_h = sa_rbuf[0];
        freq_new_l = sa_rbuf[1];
        freq = freq_new_h;
        freq = freq << 8;
        freq += freq_new_l;
    
        if(freq == FREQ_R1)
            ch = 0;
        else if(freq == FREQ_R2)
            ch = 1;
        else if(freq == FREQ_R3)
            ch = 2;
        else if(freq == FREQ_R4)
            ch = 3;
        else if(freq == FREQ_R5)
            ch = 4;
        else if(freq == FREQ_R6)
            ch = 5;
        else if(freq == FREQ_R7)
            ch = 6;
        else if(freq == FREQ_R8)
            ch = 7;
        else if(freq == FREQ_F2)
            ch = 8;
        else if(freq == FREQ_F4)
            ch = 9;
        
        if(ch == 8)
            ch_bf = 25;
        else if(ch == 9)
            ch_bf = 27;
        else
            ch_bf = ch + 32;
        
        #ifdef DBG_SMARTAUDIO
        _outchar('F');
        Printf("%bx", ch);
        #endif
        
        if (ch != RF_FREQ) {
            RF_FREQ = ch;
            if(last_SA_lock && (seconds < WAIT_SA_CONFIG))
                ch_init = RF_FREQ;
            else
                DM6300_SetChannel(RF_FREQ);
            Setting_Save();
        }
        break;

    case SA_SET_MODE:
        if(mode_p != sa_rbuf[0]) {
            mode_p = sa_rbuf[0] & 0x07;
            
            if(mode_p & 0x04) {//deactive pitmode
                PIT_MODE = 0;
                mode_o &= 0x1d;
            }
            else {//active pitmode
                PIT_MODE = 2;
                mode_o |= 0x02;
            }
            if(cur_pwr == (POWER_MAX+2)) return;

            if(PIT_MODE){
                if(last_SA_lock && seconds < WAIT_SA_CONFIG)
                    pwr_init = cur_pwr;
                else if(dbm_to_pwr(SA_dbm) == (POWER_MAX+2)){
                    cur_pwr = POWER_MAX + 2;
                    PIT_MODE = 0;
                    vtx_pit = PIT_0MW;
                    #ifdef _DEBUG_SA
                    Printf("\n\rSA:Enter 0mW");
                    #endif
                    WriteReg(0, 0x8F, 0x10); // reset RF_chip
                    temp_err = 1;
                }else{
                    DM6300_SetPower(POWER_MAX+1, RF_FREQ, 0);
                    cur_pwr = POWER_MAX + 1;
                }
            }else{
                if(last_SA_lock && seconds < WAIT_SA_CONFIG)
                    pwr_init = cur_pwr;
                else if(dbm_to_pwr(SA_dbm) == (POWER_MAX+2)){
                    cur_pwr = POWER_MAX + 2;
                    PIT_MODE = 0;
                    vtx_pit = PIT_0MW;
                    #ifdef _DEBUG_SA
                    Printf("\n\rSA:Enter 0mW");
                    #endif
                    WriteReg(0, 0x8F, 0x10); // reset RF_chip
                    temp_err = 1;
                }else{
            		#ifndef VIDEO_PAT
            		#ifdef VTX_L
                    if((RF_POWER == 3) && (!g_IS_ARMED)){
                        pwr_lmt_done = 0;
                        cur_pwr = 3;
                        #ifdef _DEBUG_SA
                        Printf("\r\npwr_lmt_done reset");
                        #endif
                    }
                    else
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
        #ifdef DBG_SMARTAUDIO
        Printf("M_P:%bx  ", mode_p);
        Printf("M_O:%bx", mode_o);
        #endif
        break;

    default:
        #ifdef DBG_SMARTAUDIO
        _outchar('#');
        #endif
        break;
    }
    //_outchar('_');
}

uint8_t SA_task()
{
	static uint8_t SA_state=0,SA=0xFF; 

	if(SA_state == 0) {  //monitor SA pin
		SA = (SA<<1) | SUART_PORT;
		if(SA == 0xfe) {
            SA_config = 1;
            SA_is_0 = 1;
			IE = 0xC2; //UART1 & Timer0 enabled, UART0 disabled
			SA_state = 1;
            #ifdef DBG_SMARTAUDIO
            _outchar('\n');
            _outchar('\r');
            _outchar('<');
            #endif
		}
	}
	else {
		if(SA_Process()) {
            SA_config = 0;
			IE =  0xD2; //UART1 & Timer0 enabled, UART0 0 enable
			SA_state = 0;
            SA = 0xFF;
            #ifdef DBG_SMARTAUDIO
            _outchar('>');
            #endif
		}
	}
	return SA_state;
}


uint8_t SA_Process() {
    static uint8_t rx = 0;
    static uint8_t sa_status = SA_HEADER0;
    static uint8_t cmd = 0;
    static uint8_t crc = 0;
    static uint8_t len = 0;
    static uint8_t index = 0;
    uint8_t ret = 0;
    
    if(SUART_ready()) {
        rx = SUART_rx();
        #ifdef DBG_SMARTAUDIO
        //Printf("%bx ",rx);
        #endif
        switch (sa_status) {
        case SA_HEADER0:
            if (rx == SA_HEADER0_BYTE) {
                crc = crc8tab[rx]; // 0 ^ rx = rx
                index = 0;
                sa_status = SA_HEADER1;
            }
            else{
                ret = 1;
            }
            break;

        case SA_HEADER1:
            if (rx == SA_HEADER1_BYTE) {
                crc = crc8tab[crc ^ rx];
                sa_status = SA_CMD;
            }
            else {
                sa_status = SA_HEADER0;
                ret = 1;
            }
            break;
            
        case SA_CMD:
            if(rx & 1) {
                cmd = rx >> 1;
                crc = crc8tab[crc ^ rx];
                sa_status = SA_LENGTH;
            }
            else {
                sa_status = SA_HEADER0;
                ret = 1;
            }
            break;

        case SA_LENGTH:
            len = rx;
            if (len > 16) {
                sa_status = SA_HEADER0;
                ret = 1;
            }
            else {
                crc = crc8tab[crc ^ rx];
                if (len == 0)
                    sa_status = SA_CRC;
                else {
                    sa_status = SA_PAYLOAD;
                    index = 0;
                }
            }
            break;

        case SA_PAYLOAD:
            crc = crc8tab[crc ^ rx];
            sa_rbuf[index++] = rx;
            len--;
            if(len == 0)
                sa_status = SA_CRC;
            break;

        case SA_CRC:
            if(crc == rx) {
                SA_lock = 1;
                SA_Update(cmd);
                SA_Response(cmd);
            }
            else {
                #ifdef DBG_SMARTAUDIO
                _outchar('$');
                #endif
            }
            ret = 1;
            sa_status = SA_HEADER0;
            break;

        default:
            ret = 1;
            sa_status = SA_HEADER0;
            break;
        }
    }
    else{
        #ifdef DBG_SMARTAUDIO
        _outchar('.');
        #endif
    }
    return ret;
}

void SA_Init() {
    SA_dbm = pwr_to_dbm(RF_POWER);
    SA_dbm_last = SA_dbm;
    ch = RF_FREQ;
    if(ch==8)
        ch_bf = 25;
    else if(ch==9)
        ch_bf = 27;
    else
        ch_bf = ch + 32;
    mode_o |= PIT_MODE;
    mode_p |= (PIT_MODE<<1);
    
    switch(ch){
        case 0: freq = FREQ_R1; break;
        case 1: freq = FREQ_R2; break;
        case 2: freq = FREQ_R3; break;
        case 3: freq = FREQ_R4; break;
        case 4: freq = FREQ_R5; break;
        case 5: freq = FREQ_R6; break;
        case 6: freq = FREQ_R7; break;
        case 7: freq = FREQ_R8; break;
        case 8: freq = FREQ_F2; break;
        case 9: freq = FREQ_F4; break;
        default: freq = FREQ_R1; break;
    }
    freq_new_h = (uint8_t)(freq >> 8);
    freq_new_l = (uint8_t)(freq & 0xff);
}

#endif
