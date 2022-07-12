#include "common.h"
#include "hardware.h"
#include "print.h"
#include "global.h"
#include "monitor.h"
#include "i2c.h"
#include "i2c_device.h"
#include "dm6300.h"
#include "spi.h"

int16_t auxadc_offset = 0;
uint32_t init6300_fcnt = 0;
uint32_t init6300_fnum[FREQ_MAX_EXT+1] = {0};

uint32_t dcoc_ih = 0x075F0000;
uint32_t dcoc_qh = 0x075F0000;

#ifdef VTX_L
uint8_t table_power[FREQ_MAX_EXT+1][POWER_MAX+1] = {
    {0x70, 0x68, 0x5c, 0x60},         
    {0x70, 0x68, 0x5c, 0x60},         
    {0x70, 0x68, 0x60, 0x60},
    {0x72, 0x6d, 0x60, 0x60},         
    {0x74, 0x70, 0x62, 0x5c},         
    {0x78, 0x74, 0x64, 0x5b},         
    {0x7a, 0x77, 0x64, 0x5b},
    {0x7a, 0x77, 0x64, 0x5b},
    {0x72, 0x6d, 0x60, 0x60},  //ch9-5760        
    {0x74, 0x70, 0x62, 0x5c}}; //ch10-5800
#else
uint8_t table_power[FREQ_MAX_EXT+1][POWER_MAX+1] = {
    {0x79, 0x83},         
    {0x77, 0x81},         
    {0x75, 0x80},
    {0x73, 0x7E},         
    {0x72, 0x7C},         
    {0x70, 0x7B},         
    {0x72, 0x7E},
    {0x71, 0x7C},
    {0x73, 0x7E},  //ch9-5760        
    {0x72, 0x7C}}; //ch10-5800
#endif
#ifndef Raceband
    //ch1-8,5660M/5695M/5735M/5770M/5805M/5839M/5878M/5914M 
    uint32_t tab[3][FREQ_MAX+1] = {
        {  0x3746,   0x379D,   0x3801,   0x3859,   0x38B0,   0x3905,   0x3967,   0x39C1},
        {    0x93,     0x94,     0x95,     0x96,     0x97,     0x98,     0x99,     0x9A},
        {0xCAAAAB, 0x9D5555, 0xB2AAAB, 0x855555, 0x580000, 0x1D5555, 0x255555,  0x55555}
    };
#else
//Raceband1-8,5658M/5695M/5732M/5769M/5806M/5843M/5880M/5917M/5760M/5800M    
    uint32_t tab[3][FREQ_MAX_EXT+1] = {
        {  0x3867,   0x379D,   0x3924,   0x3982,   0x39E1,   0x3A3F,   0x3A9E,   0x3AFC,   0x3840,   0x38A4},
        {    0x93,     0x94,     0x95,     0x96,     0x97,     0x98,     0x99,     0x9a,     0x96,     0x97},
        {0xB00000, 0x9D5555, 0x8AAAAB, 0x780000, 0x655555, 0x52AAAB, 0x400000, 0x2D5555, 0x000000, 0x155555}
    };
#endif        
    
void DM6300_SetChannel(uint8_t ch)
{
    #ifdef _DEBUG_MODE
    Printf("\r\nset ch:%bx",ch);
    #endif
    
    if(ch > 9) ch = 0;
/*#ifndef _RF_CALIB
#ifndef _DEBUG_MODE
    else if(ch == 5) ch = 7;
    else if(ch == 6) ch = 5;
    else if(ch == 7) ch = 6;
#endif
#endif
*/   
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    
    SPI_Write(0x3, 0x2B0, 0x00000000, 0x00077777);
    SPI_Write(0x3, 0x030, 0x00000000, 0x00000013);
    SPI_Write(0x3, 0x034, 0x00000000, 0x00000013);
    SPI_Write(0x3, 0x038, 0x00000000, 0x00000370);
    SPI_Write(0x3, 0x03C, 0x00000000, 0x00000410);
    SPI_Write(0x3, 0x040, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x044, 0x00000000, 0x0D640735);
    SPI_Write(0x3, 0x048, 0x00000000, 0x01017F03);
    SPI_Write(0x3, 0x04C, 0x00000000, 0x021288A2);
    SPI_Write(0x3, 0x050, 0x00000000, 0x00FFCF33);
    SPI_Write(0x3, 0x054, 0x00000000, 0x1F3C3440);
    SPI_Write(0x3, 0x028, 0x00000000, 0x00008000 + (init6300_fcnt & 0xFF));
    SPI_Write(0x3, 0x020, 0x00000000, init6300_fnum[ch]); //tab[0][ch] WAIT(1);
    SPI_Write(0x3, 0x01C, 0x00000000, 0x00000002);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000001); //WAIT(1);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000000); //WAIT(1);
    SPI_Write(0x3, 0x028, 0x00000000, 0x00008000 + (init6300_fcnt & 0xFF));
    SPI_Write(0x3, 0x020, 0x00000000, init6300_fnum[ch]); //tab[0][ch]
    SPI_Write(0x3, 0x01C, 0x00000000, 0x00000003);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000001); //WAIT(1);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000000); //WAIT(1);
    SPI_Write(0x3, 0x050, 0x00000000, 0x00FFCFB3);
    SPI_Write(0x3, 0x004, 0x00000000, tab[1][ch]);
    SPI_Write(0x3, 0x008, 0x00000000, tab[2][ch]);
    SPI_Write(0x3, 0x000, 0x00000000, 0x00000000); //WAIT(1);
    SPI_Write(0x3, 0x000, 0x00000000, 0x00000003);
    SPI_Write(0x3, 0x050, 0x00000000, 0x000333B3);
    SPI_Write(0x3, 0x040, 0x00000000, 0x07070002);
    SPI_Write(0x3, 0x030, 0x00000000, 0x00000010);
}

void DM6300_SetPower(uint8_t pwr, uint8_t freq, uint8_t offset)
{
	#ifdef VTX_L
    uint16_t a_tab[4] = {0x204, 0x11F, 0x21F, 0x31F};  
    #else
    uint16_t a_tab[2] = {0x21F, 0x41F};
    #endif
    int16_t p;
    #ifdef _DEBUG_MODE
    Printf("\r\nDM6300 set power:%bx",pwr);
    #endif
    if(freq > 9) freq = 0;
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    
    if(pwr == POWER_MAX+1){
        SPI_Write(0x3, 0x330, 0x00000000, 0x21F);
        SPI_Write(0x3, 0xD1C, 0x00000000, PIT_POWER);
    }
    else {
        SPI_Write(0x3, 0x330, 0x00000000, a_tab[pwr]);
        
#ifndef _RF_CALIB
        if(RF_POWER == 0 && pwr == 0){
            p = table_power[freq][pwr] + offset - 2;
            
            if(OFFSET_25MW<=10)
                p += OFFSET_25MW;
            else if (p < (OFFSET_25MW - 10))
                p = 0;
            else
                p = p + 10 - OFFSET_25MW;
        }
        else
#endif
            p = table_power[freq][pwr] + offset - 2;
        
        if(p > 255) p = 255;
        else if(p < 0) p = 0;
        SPI_Write(0x3, 0xD1C, 0x00000000, (uint8_t)p);
    }
 
    #ifdef _DEBUG_MODE
    Printf("\r\nDM6300 SetPower done.  %bx, %x", table_power[freq][pwr], offset);
    #endif
}

