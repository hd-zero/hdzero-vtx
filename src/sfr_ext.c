#include "common.h"
#include "sfr_ext.h"

/////////////////////////////////////////////////////////////////
// reg w/r
void WriteReg(uint8_t page, uint8_t addr, uint8_t dat)
{
    SFR_DATA  = dat;
    SFR_ADDRL = addr;
    SFR_CMD   = page ? 0x02 : 0x00;
}

uint8_t ReadReg(uint8_t page, uint8_t addr)
{
    uint8_t busy = 1;
    
    SFR_ADDRL = addr;
    SFR_CMD   = page ? 0x03 : 0x01;
    
    while(busy != 0){
        busy = SFR_BUSY & 0x02;
    }
    
    return SFR_DATA;
}

/////////////////////////////////////////////////////////////////
// AD936X w/r
void Write936x(uint16_t addr, uint8_t dat)
{
    uint8_t busy = 1;
    
    SFR_DATA  = dat;
    SFR_ADDRL = addr & 0xFF;
    SFR_ADDRH = (addr>>8) & 0xFF;
    SFR_CMD   = 0x10;
    
    while(busy != 0){
        busy = SFR_BUSY & 0x01;
    }
}

uint8_t Read936x(uint16_t addr)
{
    uint8_t busy = 1;

    SFR_ADDRL = addr & 0xFF;
    SFR_ADDRH = (addr>>8) & 0xFF;
    SFR_CMD   = 0x11;
    
    while(busy != 0){
        busy = SFR_BUSY & 0x01;
    }
    
    return SFR_DATA;
}

/////////////////////////////////////////////////////////////////
// flight control
void DP_tx(uint8_t dat)
{
    SFR_DATA  = dat;
    SFR_CMD   = 0x20;
}
/*
// OSD
void OSD_Mark_wr(uint16_t addr, uint8_t dat)
{
    SFR_DATA  = dat;
    SFR_ADDRL = addr & 0xff;
    SFR_ADDRH = (addr>>8) & 0xff;
    SFR_CMD   = 0x30;
}

void OSD_Map_wr(uint16_t addr, uint8_t dat)
{
    SFR_DATA  = dat;
    SFR_ADDRL = addr & 0xff;
    SFR_ADDRH = (addr>>8) & 0xff;
    SFR_CMD   = 0x31;
}

void OSD_CH_wr(uint16_t addr, uint8_t dat)
{
    SFR_DATA  = dat;
    SFR_ADDRL = addr & 0xff;
    SFR_ADDRH = (addr>>8) & 0xff;
    SFR_CMD   = 0x32;
}
*/