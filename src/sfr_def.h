#ifndef __SFR_DEF_H_
#define __SFR_DEF_H_

#include "toolchain.h"

////////////////////////////////////////////////////////////////////////////////
// SFR

SFR(P0, 0x80);     /* Port 0                    */
SFR(SP, 0x81);     /* Stack Pointer             */
SFR(DPL0, 0x82);   /* Data Pointer 0 Low byte   */
SFR(DPH0, 0x83);   /* Data Pointer 0 High byte  */
SFR(DPL1, 0x84);   /* Data Pointer 1 Low byte   */
SFR(DPH1, 0x85);   /* Data Pointer 1 High byte  */
SFR(DPS, 0x86);
SFR(PCON, 0x87);   /* Power Configuration       */

SFR(TCON, 0x88);   /* Timer 0,1 Configuration   */
SFR(TMOD, 0x89);   /* Timer 0,1 Mode            */
SFR(TL0, 0x8A);    /* Timer 0 Low byte counter  */
SFR(TL1, 0x8B);    /* Timer 1 Low byte counter  */
SFR(TH0, 0x8C);    /* Timer 0 High byte counter */
SFR(TH1, 0x8D);    /* Timer 1 High byte counter */
SFR(CKCON, 0x8E);  /* XDATA Wait States         */

SFR(P1, 0x90);     /* Port 1                    */
SFR(EIF, 0x91);
SFR(WTST, 0x92);   /* Program Wait States       */
SFR(DPX0, 0x93);   /* Data Page Pointer 0       */
SFR(DPX1, 0x95);   /* Data Page Pointer 1       */

SFR(SCON0, 0x98);  /* Serial 0 Configuration    */
SFR(SBUF0, 0x99);  /* Serial 0 I/O Buffer       */

SFR(P2, 0xA0);     /* Port 2                    */

SFR(IE, 0xA8);     /* Interrupt Enable          */

SFR(P3, 0xB0);     /* Port 3                    */

SFR(IP, 0xB8);

SFR(SCON1, 0xC0);  /* Serial 1 Configuration    */
SFR(SBUF1, 0xC1);  /* Serial 1 I/O Buffer       */

SFR(T2CON, 0xC8);
SFR(T2IF, 0xC9);
SFR(RLDL, 0xCA);
SFR(RLDH, 0xCB);
SFR(TL2, 0xCC);
SFR(TH2, 0xCD);
SFR(CCEN, 0xCE);

SFR(PSW, 0xD0);    /* Program Status Word       */

SFR(WDCON, 0xD8);

SFR(ACC, 0xE0);    /* Accumulator               */

SFR(EIE, 0xE8);    /* External Interrupt Enable */
SFR(STATUS, 0xE9); /* Status register           */
SFR(MXAX, 0xEA);   /* MOVX @Ri High address     */
SFR(TA, 0xEB);

SFR(B, 0xF0);      /* B Working register        */

SFR(EIP, 0xF8);

////////////////////////////////////////////////////////////////////////////////
// Bit define

#ifndef SDCC
/*  P0  */
SBIT(P0_7, P0^7);
SBIT(P0_6, P0^6);
SBIT(P0_5, P0^5);
SBIT(P0_4, P0^4);
SBIT(P0_3, P0^3);
SBIT(P0_2, P0^2);
SBIT(P0_1, P0^1);
SBIT(P0_0, P0^0);

/*  P1  */
SBIT(P1_7, P1^7);
SBIT(P1_6, P1^6);
SBIT(P1_5, P1^5);
SBIT(P1_4, P1^4);
SBIT(P1_3, P1^3);
SBIT(P1_2, P1^2);
SBIT(P1_1, P1^1);
SBIT(P1_0, P1^0);

/*  P2  */
SBIT(P2_7, P2^7);
SBIT(P2_6, P2^6);
SBIT(P2_5, P2^5);
SBIT(P2_4, P2^4);
SBIT(P2_3, P2^3);
SBIT(P2_2, P2^2);
SBIT(P2_1, P2^1);
SBIT(P2_0, P2^0);

/*  P3  */
SBIT(P3_7, P3^7);
SBIT(P3_6, P3^6);
SBIT(P3_5, P3^5);
SBIT(P3_4, P3^4);
SBIT(P3_3, P3^3);
SBIT(P3_2, P3^2);
SBIT(P3_1, P3^1);
SBIT(P3_0, P3^0);

/*  TCON  */
SBIT(TF1, TCON^7);
SBIT(TR1, TCON^6);
SBIT(TF0, TCON^5);
SBIT(TR0, TCON^4);
SBIT(IE1, TCON^3);
SBIT(IT1, TCON^2);
SBIT(IE0, TCON^1);
SBIT(IT0, TCON^0);

/*  IE  */
SBIT(EA, IE^7);
SBIT(ES1, IE^6);
SBIT(ET2, IE^5);
SBIT(ES0, IE^4);
SBIT(ET1, IE^3);
SBIT(EX1, IE^2);
SBIT(ET0, IE^1);
SBIT(EX0, IE^0);

/*  IP  */
SBIT(PS1, IP^6);
SBIT(PT2, IP^5);
SBIT(PS0, IP^4);
SBIT(PT1, IP^3);
SBIT(PX1, IP^2);
SBIT(PT0, IP^1);
SBIT(PX0, IP^0);

/*  SCON0  */
SBIT(SM0, SCON0^7);
SBIT(SM1, SCON0^6);
SBIT(SM2, SCON0^5);
SBIT(REN, SCON0^4);
SBIT(TB8, SCON0^3);
SBIT(RB8, SCON0^2);
SBIT(TI, SCON0^1);
SBIT(RI, SCON0^0);

/*  SCON1  */
SBIT(SM10, SCON1^7);
SBIT(SM11, SCON1^6);
SBIT(SM12, SCON1^5);
SBIT(REN1, SCON1^4);
SBIT(TB18, SCON1^3);
SBIT(RB18, SCON1^2);
SBIT(TI1, SCON1^1);
SBIT(RI1, SCON1^0);

/*  EIF  */
//SBIT(INT6F, EIF^4);
//SBIT(INT5F, EIF^3);
//SBIT(INT4F, EIF^2);
//SBIT(INT3F, EIF^1);
//SBIT(INT2F, EIF^0);
#endif

#endif