void DM6300_SetSingleTone(uint8_t enable)
{
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    
    if(enable){
        SPI_Write(0x3, 0x08C, 0x00000000, 0x18040000);
        SPI_Write(0x3, 0x0C0, 0x00000000,          2);
        SPI_Write(0x3, 0x0BC, 0x00000000, 0x5ED097B4);
    }
    else{
        SPI_Write(0x3, 0x08C, 0x00000000,          0);
        SPI_Write(0x3, 0x0C0, 0x00000000,          0);
        SPI_Write(0x3, 0x0BC, 0x00000000,          0);
    }
}

void DM6300_RFTest()
{
    uint16_t i;
    for(i = 0; i<100; i++)
        SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
}

void DM6300_InitAUXADC()
{
    uint32_t dh,dl;
    int16_t dat1,dat2,dat3;
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Read (0x3, 0x254, &dh, &dl);
    #ifdef _DEBUG_MODE
    Printf("\r\nDM6300 0x254 = %x%x.\r\n",(uint16_t)((dl>>16)&0xFFFF), (uint16_t)(dl&0xFFFF));
    #endif
    dl |= 0x200;
    #ifdef _DEBUG_MODE
    Printf("\r\nDM6300 0x254 = %x%x.\r\n",(uint16_t)((dl>>16)&0xFFFF), (uint16_t)(dl&0xFFFF));
    #endif
    SPI_Write(0x3, 0x254, 0x00000000, dl);
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    SPI_Write(0x3, 0x17C, 0x00000000, 0x19);
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0x2A0, 0x00000000, 0xC05B55FE);
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    SPI_Read (0x3, 0x17C, &dh, &dl);
    dat1 = ((int32_t)dl) >> 20;
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0x2A0, 0x00000000, 0x305B55FE);
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    SPI_Read (0x3, 0x17C, &dh, &dl);
    dat2 = ((int32_t)dl) >> 20;
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0x2A0, 0x00000000, 0xA05B51FE);
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    SPI_Read (0x3, 0x17C, &dh, &dl);
    dat3 = ((int32_t)dl) >> 20;
    
    auxadc_offset = dat3 - ((dat1+dat2) >> 1);
    #ifdef _DEBUG_MODE
    Printf("\r\nDM6300 AUXADC Calib done. data1=%x, data2=%x, data3=%x, offset=%x", dat1, dat2, dat3, auxadc_offset);
    #endif
}

void DM6300_AUXADC_Calib()
{
    WriteReg(0, 0x8F, 0x01);
    DM6300_InitAUXADC();
    WriteReg(0, 0x8F, 0x11);
}
/*void DM6300_CalibRF()
{
    uint8_t i, j;
    uint32_t dh, dl;
    int16_t vol;
    
    DM6300_SetSingleTone(1);
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0x2A0, 0x00000000, 0xA05B45FE);
    
    for (i=0;i<8;i++){
        DM6300_SetChannel(i);
        DM6300_SetPower(0, i, 0);
        Printf("\r\nPA voltage detect: channel = %d,voltage =",(uint16_t)i);
        for(j=0;j<8;j++){
            SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
            SPI_Read (0x3, 0x17C, &dh, &dl);
            vol = (((int32_t)dl) >> 20) + auxadc_offset;
            Printf("%x ", vol);
        }
        //WAIT(1000);
    }
    
    DM6300_SetSingleTone(0);
}*/

int16_t DM6300_GetTemp()
{
    static uint8_t init = 1;
    uint32_t dh, dl;
    int16_t temp;
    
    if(init){
        init = 0;
        SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
        SPI_Write(0x3, 0x2C0, 0x00000000, 0x00000100);
        SPI_Write(0x3, 0x2A0, 0x00000000, 0xA04005FE);
    }
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    SPI_Read (0x3, 0x17C, &dh, &dl);
    temp = (((int32_t)dl) >> 20) + auxadc_offset;
    
    return temp;
}

void DM6300_init1()
{
    SPI_Write(0x6, 0xFFC, 0x00000000, 0x00000000);
    SPI_Write(0x6, 0xFFC, 0x00000000, 0x00000001);
    SPI_Write(0x6, 0x7FC, 0x00000000, 0x00000000);
    SPI_Write(0x6, 0xF1C, 0x00000000, 0x00000001);
    SPI_Write(0x6, 0xF20, 0x00000000, 0x0000FCD0);
    SPI_Write(0x6, 0xF04, 0x00000000, 0x00004741);
    SPI_Write(0x6, 0xF08, 0x00000000, 0x00000083);
    SPI_Write(0x6, 0xF08, 0x00000000, 0x000000C3);
    SPI_Write(0x6, 0xF40, 0x00000000, 0x00000003);
    SPI_Write(0x6, 0xF40, 0x00000000, 0x00000001);
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x6, 0xFFC, 0x00000000, 0x00000000);
    SPI_Write(0x6, 0xFFC, 0x00000000, 0x00000001);
}

