#ifndef __SFR_DEF_H_
#define __SFR_DEF_H_

////////////////////////////////////////////////////////////////////////////////
// SFR

sfr P0        = 0x80;   /* Port 0                    */
sfr SP        = 0x81;   /* Stack Pointer             */
sfr DPL0      = 0x82;   /* Data Pointer 0 Low byte   */
sfr DPH0      = 0x83;   /* Data Pointer 0 High byte  */
sfr DPL1      = 0x84;   /* Data Pointer 1 Low byte   */
sfr DPH1      = 0x85;   /* Data Pointer 1 High byte  */
sfr DPS       = 0x86;
sfr PCON      = 0x87;   /* Power Configuration       */

sfr TCON      = 0x88;   /* Timer 0,1 Configuration   */
sfr TMOD      = 0x89;   /* Timer 0,1 Mode            */
sfr TL0       = 0x8A;   /* Timer 0 Low byte counter  */
sfr TL1       = 0x8B;   /* Timer 1 Low byte counter  */
sfr TH0       = 0x8C;   /* Timer 0 High byte counter */
sfr TH1       = 0x8D;   /* Timer 1 High byte counter */
sfr CKCON     = 0x8E;   /* XDATA Wait States         */

sfr P1        = 0x90;   /* Port 1                    */
sfr EIF       = 0x91;
sfr WTST      = 0x92;   /* Program Wait States       */
sfr DPX0      = 0x93;   /* Data Page Pointer 0       */
sfr DPX1      = 0x95;   /* Data Page Pointer 1       */

sfr SCON0     = 0x98;   /* Serial 0 Configuration    */
sfr SBUF0     = 0x99;   /* Serial 0 I/O Buffer       */

sfr P2        = 0xA0;   /* Port 2                    */

sfr IE        = 0xA8;   /* Interrupt Enable          */

sfr P3        = 0xB0;   /* Port 3                    */

sfr IP        = 0xB8;

sfr SCON1     = 0xC0;   /* Serial 1 Configuration    */
sfr SBUF1     = 0xC1;   /* Serial 1 I/O Buffer       */

sfr T2CON     = 0xC8;
sfr T2IF      = 0xC9;
sfr RLDL      = 0xCA;
sfr RLDH      = 0xCB;
sfr TL2       = 0xCC;
sfr TH2       = 0xCD;
sfr CCEN      = 0xCE;

sfr PSW       = 0xD0;   /* Program Status Word       */

sfr WDCON     = 0xD8;

sfr ACC       = 0xE0;   /* Accumulator               */

sfr EIE       = 0xE8;   /* External Interrupt Enable */
sfr STATUS    = 0xE9;   /* Status register           */
sfr MXAX      = 0xEA;   /* MOVX @Ri High address     */
sfr TA        = 0xEB;

sfr B         = 0xF0;   /* B Working register        */

sfr EIP       = 0xF8;

////////////////////////////////////////////////////////////////////////////////
// Bit define

/*  P0  */
sbit P0_7     = P0^7;
sbit P0_6     = P0^6;
sbit P0_5     = P0^5;
sbit P0_4     = P0^4;
sbit P0_3     = P0^3;
sbit P0_2     = P0^2;
sbit P0_1     = P0^1;
sbit P0_0     = P0^0;

/*  P1  */
sbit P1_7     = P1^7;
sbit P1_6     = P1^6;
sbit P1_5     = P1^5;
sbit P1_4     = P1^4;
sbit P1_3     = P1^3;
sbit P1_2     = P1^2;
sbit P1_1     = P1^1;
sbit P1_0     = P1^0;

/*  P2  */
sbit P2_7     = P2^7;
sbit P2_6     = P2^6;
sbit P2_5     = P2^5;
sbit P2_4     = P2^4;
sbit P2_3     = P2^3;
sbit P2_2     = P2^2;
sbit P2_1     = P2^1;
sbit P2_0     = P2^0;

/*  P3  */
sbit P3_7     = P3^7;
sbit P3_6     = P3^6;
sbit P3_5     = P3^5;
sbit P3_4     = P3^4;
sbit P3_3     = P3^3;
sbit P3_2     = P3^2;
sbit P3_1     = P3^1;
sbit P3_0     = P3^0;

/*  TCON  */
sbit TF1      = TCON^7;
sbit TR1      = TCON^6;
sbit TF0      = TCON^5;
sbit TR0      = TCON^4;
sbit IE1      = TCON^3;
sbit IT1      = TCON^2;
sbit IE0      = TCON^1;
sbit IT0      = TCON^0;

/*  IE  */
sbit EA       = IE^7;
sbit ES1      = IE^6;
sbit ET2      = IE^5;
sbit ES0      = IE^4;
sbit ET1      = IE^3;
sbit EX1      = IE^2;
sbit ET0      = IE^1;
sbit EX0      = IE^0;

/*  IP  */
sbit PS1      = IP^6;
sbit PT2      = IP^5;
sbit PS0      = IP^4;
sbit PT1      = IP^3;
sbit PX1      = IP^2;
sbit PT0      = IP^1;
sbit PX0      = IP^0;

/*  SCON0  */
sbit SM0      = SCON0^7;
sbit SM1      = SCON0^6;
sbit SM2      = SCON0^5;
sbit REN      = SCON0^4;
sbit TB8      = SCON0^3;
sbit RB8      = SCON0^2;
sbit TI       = SCON0^1;
sbit RI       = SCON0^0;

/*  SCON1  */
sbit SM10     = SCON1^7;
sbit SM11     = SCON1^6;
sbit SM12     = SCON1^5;
sbit REN1     = SCON1^4;
sbit TB18     = SCON1^3;
sbit RB18     = SCON1^2;
sbit TI1      = SCON1^1;
sbit RI1      = SCON1^0;

/*  EIF  */
//sbit INT6F    = EIF^4;
//sbit INT5F    = EIF^3;
//sbit INT4F    = EIF^2;
//sbit INT3F    = EIF^1;
//sbit INT2F    = EIF^0;

#endif
