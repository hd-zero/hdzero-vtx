#include "common.h"
#include "global.h"
#include "uart.h"
#include "isr.h"
#include "uart.h"

BIT_TYPE int0_req = 0;
BIT_TYPE int1_req = 0;
uint8_t btn1_tflg = 0;
uint8_t pwr_sflg = 0;    //power autoswitch flag
uint8_t pwr_tflg = 0;
uint8_t cfg_tflg = 0;
uint8_t temp_tflg = 0;
uint8_t timer_4hz = 0;
uint8_t timer_8hz = 0;
uint8_t timer_16hz = 0;
IDATA_SEG uint16_t timer_ms10x = 0;
uint16_t seconds = 0;
uint8_t RS0_ERR = 0;

void CPU_init(void)
{
    SCON0 = 0x50;   // [7:6] uart0 mode: 0x01 = 8bit, variable baudrate(related to timer1)
                    // [4]   uart0 rx enable

    SCON1 = 0x50;   // [7:6] uart1 mode: 0x01 = 8bit, variable baudrate(related to timer1)
                    // [4]   uart1 rx enable

    PCON  = 0xC0;   // [7]   double rate for uart0
                    // [6]   double rate for uart1

    TMOD  = 0x20;   // [7]   timer1 enabled by pin gate1
                    // [6]   timer1: 0=as timer, 1=as counter
                    // [5:4] tmier1 mode: 0x10 = 8bit timer with 8bit auto-reload
                    // [3]   timer0 enabled by pin gate0
                    // [2]   timer0: 0=as timer, 1=as counter
                    // [1:0] tmier0 mode: 0x01 = 16bit timer, {TH0,TL0} cascaded

    CKCON = 0x1F;   // [4]   timer1 uses a divide-by-N of the system clock frequency. 0:N=12; 1:N=4
                    // [3]   timer0 uses a divide-by-N of the system clock frequency. 0:N=12; 1:N=4
    
    TH0   = 139;
    TL0   = 0;
    
    TH1   = 0xEC;   // [7:0] in timer mode 0x10:   ----------------->> 148.5MHz: 0x87; 100MHz: 0xAF; 54MHz: 0xD4; 27MHz: 0xEA
                    //	               f(clk)
                    //  BaudRate = --------------  (M=16 or 32, decided by PCON double rate flag)
                    //             N*(256-TH1)*M   (N=4 or 12, decided by CKCON [4])
                    // 19200  - 0x87
                    // 115200 - 0xEC    256 - 148.5M/64/115200 = 0xEC  0.7%
                    // 230400 - 0xF6    256 - 148.5M/64/230400 = 0xF6  0.7%
                    // 250000 - 0xF7    256 - 148.5M/64/250000 = 0xF7  3.125%

    TCON  = 0x50;   // [6]   enable timer1
                    // [4]   enable timer0

    IE    = 0xD2;   // [7]   enable global interupts  1
                    // [6]   enable uart1  interupt   1
                    // [5]   enable timer2 interupt   0
                    // [4]   enable uart0  interupt   1 
                    // [3]   enable timer1 interupt   0 
                    // [2]   enable INT1   interupt   0 
                    // [1]   enable timer0 interupt   0
                    // [0]   enable INT0   interupt   0
    IP    = 0x10;   // UART0=higher priority, Timer 0 = low
    
}

void Timer0_isr(void) INTERRUPT(1)
{
    TH0 = 138;

    #ifdef USE_SMARTAUDIO
    if(SA_config) {
        if(suart_tx_en) {
            //TH0 = 139;
            suart_txint();
        }
        else
            suart_rxint();
    }
    #endif
    
    timer_ms10x++;
}

void Timer1_isr(void) INTERRUPT(3)
{
}		 


void Ext0_isr(void) INTERRUPT(0)
{
	int0_req = 1;
}

void Ext1_isr(void) INTERRUPT(2)
{
	int1_req = 1;
}

void UART0_isr() INTERRUPT(4)
{
	if( RI ) {			//RX int
		RI = 0;
		RS_buf[RS_in++] = SBUF0;
		if( RS_in>=BUF_MAX ) RS_in = 0;
        if(RS_in == RS_out) RS0_ERR = 1;
	}

	if( TI ) {			//TX int
		TI = 0;
		RS_Xbusy=0;
	}
}

void UART1_isr() INTERRUPT(6)
{
    
	if( RI1 ) {			//RX int
		RI1 = 0;
		RS_buf1[RS_in1++] = SBUF1;
		if( RS_in1>=BUF_MAX ) RS_in1 = 0;
	}

	if( TI1 ) {			//TX int
		TI1 = 0;
		RS_Xbusy1=0;
	}
}