void DM6300_init2(uint8_t sel)
{
    if(sel){
        SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
        SPI_Write(0x3, 0x2AC, 0x00000000, 0x00000300);
        SPI_Write(0x3, 0x2B0, 0x00000000, 0x00007007);
        SPI_Write(0x3, 0x230, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x234, 0x00000000, 0x10000000);
        SPI_Write(0x3, 0x238, 0x00000000, 0x000000BF);
        SPI_Write(0x3, 0x23C, 0x00000000, 0x55530610); //0x15840410  0x15530610
        SPI_Write(0x3, 0x240, 0x00000000, 0x3FFC0047); //0x0001C047  0x3FFC0047
        SPI_Write(0x3, 0x244, 0x00000000, 0x00188A13); //0x00188A17
        SPI_Write(0x3, 0x248, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x24C, 0x00000000, 0x0A121707); //0x08021707
        SPI_Write(0x3, 0x250, 0x00000000, 0x017F0001);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x228, 0x00000000, 0x0000807A);
        SPI_Write(0x3, 0x220, 0x00000000, 0x00002FA8); //0x00002666
        SPI_Write(0x3, 0x21C, 0x00000000, 0x00000002);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000001);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x21C, 0x00000000, 0x00000003);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000001);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x244, 0x00000000, 0x00188A17);
        SPI_Write(0x3, 0x204, 0x00000000, 0x00000032); //0x0000004c
        SPI_Write(0x3, 0x208, 0x00000000, 0x00000000); //0x00666666
        SPI_Write(0x3, 0x200, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x200, 0x00000000, 0x00000003);
        SPI_Write(0x3, 0x240, 0x00000000, 0x00030041); //0x0003C045
        SPI_Write(0x3, 0x248, 0x00000000, 0x00000404);
        SPI_Write(0x3, 0x250, 0x00000000, 0x0F7F0001);
        SPI_Write(0x3, 0x254, 0x00000000, 0x00007813); //0x00007813
        SPI_Write(0x3, 0x258, 0x00000000, 0x00010003);
    }
    else{
        // 02_BBPLL_3456
        SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
        SPI_Write(0x3, 0x2AC, 0x00000000, 0x00000300);
        SPI_Write(0x3, 0x2B0, 0x00000000, 0x00077777);
        SPI_Write(0x3, 0x230, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x234, 0x00000000, 0x10000000);
        SPI_Write(0x3, 0x238, 0x00000000, 0x000000BF);
        SPI_Write(0x3, 0x23C, 0x00000000, 0x15530610); //0x15840410  0x15530610
        SPI_Write(0x3, 0x240, 0x00000000, 0x3FFC0047); //0x0001C047  0x3FFC0047
        SPI_Write(0x3, 0x244, 0x00000000, 0x00188A13); //0x00188A17
        SPI_Write(0x3, 0x248, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x24C, 0x00000000, 0x0A161707); //0x08021707
        SPI_Write(0x3, 0x250, 0x00000000, 0x017F0001);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x228, 0x00000000, 0x00008083);
        SPI_Write(0x3, 0x220, 0x00000000, 0x00002E0E); //0x00002666
        SPI_Write(0x3, 0x21C, 0x00000000, 0x00000002);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000001);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x21C, 0x00000000, 0x00000003);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000001);
        SPI_Write(0x3, 0x218, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x244, 0x00000000, 0x00188A17);
        SPI_Write(0x3, 0x204, 0x00000000, 0x0000002D); //0x0000004c
        SPI_Write(0x3, 0x208, 0x00000000, 0x00000000); //0x00666666
        SPI_Write(0x3, 0x200, 0x00000000, 0x00000000);
        SPI_Write(0x3, 0x200, 0x00000000, 0x00000003);
        SPI_Write(0x3, 0x240, 0x00000000, 0x00030041); //0x0003C045
        SPI_Write(0x3, 0x248, 0x00000000, 0x00000404);
        SPI_Write(0x3, 0x250, 0x00000000, 0x0F7F0001);
        SPI_Write(0x3, 0x254, 0x00000000, 0x00007813); //0x00007813
        SPI_Write(0x3, 0x258, 0x00000000, 0x00010002);
    }
}


void DM6300_init3(uint8_t ch)
{
    // 03_RFPLL_CA1_TX_10G
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0x2B0, 0x00000000, 0x00077777);
    SPI_Write(0x3, 0x030, 0x00000000, 0x00000013);
    SPI_Write(0x3, 0x034, 0x00000000, 0x00000013);
    SPI_Write(0x3, 0x038, 0x00000000, 0x00000370);
    SPI_Write(0x3, 0x03C, 0x00000000, 0x00000410);
    SPI_Write(0x3, 0x040, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x044, 0x00000000, 0x0D640735);
    SPI_Write(0x3, 0x048, 0x00000000, 0x01017F03);
    SPI_Write(0x3, 0x04C, 0x00000000, 0x021288A2);
    SPI_Write(0x3, 0x050, 0x00000000, 0x00FFCF33);
    SPI_Write(0x3, 0x054, 0x00000000, 0x1F3C3440);
    //SPI_Write(0x3, 0x028, 0x00000000, 0x00008030);
    SPI_Write(0x3, 0x028, 0x00000000, 0x00008000 + (init6300_fcnt & 0xFF));
    //SPI_Write(0x3, 0x020, 0x00000000, 0x00003746);
    SPI_Write(0x3, 0x020, 0x00000000, init6300_fnum[ch]); //tab[0][ch] WAIT(1);
    SPI_Write(0x3, 0x01C, 0x00000000, 0x00000002);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000001);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000000);
    //SPI_Write(0x3, 0x028, 0x00000000, 0x00008030);
    SPI_Write(0x3, 0x028, 0x00000000, 0x00008000 + (init6300_fcnt & 0xFF));
    //SPI_Write(0x3, 0x020, 0x00000000, 0x00003746);
    SPI_Write(0x3, 0x020, 0x00000000, init6300_fnum[ch]); //tab[0][ch]
    SPI_Write(0x3, 0x01C, 0x00000000, 0x00000003);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000001);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x050, 0x00000000, 0x00FFCFB3);
    //SPI_Write(0x3, 0x004, 0x00000000, 0x00000093);
    //SPI_Write(0x3, 0x008, 0x00000000, 0x00CAAAAB);
    SPI_Write(0x3, 0x004, 0x00000000, tab[1][ch]);
    SPI_Write(0x3, 0x008, 0x00000000, tab[2][ch]);
    SPI_Write(0x3, 0x000, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x000, 0x00000000, 0x00000003);
    SPI_Write(0x3, 0x050, 0x00000000, 0x000333B3);
    SPI_Write(0x3, 0x040, 0x00000000, 0x07070002);
    SPI_Write(0x3, 0x030, 0x00000000, 0x00000010);
}


void DM6300_init4()
{
    // 04_TX_CA1_RF
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0x31C, 0x00000000, 0x00000000); // 0x00000030
    SPI_Write(0x3, 0x300, 0x00000000, 0xC000281B);
    SPI_Write(0x3, 0x304, 0x00000000, 0x0CC00006);
    SPI_Write(0x3, 0x308, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x30C, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x310, 0x00000000, 0xFFFFFF14);
    SPI_Write(0x3, 0x314, 0x00000000, 0x4B7FFFE0);
    SPI_Write(0x3, 0x318, 0x00000000, 0xFFFFFFFF);
    SPI_Write(0x3, 0x320, 0x00000000, 0x00008000);
    SPI_Write(0x3, 0x324, 0x00000000, 0x0000004C);
    SPI_Write(0x3, 0x328, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x32C, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x330, 0x00000000, 0x00000000); //remove init DC high
    SPI_Write(0x3, 0x334, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x31C, 0x00000000, 0x00000011);
}


void DM6300_init5()
{
    // 05_tx_cal_DAC_BBF
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    SPI_Write(0x3, 0x194, 0x00000000, 0x0001FFFF);
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0x778, 0x00000000, 0x80000A80); //0x80000A80 0x00400680
    SPI_Write(0x3, 0x77C, 0x00000000, 0x0D000000); //0x0D000000 0x0D800680
    SPI_Write(0x3, 0x780, 0x00000000, 0x80000A80); //0x80000A80 0x00400680
    SPI_Write(0x3, 0x784, 0x00000000, 0x0D000000); //0x0D000000 0x0D800680
    SPI_Write(0x3, 0x788, 0x00000000, 0x0FFFFFFF);
    SPI_Write(0x3, 0x78C, 0x00000000, 0x00300FD6); //0x00300FD6 0x80080FD5
    SPI_Write(0x3, 0x384, 0x00000000, 0x00075075);
    SPI_Write(0x3, 0x38C, 0x00000000, 0x00075075);
    SPI_Write(0x3, 0x390, 0x00000000, 0x042F5444);
    SPI_Write(0x3, 0x394, 0x00000000, 0x2E788108);
    SPI_Write(0x3, 0x398, 0x00000000, 0x2E788128);
    SPI_Write(0x3, 0x39C, 0x00000000, 0x2C80C0C8);
    SPI_Write(0x3, 0x3A0, 0x00000000, 0x00006868);
    SPI_Write(0x3, 0x3A4, 0x00000000, 0x04150404);
    SPI_Write(0x3, 0x3A8, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x3AC, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x3B0, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x3D8, 0x00000000, 0x0000000A);
    SPI_Write(0x3, 0x380, 0x00000000, 0x075F8000);
    SPI_Write(0x3, 0x388, 0x00000000, 0x075F8000);
}


void DM6300_init6(uint8_t sel)
{
    // 06_TX_CA1_RBDP_CMOS
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    SPI_Write(0x3, 0x0E4, 0x00000000, 0x00000002);
    SPI_Write(0x3, 0x0E8, 0x00000000, 0x0000000D);
    SPI_Write(0x3, 0x08c, 0x00000000, 0x00000000);
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0xd04, 0x00000000, 0x020019D9); //0x0x020000ED
    SPI_Write(0x3, 0xd08, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0xd1c, 0x00000000, 0x00000000); //remove init DC high
    SPI_Write(0x3, 0xd00, 0x00000000, 0x00000003);
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    if(sel)
        SPI_Write(0x3, 0x080, 0x00000000, 0x16318C0C);
    else
        SPI_Write(0x3, 0x080, 0x00000000, 0x1084210C);
    
    SPI_Write(0x3, 0x020, 0x00000000, 0x0000000C);
    SPI_Write(0x3, 0x018, 0x00000000, 0x04F16040);
    SPI_Write(0x3, 0x088, 0x00000000, 0x00000085);
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0xD08, 0x00000000, 0x03200300);
    SPI_Write(0x3, 0xD0C, 0x00000000, 0x00000000);
}


void DM6300_init7(uint8_t sel)
{
    if(sel){
        SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
        SPI_Write(0x3, 0xC00, 0x00000000, 0x005B0047);
        SPI_Write(0x3, 0xC04, 0x00000000, 0xFFBB0017);
        SPI_Write(0x3, 0xC08, 0x00000000, 0xFFB2FF8D);
        SPI_Write(0x3, 0xC0C, 0x00000000, 0x006B0011);
        SPI_Write(0x3, 0xC10, 0x00000000, 0x003E0080);
        SPI_Write(0x3, 0xC14, 0x00000000, 0xFF78FFCE);
        SPI_Write(0x3, 0xC18, 0x00000000, 0xFFD4FF78);
        SPI_Write(0x3, 0xC1C, 0x00000000, 0x00A30052);
        SPI_Write(0x3, 0xC20, 0x00000000, 0x0018008C);
        SPI_Write(0x3, 0xC24, 0x00000000, 0xFF43FF8C);
        SPI_Write(0x3, 0xC28, 0x00000000, 0x0001FF71);
        SPI_Write(0x3, 0xC2C, 0x00000000, 0x00D80099);
        SPI_Write(0x3, 0xC30, 0x00000000, 0xFFE2008E);
        SPI_Write(0x3, 0xC34, 0x00000000, 0xFF0CFF3D);
        SPI_Write(0x3, 0xC38, 0x00000000, 0x0042FF77);
        SPI_Write(0x3, 0xC3C, 0x00000000, 0x011200F2);
        SPI_Write(0x3, 0xC40, 0x00000000, 0xFF910081);
        SPI_Write(0x3, 0xC44, 0x00000000, 0xFECDFED5);
        SPI_Write(0x3, 0xC48, 0x00000000, 0x00A9FF8D);
        SPI_Write(0x3, 0xC4C, 0x00000000, 0x01580170);
        SPI_Write(0x3, 0xC50, 0x00000000, 0xFF0C005D);
        SPI_Write(0x3, 0xC54, 0x00000000, 0xFE7AFE38);
        SPI_Write(0x3, 0xC58, 0x00000000, 0x015DFFC4);
        SPI_Write(0x3, 0xC5C, 0x00000000, 0x01C20240);
        SPI_Write(0x3, 0xC60, 0x00000000, 0xFE060007);
        SPI_Write(0x3, 0xC64, 0x00000000, 0xFDE4FD0A);
        SPI_Write(0x3, 0xC68, 0x00000000, 0x03050054);
        SPI_Write(0x3, 0xC6C, 0x00000000, 0x02BD0434);
        SPI_Write(0x3, 0xC70, 0x00000000, 0xFAB7FEEF);
        SPI_Write(0x3, 0xC74, 0x00000000, 0xFB99F8D8);
        SPI_Write(0x3, 0xC78, 0x00000000, 0x0E9D0376);
        SPI_Write(0x3, 0xC7C, 0x00000000, 0x20271979);
        SPI_Write(0x3, 0xC80, 0x00000000, 0x19792027);
        SPI_Write(0x3, 0xC84, 0x00000000, 0x03760E9D);
        SPI_Write(0x3, 0xC88, 0x00000000, 0xF8D8FB99);
        SPI_Write(0x3, 0xC8C, 0x00000000, 0xFEEFFAB7);
        SPI_Write(0x3, 0xC90, 0x00000000, 0x043402BD);
        SPI_Write(0x3, 0xC94, 0x00000000, 0x00540305);
        SPI_Write(0x3, 0xC98, 0x00000000, 0xFD0AFDE4);
        SPI_Write(0x3, 0xC9C, 0x00000000, 0x0007FE06);
        SPI_Write(0x3, 0xCA0, 0x00000000, 0x024001C2);
        SPI_Write(0x3, 0xCA4, 0x00000000, 0xFFC4015D);
        SPI_Write(0x3, 0xCA8, 0x00000000, 0xFE38FE7A);
        SPI_Write(0x3, 0xCAC, 0x00000000, 0x005DFF0C);
        SPI_Write(0x3, 0xCB0, 0x00000000, 0x01700158);
        SPI_Write(0x3, 0xCB4, 0x00000000, 0xFF8D00A9);
        SPI_Write(0x3, 0xCB8, 0x00000000, 0xFED5FECD);
        SPI_Write(0x3, 0xCBC, 0x00000000, 0x0081FF91);
        SPI_Write(0x3, 0xCC0, 0x00000000, 0x00F20112);
        SPI_Write(0x3, 0xCC4, 0x00000000, 0xFF770042);
        SPI_Write(0x3, 0xCC8, 0x00000000, 0xFF3DFF0C);
        SPI_Write(0x3, 0xCCC, 0x00000000, 0x008EFFE2);
        SPI_Write(0x3, 0xCD0, 0x00000000, 0x009900D8);
        SPI_Write(0x3, 0xCD4, 0x00000000, 0xFF710001);
        SPI_Write(0x3, 0xCD8, 0x00000000, 0xFF8CFF43);
        SPI_Write(0x3, 0xCDC, 0x00000000, 0x008C0018);
        SPI_Write(0x3, 0xCE0, 0x00000000, 0x005200A3);
        SPI_Write(0x3, 0xCE4, 0x00000000, 0xFF78FFD4);
        SPI_Write(0x3, 0xCE8, 0x00000000, 0xFFCEFF78);
        SPI_Write(0x3, 0xCEC, 0x00000000, 0x0080003E);
        SPI_Write(0x3, 0xCF0, 0x00000000, 0x0011006B);
        SPI_Write(0x3, 0xCF4, 0x00000000, 0xFF8DFFB2);
        SPI_Write(0x3, 0xCF8, 0x00000000, 0x0017FFBB);
        SPI_Write(0x3, 0xCFC, 0x00000000, 0x0047005B);
    }
    else{
        // 07_fir_128stap
        SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
        SPI_Write(0x3, 0xC00, 0x00000000, 0x004E0041);
        SPI_Write(0x3, 0xC04, 0x00000000, 0x0053005D);
        SPI_Write(0x3, 0xC08, 0x00000000, 0xFFF7002D);
        SPI_Write(0x3, 0xC0C, 0x00000000, 0xFFB1FFC7);
        SPI_Write(0x3, 0xC10, 0x00000000, 0xFFF0FFC1);
        SPI_Write(0x3, 0xC14, 0x00000000, 0x00450025);
        SPI_Write(0x3, 0xC18, 0x00000000, 0x0009003B);
        SPI_Write(0x3, 0xC1C, 0x00000000, 0xFF98FFC6);
        SPI_Write(0x3, 0xC20, 0x00000000, 0xFFD2FF9A);
        SPI_Write(0x3, 0xC24, 0x00000000, 0x00670024);
        SPI_Write(0x3, 0xC28, 0x00000000, 0x00380071);
        SPI_Write(0x3, 0xC2C, 0x00000000, 0xFF79FFD4);
        SPI_Write(0x3, 0xC30, 0x00000000, 0xFF94FF5C);
        SPI_Write(0x3, 0xC34, 0x00000000, 0x0085000B);
        SPI_Write(0x3, 0xC38, 0x00000000, 0x008A00BC);
        SPI_Write(0x3, 0xC3C, 0x00000000, 0xFF620000);
        SPI_Write(0x3, 0xC40, 0x00000000, 0xFF2DFF09);
        SPI_Write(0x3, 0xC44, 0x00000000, 0x0093FFCA);
        SPI_Write(0x3, 0xC48, 0x00000000, 0x010D011B);
        SPI_Write(0x3, 0xC4C, 0x00000000, 0xFF640060);
        SPI_Write(0x3, 0xC50, 0x00000000, 0xFE88FE9F);
        SPI_Write(0x3, 0xC54, 0x00000000, 0x007DFF42);
        SPI_Write(0x3, 0xC58, 0x00000000, 0x01E70195);
        SPI_Write(0x3, 0xC5C, 0x00000000, 0xFF9C0127);
        SPI_Write(0x3, 0xC60, 0x00000000, 0xFD5BFE0B);
        SPI_Write(0x3, 0xC64, 0x00000000, 0x000FFE15);
        SPI_Write(0x3, 0xC68, 0x00000000, 0x03B9025D);
        SPI_Write(0x3, 0xC6C, 0x00000000, 0x007B0323);
        SPI_Write(0x3, 0xC70, 0x00000000, 0xF9E2FCC5);
        SPI_Write(0x3, 0xC74, 0x00000000, 0xFDD2F9D3);
        SPI_Write(0x3, 0xC78, 0x00000000, 0x0F7905A3);
        SPI_Write(0x3, 0xC7C, 0x00000000, 0x1DE41880);
        SPI_Write(0x3, 0xC80, 0x00000000, 0x18801DE4);
        SPI_Write(0x3, 0xC84, 0x00000000, 0x05A30F79);
        SPI_Write(0x3, 0xC88, 0x00000000, 0xF9D3FDD2);
        SPI_Write(0x3, 0xC8C, 0x00000000, 0xFCC5F9E2);
        SPI_Write(0x3, 0xC90, 0x00000000, 0x0323007B);
        SPI_Write(0x3, 0xC94, 0x00000000, 0x025D03B9);
        SPI_Write(0x3, 0xC98, 0x00000000, 0xFE15000F);
        SPI_Write(0x3, 0xC9C, 0x00000000, 0xFE0BFD5B);
        SPI_Write(0x3, 0xCA0, 0x00000000, 0x0127FF9C);
        SPI_Write(0x3, 0xCA4, 0x00000000, 0x019501E7);
        SPI_Write(0x3, 0xCA8, 0x00000000, 0xFF42007D);
        SPI_Write(0x3, 0xCAC, 0x00000000, 0xFE9FFE88);
        SPI_Write(0x3, 0xCB0, 0x00000000, 0x0060FF64);
        SPI_Write(0x3, 0xCB4, 0x00000000, 0x011B010D);
        SPI_Write(0x3, 0xCB8, 0x00000000, 0xFFCA0093);
        SPI_Write(0x3, 0xCBC, 0x00000000, 0xFF09FF2D);
        SPI_Write(0x3, 0xCC0, 0x00000000, 0x0000FF62);
        SPI_Write(0x3, 0xCC4, 0x00000000, 0x00BC008A);
        SPI_Write(0x3, 0xCC8, 0x00000000, 0x000B0085);
        SPI_Write(0x3, 0xCCC, 0x00000000, 0xFF5CFF94);
        SPI_Write(0x3, 0xCD0, 0x00000000, 0xFFD4FF79);
        SPI_Write(0x3, 0xCD4, 0x00000000, 0x00710038);
        SPI_Write(0x3, 0xCD8, 0x00000000, 0x00240067);
        SPI_Write(0x3, 0xCDC, 0x00000000, 0xFF9AFFD2);
        SPI_Write(0x3, 0xCE0, 0x00000000, 0xFFC6FF98);
        SPI_Write(0x3, 0xCE4, 0x00000000, 0x003B0009);
        SPI_Write(0x3, 0xCE8, 0x00000000, 0x00250045);
        SPI_Write(0x3, 0xCEC, 0x00000000, 0xFFC1FFF0);
        SPI_Write(0x3, 0xCF0, 0x00000000, 0xFFC7FFB1);
        SPI_Write(0x3, 0xCF4, 0x00000000, 0x002DFFF7);
        SPI_Write(0x3, 0xCF8, 0x00000000, 0x005D0053);
        SPI_Write(0x3, 0xCFC, 0x00000000, 0x0041004E);
    }
}

void DM6300_Init(uint8_t ch, uint8_t bw)
{
    int i;
    uint32_t rh, rl;
    
#ifdef Raceband
    //                   5658,  5695,  5732,  5769,  5806,  5843,  5880,  5917
    //uint32_t freq[FREQ_MAX+1] = {113160,113900,114640,115380,116120,116860,117600,118340};
    //                   5658,  5695,  5732,  5769,  5806,  5843,  5880,  5917,  5760,  5800
    uint32_t freq[FREQ_MAX_EXT+1] = {113160,113900,114640,115380,116120,116860,117600,118340,115200,116000};
#else
    //                            5660,  5695,  5735,  5770,  5805,  5839,  5878,  5914
    uint32_t freq[FREQ_MAX+1] = {113200,113900,114700,115400,116100,116780,117560,118280};
#endif
    
    // 01_INIT
    DM6300_init1();

#ifdef USE_EFUSE    
    DM6300_EFUSE1();
#endif
    
    // cnt, num
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0x2B0, 0x00000000, 0x00077777);
    SPI_Write(0x3, 0x030, 0x00000000, 0x00000013);
    SPI_Write(0x3, 0x034, 0x00000000, 0x00000013);
    SPI_Write(0x3, 0x038, 0x00000000, 0x00000370);
    SPI_Write(0x3, 0x03C, 0x00000000, 0x00000410);
    SPI_Write(0x3, 0x040, 0x00000000, 0x00000000);
    SPI_Write(0x3, 0x044, 0x00000000, 0x0D640735);
    SPI_Write(0x3, 0x048, 0x00000000, 0x01017F03);
    SPI_Write(0x3, 0x04C, 0x00000000, 0x021288A2);
    SPI_Write(0x3, 0x050, 0x00000000, 0x00FFCF33);
    SPI_Write(0x3, 0x054, 0x00000000, 0x1F3C3440);
    SPI_Write(0x3, 0x028, 0x00000000, 0x00008008);
    SPI_Write(0x3, 0x01C, 0x00000000, 0x0000FFF6);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000001); //WAIT(1);
    SPI_Write(0x3, 0x018, 0x00000000, 0x00000000);
    
    SPI_Read(0x3, 0x02C, &rh, &rl);
    init6300_fcnt = rl & 0x3FFF;
    init6300_fcnt = 0x20000/init6300_fcnt-3;
    for(i=0; i<FREQ_MAX_EXT+1; i++)
        init6300_fnum[i] = freq[i]*init6300_fcnt/384;
   
    // 02_BBPLL_3456
    DM6300_init2(bw);
    
    // 03_RFPLL_CA1_TX_10G
    DM6300_init3(ch);
    
    // 04_TX_CA1_RF
    DM6300_init4();
    
    // 05_tx_cal_DAC_BBF
    DM6300_init5();
    
    // 06_TX_CA1_RBDP_CMOS
    DM6300_init6(bw);
    
    // 07_fir_128stap
    DM6300_init7(bw);
    
#ifdef USE_EFUSE    
    DM6300_EFUSE2();
#endif
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    #ifdef _DEBUG_MODE
    Printf("\r\nDM6300 init done.");
    #endif
}

////////////////////////////////////////////////////////////////////////////////
// efuse rd amd imp
#define  EFUSE_NUM       12   // 12 macro
#define  EFUSE_SIZE      128  // 128*8=1024bit

typedef struct _MACRO0
{
    unsigned char  chip_name[8];
    unsigned char  chip_ver[4];
    unsigned char  chip_id[8];
    unsigned char  chip_grade[4];
    unsigned char  crc[4];
    unsigned char  efuse_ver[4];
    unsigned char  rsvd1[32];
    unsigned short mode;
    unsigned short band_num;
    unsigned char  rsvd2[60];
}MACRO0_T;

typedef struct _MACRO1
{
    unsigned short ical;
    unsigned short rcal;
    unsigned long   bandgap;
	unsigned long   tempsensor;
    unsigned char  rsvd[116];
}MACRO1_T;

typedef struct _RX_CAL
{
    unsigned short freq_start;
    unsigned short freq_stop;
    unsigned long   iqmismatch[3];
    unsigned long   im2;
    unsigned char  rsvd[12];
}RX_CAL_T;

typedef struct _TX_CAL
{
    unsigned short freq_start;
    unsigned short freq_stop;
    unsigned long   dcoc_i;
    unsigned long   dcoc_q;
    
    unsigned long   iqmismatch;
    
    unsigned long   dcoc_i_dvm;
    unsigned long   dcoc_q_dvm;
    
    unsigned char  rsvd[8];
}TX_CAL_T;

typedef struct _MACRO2
{
    RX_CAL_T rx1;
    TX_CAL_T tx1;
    RX_CAL_T rx2;
    TX_CAL_T tx2;
}MACRO2_T;

typedef union _EFUSE
{
    unsigned char dat[EFUSE_NUM][EFUSE_SIZE];
    struct
    {
        MACRO0_T m0;
        MACRO1_T m1;
        MACRO2_T m2[EFUSE_NUM-2];
        unsigned short band;
        unsigned short err;
    }macro;
}EFUSE_T;

EFUSE_T efuse;

void DM6300_EFUSE1()
{
    int i, j;
    int efuse_rdy = 1;
    
    uint32_t rh,rl;

    memset((char*)&efuse, 0, sizeof(EFUSE_T));
    
    // EFUSE_RST = 1;
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    SPI_Write(0x3, 0x0E0, 0x00000000, 0x00000001);

    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    
    for(j=28; j<32/*EFUSE_SIZE*/; j++) // read macro 0
    {
        // EFUSE_CFG = (0<<11) | (j<<4) | 0x1;
        SPI_Write(0x3, 0x7D0, 0x00000000, (j<<4) | 0x1);
        
        // while(EFUSE_RDY&1);
        while(efuse_rdy){
            SPI_Read(0x3, 0x7D4, &rh, &rl);
            efuse_rdy = rl & 1;
        }
        efuse_rdy = 1;
        
        // efuse.data[0][j] = EFUSE_DAT;
        SPI_Read(0x3, 0x7D8, &rh, &rl);
        efuse.dat[0][j] = rl & 0xFF;
    }
    
    for(j=66; j<68/*EFUSE_SIZE*/; j++) // read macro 0
    {
        // EFUSE_CFG = (0<<11) | (j<<4) | 0x1;
        SPI_Write(0x3, 0x7D0, 0x00000000, (j<<4) | 0x1);
        
        // while(EFUSE_RDY&1);
        while(efuse_rdy){
            SPI_Read(0x3, 0x7D4, &rh, &rl);
            efuse_rdy = rl & 1;
        }
        efuse_rdy = 1;
        
        // efuse.data[0][j] = EFUSE_DAT;
        SPI_Read(0x3, 0x7D8, &rh, &rl);
        efuse.dat[0][j] = rl & 0xFF;
    }

    for(j=0; j<12/*EFUSE_SIZE*/; j++) // read macro 1
    {
        // EFUSE_CFG = (1<<11) | (j<<4) | 0x1;
        SPI_Write(0x3, 0x7D0, 0x00000000, (1<<11) | (j<<4) | 0x1);
        
        // while(EFUSE_RDY&1);
        while(efuse_rdy){
            SPI_Read(0x3, 0x7D4, &rh, &rl);
            efuse_rdy = rl & 1;
        }
        efuse_rdy = 1;
        
        // efuse.data[1][j] = EFUSE_DAT;
        SPI_Read(0x3, 0x7D8, &rh, &rl);
        efuse.dat[1][j] = rl & 0xFF;
    }

    //for(i=2; i<efuse.macro.m0.band_num+2; i++) // read macro 2~11
    efuse.macro.m0.band_num = (efuse.macro.m0.band_num >> 8) | (efuse.macro.m0.band_num << 8);
    for(i=2; i<efuse.macro.m0.band_num+2; i++) // read macro 2~11
    {
        for(j=32; j<56/*EFUSE_SIZE*/; j++)
        {
            // EFUSE_CFG = (i<<11) | (j<<4) | 0x1;
            SPI_Write(0x3, 0x7D0, 0x00000000, (i<<11) | (j<<4) | 0x1);
        
            // while(EFUSE_RDY&1);
            while(efuse_rdy){
                SPI_Read(0x3, 0x7D4, &rh, &rl);
                efuse_rdy = rl & 1;
            }
            efuse_rdy = 1;
            
            // efuse.data[i][j] = EFUSE_DAT;
            SPI_Read(0x3, 0x7D8, &rh, &rl);
            efuse.dat[i][j] = rl & 0xFF;
						
						if(j == 35){
							efuse.macro.m2[i-2].tx1.freq_start = (efuse.macro.m2[i-2].tx1.freq_start >> 8) | (efuse.macro.m2[i-2].tx1.freq_start << 8);
							efuse.macro.m2[i-2].tx1.freq_stop = (efuse.macro.m2[i-2].tx1.freq_stop >> 8) | (efuse.macro.m2[i-2].tx1.freq_stop << 8);
							if(efuse.macro.m2[i-2].tx1.freq_start<5000 || efuse.macro.m2[i-2].tx1.freq_stop>6000)
								break;
						}
        }
        }

    // EFUSE_CFG = 0;
    SPI_Write(0x3, 0x7D0, 0x00000000, 0x00000000);
    // EFUSE_RST = 0;
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000019);
    SPI_Write(0x3, 0x0E0, 0x00000000, 0x00000000);
	
    // application
    efuse.macro.m1.bandgap = ((efuse.macro.m1.bandgap >> 24) & 0xFF) |
                             ((efuse.macro.m1.bandgap >> 8)  & 0xFF00) |
                             ((efuse.macro.m1.bandgap << 8)  & 0xFF0000) |
                              ((efuse.macro.m1.bandgap << 24) & 0xFF000000);
    efuse.macro.m1.ical = (efuse.macro.m1.ical >> 8) | (efuse.macro.m1.ical << 8);
    efuse.macro.m1.rcal = (efuse.macro.m1.rcal >> 8) | (efuse.macro.m1.rcal << 8);
        
    //Printf("\r\nband_num=%x", efuse.macro.m0.band_num);
    //Printf("\r\nbandgap=%lx", efuse.macro.m1.bandgap);
    //Printf("\r\nical=%x", efuse.macro.m1.ical);
    //Printf("\r\nrcal=%x", efuse.macro.m1.rcal);
        
    rh = ((efuse.macro.m1.ical & 0x1F) << 3) | (efuse.macro.m1.rcal & 0x7);
    //Printf("\r\nrh=%lx", rh);
    	
    SPI_Write(0x6, 0xF14, 0x00000000, efuse.macro.m1.bandgap);
    SPI_Write(0x6, 0xF18, 0x00000000, rh);
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
}


void DM6300_EFUSE2()
{   
    int i;
    //uint32_t d0,d1,d2,d3,d4,d5,d6,d7;
    //uint32_t wdat;
    uint32_t ef_data;
    uint8_t version[5];
    uint32_t rdat;
    
    
    version[4] = 0;
    for(i=0; i<4; i++){
        version[i] = efuse.macro.m0.efuse_ver[i];        
    }
    #ifdef _DEBUG_MODE
    Printf("\r\n version = %s",version);
    #endif
    //version[1];  //version[1] M.N---M
    //version[3];  //version[3] M.N---N
    
    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
    SPI_Write(0x3, 0x3AC, 0x00000000, 0x00000012);
	for(i=0; i<efuse.macro.m0.band_num; i++) // find match macro 5.8G
    //for(i=0; i<FREQ_MAX_EXT+1; i++) // find match macro 5.8G
	{
        //efuse.macro.m2[i].tx1.freq_start = (efuse.macro.m2[i].tx1.freq_start >> 8) | (efuse.macro.m2[i].tx1.freq_start << 8);
        //efuse.macro.m2[i].tx1.freq_stop = (efuse.macro.m2[i].tx1.freq_stop >> 8) | (efuse.macro.m2[i].tx1.freq_stop << 8);
        #ifdef _DEBUG_MODE
        Printf("\r\n start=%x, stop=%x", efuse.macro.m2[i].tx1.freq_start, efuse.macro.m2[i].tx1.freq_stop);
        #endif
        
		if(efuse.macro.m2[i].tx1.freq_start>=5000 && efuse.macro.m2[i].tx1.freq_stop<=6000)
        {
            //*((volatile unsigned int *)0x200D08) = efuse.macro.m2[i].tx1.iqmismatch;
            //*((volatile unsigned int *)0x200380) = efuse.macro.m2[i].tx1.dcoc_i;
            //*((volatile unsigned int *)0x200388) = efuse.macro.m2[i].tx1.dcoc_q;
            
            if((version[1]=='2') && (version[3]=='3')){ //version = 2.3
                efuse.macro.m2[i].tx1.dcoc_i = efuse.macro.m2[i].tx1.dcoc_i_dvm;
                efuse.macro.m2[i].tx1.dcoc_q = efuse.macro.m2[i].tx1.dcoc_q_dvm;
            }
            else{
                efuse.macro.m2[i].tx1.dcoc_i = efuse.macro.m2[i].tx1.dcoc_i;
                efuse.macro.m2[i].tx1.dcoc_q = efuse.macro.m2[i].tx1.dcoc_q;
            }
            
            efuse.macro.m2[i].tx1.iqmismatch = ((efuse.macro.m2[i].tx1.iqmismatch >> 24) & 0xFF) |
                                              ((efuse.macro.m2[i].tx1.iqmismatch >> 8)  & 0xFF00) |
                                              ((efuse.macro.m2[i].tx1.iqmismatch << 8)  & 0xFF0000) |
                                              ((efuse.macro.m2[i].tx1.iqmismatch << 24) & 0xFF000000);
            
            efuse.macro.m2[i].tx1.dcoc_i = ((efuse.macro.m2[i].tx1.dcoc_i >> 24) & 0xFF) |
                                              ((efuse.macro.m2[i].tx1.dcoc_i >> 8)  & 0xFF00) |
                                              ((efuse.macro.m2[i].tx1.dcoc_i << 8)  & 0xFF0000) |
                                              ((efuse.macro.m2[i].tx1.dcoc_i << 24) & 0xFF000000);
            
            efuse.macro.m2[i].tx1.dcoc_q = ((efuse.macro.m2[i].tx1.dcoc_q >> 24) & 0xFF) |
                                              ((efuse.macro.m2[i].tx1.dcoc_q >> 8)  & 0xFF00) |
                                              ((efuse.macro.m2[i].tx1.dcoc_q << 8)  & 0xFF0000) |
                                              ((efuse.macro.m2[i].tx1.dcoc_q << 24) & 0xFF000000);
					  
            Printf("\r\niqmismatch_old=%lx", efuse.macro.m2[i].tx1.iqmismatch);
            Printf("\r\ndcoc_i_old=%lx", efuse.macro.m2[i].tx1.dcoc_i);
            Printf("\r\ndcoc_q_old=%lx", efuse.macro.m2[i].tx1.dcoc_q);
            
            //change dc_i/dc_q
            Printf("\r\n version[1] = %c",(uint16_t)version[1]);
            Printf("\r\n version[3] = %c",(uint16_t)version[3]);
            
            //if((version[1]>'2') | ((version[1]>='2') && (version[3]>'1'))){ //version > 2.1
            if((version[1]=='2') && (version[3]=='2')){ //version = 2.2            
                ef_data = efuse.macro.m2[i].tx1.dcoc_i;
                ef_data = (ef_data & 0xFFFF8000) | ((((((ef_data & 0x00007f7f)+0x00000202)>>2)&0x00001f1f)) | 0x00000080);
                efuse.macro.m2[i].tx1.dcoc_i = ef_data;
                
                ef_data = efuse.macro.m2[i].tx1.dcoc_q;
                ef_data = (ef_data & 0xFFFF8000) | ((((((ef_data & 0x00007f7f)+0x00000202)>>2)&0x00001f1f)) | 0x00000080);
                efuse.macro.m2[i].tx1.dcoc_q = ef_data;
            }
            
            SPI_Write(0x3, 0xD08, 0x00000000,  efuse.macro.m2[i].tx1.iqmismatch);
            SPI_Write(0x3, 0x380, 0x00000000,  efuse.macro.m2[i].tx1.dcoc_i);
            SPI_Write(0x3, 0x388, 0x00000000,  efuse.macro.m2[i].tx1.dcoc_q);           
            
            Printf("\r\niqmismatch=%lx", efuse.macro.m2[i].tx1.iqmismatch);
            Printf("\r\ndcoc_i=%lx", efuse.macro.m2[i].tx1.dcoc_i);
            Printf("\r\ndcoc_q=%lx", efuse.macro.m2[i].tx1.dcoc_q);
            
            dcoc_ih = efuse.macro.m2[i].tx1.dcoc_i & 0xFFFF0000;
            dcoc_qh = efuse.macro.m2[i].tx1.dcoc_q & 0xFFFF0000;
            
            if(EE_VALID){
                WAIT(10); rdat = I2C_Read(ADDR_EEPROM, EEP_ADDR_DCOC_EN, 0, 0);
                if((rdat & 0xFF) == 0){
                    Printf("\r\nDCOC read from EEPROM:");
                    SPI_Write(0x6, 0xFF0, 0x00000000, 0x00000018);
                    
                    WAIT(10); rdat = I2C_Read(ADDR_EEPROM, EEP_ADDR_DCOC_IH, 0, 0);
                    rdat <<= 8;
                    WAIT(10); rdat |= I2C_Read(ADDR_EEPROM, EEP_ADDR_DCOC_IL, 0, 0);
                    rdat |= dcoc_ih;
                    SPI_Write(0x3, 0x380, 0x00000000, rdat);
                    Printf("\r\ndcoc_i=%lx", rdat);
                    
                    WAIT(10); rdat = I2C_Read(ADDR_EEPROM, EEP_ADDR_DCOC_QH, 0, 0);
                    rdat <<= 8;
                    WAIT(10); rdat |= I2C_Read(ADDR_EEPROM, EEP_ADDR_DCOC_QL, 0, 0);
                    rdat |= dcoc_qh;
                    SPI_Write(0x3, 0x388, 0x00000000, rdat);
                    Printf("\r\ndcoc_q=%lx", rdat);
                }
            }
            
            /*if(EE_VALID){
                WAIT(10); d0 = I2C_Read(ADDR_EEPROM, 0xa8, 0, 0);
                WAIT(10); d1 = I2C_Read(ADDR_EEPROM, 0xa9, 0, 0);
                WAIT(10); d2 = I2C_Read(ADDR_EEPROM, 0xaa, 0, 0);
                WAIT(10); d3 = I2C_Read(ADDR_EEPROM, 0xab, 0, 0);
                WAIT(10); d4 = I2C_Read(ADDR_EEPROM, 0xac, 0, 0);
                WAIT(10); d5 = I2C_Read(ADDR_EEPROM, 0xad, 0, 0);
                WAIT(10); d6 = I2C_Read(ADDR_EEPROM, 0xae, 0, 0);
                WAIT(10); d7 = I2C_Read(ADDR_EEPROM, 0xaf, 0, 0);
                
                Printf("\r\nd0=%lx,d1=%lx,d2=%lx,d3=%lx", d0,d1,d2,d3);
                Printf("\r\nd4=%lx,d5=%lx,d6=%lx,d7=%lx", d4,d5,d6,d7);
            
                wdat = 0x075F0000;
                wdat = wdat | (d0<<8) | d1;
                SPI_Write(0x3, 0x380, 0x00000000, wdat);
                Printf("\r\nreg380=%lx", wdat);
                wdat = 0x075F0000;
                wdat = wdat | (d2<<8) | d3;
                SPI_Write(0x3, 0x388, 0x00000000, wdat);
                Printf("\r\nreg388=%lx", wdat);
                wdat = (d4 << 24) | (d5 << 16) | (d6 << 8) | d7;
                SPI_Write(0x3, 0xD08, 0x00000000, wdat);
                Printf("\r\nregD08=%lx", wdat);
            }*/
            
            break;
        }
	}
}
