#ifndef __CA51F2_H__
#define __CA51F2_H__

extern volatile __sfr unsigned char P0;
extern volatile __sfr unsigned char SP;
extern volatile __sfr unsigned char DP0L;
extern volatile __sfr unsigned char DP0H;
extern volatile __sfr unsigned char DP1L;
extern volatile __sfr unsigned char DP1H;
extern volatile __sfr unsigned char PWCON;
extern volatile __sfr unsigned char PCON;
extern volatile __sfr unsigned char TCON;
extern volatile __sfr unsigned char TMOD;
extern volatile __sfr unsigned char TL0;
extern volatile __sfr unsigned char TL1;
extern volatile __sfr unsigned char TH0;
extern volatile __sfr unsigned char TH1;
extern volatile __sfr unsigned char IT1CON;
extern volatile __sfr unsigned char IT0CON;

extern volatile __sfr unsigned char P1;
extern volatile __sfr unsigned char RCCON;
extern volatile __sfr unsigned char VCKDL;
extern volatile __sfr unsigned char VCKDH;
extern volatile __sfr unsigned char RCTAGL;
extern volatile __sfr unsigned char RCTAGH;
extern volatile __sfr unsigned char RCMSLL;
extern volatile __sfr unsigned char RCMSLH;
extern volatile __sfr unsigned char S0CON;
extern volatile __sfr unsigned char S0BUF;
extern volatile __sfr unsigned char S1CON;
extern volatile __sfr unsigned char S1BUF;
extern volatile __sfr unsigned char S1RELL;
extern volatile __sfr unsigned char S1RELH;
extern volatile __sfr unsigned char RCMSHL;
extern volatile __sfr unsigned char RCMSHH;

extern volatile __sfr unsigned char P2;
extern volatile __sfr unsigned char S2CON;
extern volatile __sfr unsigned char S2BUF;
extern volatile __sfr unsigned char S2RELL;
extern volatile __sfr unsigned char S2RELH;
extern volatile __sfr unsigned char SPCON;
extern volatile __sfr unsigned char SPDAT;
extern volatile __sfr unsigned char SPSTA;
extern volatile __sfr unsigned char IE;
extern volatile __sfr unsigned char P6;
extern volatile __sfr unsigned char WDCON;
extern volatile __sfr unsigned char WDFLG;
extern volatile __sfr unsigned char WDVTHL;
extern volatile __sfr unsigned char WDVTHH;
extern volatile __sfr unsigned char PLLCON;
extern volatile __sfr unsigned char HVTH;

extern volatile __sfr unsigned char P3;
extern volatile __sfr unsigned char I2CCON;
extern volatile __sfr unsigned char I2CADR;
extern volatile __sfr unsigned char I2CADM;
extern volatile __sfr unsigned char I2CCCR;
extern volatile __sfr unsigned char I2CDAT;
extern volatile __sfr unsigned char I2CSTA;
extern volatile __sfr unsigned char I2CFLG;
extern volatile __sfr unsigned char IP;
extern volatile __sfr unsigned char ADCON;
extern volatile __sfr unsigned char ADCFGL;
extern volatile __sfr unsigned char ADCFGH;
extern volatile __sfr unsigned char ADCDL;
extern volatile __sfr unsigned char ADCDH;
extern volatile __sfr unsigned char CKMON;
extern volatile __sfr unsigned char CKMIF;

extern volatile __sfr unsigned char P4;
extern volatile __sfr unsigned char TKCON;
extern volatile __sfr unsigned char TKCFG;
extern volatile __sfr unsigned char TKMTS;
extern volatile __sfr unsigned char TKCHS;
extern volatile __sfr unsigned char ATKML;
extern volatile __sfr unsigned char ATKMH;
extern volatile __sfr unsigned char TKIF;
extern volatile __sfr unsigned char T2CON;
extern volatile __sfr unsigned char T2MOD;
extern volatile __sfr unsigned char T2CL;
extern volatile __sfr unsigned char T2CH;
extern volatile __sfr unsigned char TL2;
extern volatile __sfr unsigned char TH2;
extern volatile __sfr unsigned char TKMSL;
extern volatile __sfr unsigned char TKMSH;

extern volatile __sfr unsigned char PSW;
extern volatile __sfr unsigned char PWMDIVH;
extern volatile __sfr unsigned char PWMDUTL;
extern volatile __sfr unsigned char PWMDUTH;
extern volatile __sfr unsigned char PWMAIF;
extern volatile __sfr unsigned char PWMBIF;
extern volatile __sfr unsigned char PWMCIF;
extern volatile __sfr unsigned char PWMDIF;
extern volatile __sfr unsigned char P5;
extern volatile __sfr unsigned char P7;
extern volatile __sfr unsigned char PWMEN;
extern volatile __sfr unsigned char PWMUPD;
extern volatile __sfr unsigned char PWMCMAX;
extern volatile __sfr unsigned char PWMCON;
extern volatile __sfr unsigned char PWMCFG;
extern volatile __sfr unsigned char PWMDIVL;

extern volatile __sfr unsigned char ACC;
extern volatile __sfr unsigned char LXCON;
extern volatile __sfr unsigned char LXCFG;
extern volatile __sfr unsigned char LXDAT;
extern volatile __sfr unsigned char LXDIVL;
extern volatile __sfr unsigned char LXDIVH;
extern volatile __sfr unsigned char MDUCON;
extern volatile __sfr unsigned char MDUDAT;
extern volatile __sfr unsigned char EXIE;
extern volatile __sfr unsigned char RTCSS;
extern volatile __sfr unsigned char RTAS;
extern volatile __sfr unsigned char RTAM;
extern volatile __sfr unsigned char RTAH;
extern volatile __sfr unsigned char RTMSS;
extern volatile __sfr unsigned char RTCIF;
extern volatile __sfr unsigned char LVDCON;

extern volatile __sfr unsigned char B;
extern volatile __sfr unsigned char RTCON;
extern volatile __sfr unsigned char RTCS;
extern volatile __sfr unsigned char RTCM;
extern volatile __sfr unsigned char RTCH;
extern volatile __sfr unsigned char RTCDL;
extern volatile __sfr unsigned char RTCDH;
extern volatile __sfr unsigned char INDEX;
extern volatile __sfr unsigned char EXIP;
extern volatile __sfr unsigned char EPIE;
extern volatile __sfr unsigned char EPIF;
extern volatile __sfr unsigned char EPCON;
extern volatile __sfr unsigned char IDLSTL;
extern volatile __sfr unsigned char IDLSTH;
extern volatile __sfr unsigned char STPSTL;
extern volatile __sfr unsigned char STPSTH;


/*  BIT Register  */
/*	PSW            */
extern volatile __sbit unsigned char CY;
extern volatile __sbit unsigned char AC;
extern volatile __sbit unsigned char F0;
extern volatile __sbit unsigned char RS1;
extern volatile __sbit unsigned char RS0;
extern volatile __sbit unsigned char OV;
extern volatile __sbit unsigned char DPS;
extern volatile __sbit unsigned char P;
		  		
/*	TCON */
extern volatile __sbit unsigned char TF1;
extern volatile __sbit unsigned char TR1;
extern volatile __sbit unsigned char TF0;
extern volatile __sbit unsigned char TR0;
extern volatile __sbit unsigned char IE1;
extern volatile __sbit unsigned char IT1;
extern volatile __sbit unsigned char IE0;
extern volatile __sbit unsigned char IT0;

/*	S0CON   */
extern volatile __sbit unsigned char SM0;
extern volatile __sbit unsigned char SM1;
extern volatile __sbit unsigned char SM2;
extern volatile __sbit unsigned char REN;
extern volatile __sbit unsigned char TB8;
extern volatile __sbit unsigned char RB8;
extern volatile __sbit unsigned char TI0;
extern volatile __sbit unsigned char RI0;

/*	IE */
extern volatile __sbit unsigned char EA;
extern volatile __sbit unsigned char ES1;
extern volatile __sbit unsigned char ET2;
extern volatile __sbit unsigned char ES0;
extern volatile __sbit unsigned char ET1;
extern volatile __sbit unsigned char EX1;
extern volatile __sbit unsigned char ET0;
extern volatile __sbit unsigned char EX0;

/*	IP */
extern volatile __sbit unsigned char PS1;
extern volatile __sbit unsigned char PT2;
extern volatile __sbit unsigned char PS0;
extern volatile __sbit unsigned char PT1;
extern volatile __sbit unsigned char PX1;
extern volatile __sbit unsigned char PT0;
extern volatile __sbit unsigned char PX0;

/*	T2CON */
extern volatile __sbit unsigned char TF2;
extern volatile __sbit unsigned char TR2;
extern volatile __sbit unsigned char T2R1;
extern volatile __sbit unsigned char T2R0;
extern volatile __sbit unsigned char T2IE;
extern volatile __sbit unsigned char UCKS;
extern volatile __sbit unsigned char T2P1;
extern volatile __sbit unsigned char T2P0;

/*	EXIE */
extern volatile __sbit unsigned char INT9EN;
extern volatile __sbit unsigned char INT8EN;
extern volatile __sbit unsigned char INT7EN;
extern volatile __sbit unsigned char INT6EN;
extern volatile __sbit unsigned char INT5EN;
extern volatile __sbit unsigned char INT4EN;
extern volatile __sbit unsigned char INT3EN;
extern volatile __sbit unsigned char INT2EN;

/*	P0 */
extern volatile __sbit unsigned char P00;
extern volatile __sbit unsigned char P01;
extern volatile __sbit unsigned char P02;
extern volatile __sbit unsigned char P03;
extern volatile __sbit unsigned char P04;
extern volatile __sbit unsigned char P05;
extern volatile __sbit unsigned char P06;
extern volatile __sbit unsigned char P07;
/*	P1 */
extern volatile __sbit unsigned char P10;
extern volatile __sbit unsigned char P11;
extern volatile __sbit unsigned char P12;
extern volatile __sbit unsigned char P13;
extern volatile __sbit unsigned char P14;
extern volatile __sbit unsigned char P15;
extern volatile __sbit unsigned char P16;
extern volatile __sbit unsigned char P17;
/*	P2 */
extern volatile __sbit unsigned char P20;
extern volatile __sbit unsigned char P21;
extern volatile __sbit unsigned char P22;
extern volatile __sbit unsigned char P23;
extern volatile __sbit unsigned char P24;
extern volatile __sbit unsigned char P25;
extern volatile __sbit unsigned char P26;
extern volatile __sbit unsigned char P27;

/*	P3 */
extern volatile __sbit unsigned char P30;
extern volatile __sbit unsigned char P31;
extern volatile __sbit unsigned char P32;
extern volatile __sbit unsigned char P33;
extern volatile __sbit unsigned char P34;
extern volatile __sbit unsigned char P35;
extern volatile __sbit unsigned char P36;
extern volatile __sbit unsigned char P37;

/*	P4 */
extern volatile __sbit unsigned char P40;
extern volatile __sbit unsigned char P41;
extern volatile __sbit unsigned char P42;
extern volatile __sbit unsigned char P43;
extern volatile __sbit unsigned char P44;
extern volatile __sbit unsigned char P45;
extern volatile __sbit unsigned char P46;
extern volatile __sbit unsigned char P47;

///*	P5	*/
extern volatile __sbit unsigned char P50;
extern volatile __sbit unsigned char P51;
extern volatile __sbit unsigned char P52;
extern volatile __sbit unsigned char P53;
extern volatile __sbit unsigned char P54;
extern volatile __sbit unsigned char P55;
extern volatile __sbit unsigned char P56;
extern volatile __sbit unsigned char P57;
//
///*	P6	*/
#define P60(n)	{if(n) P6|=0x01;else P6&=~0x01;}
#define P61(n)	{if(n) P6|=0x02;else P6&=~0x02;}
#define P62(n)	{if(n) P6|=0x04;else P6&=~0x04;}
#define P63(n)	{if(n) P6|=0x08;else P6&=~0x08;}
#define P64(n)	{if(n) P6|=0x10;else P6&=~0x10;}
#define P65(n)	{if(n) P6|=0x20;else P6&=~0x20;}
#define P66(n)	{if(n) P6|=0x40;else P6&=~0x40;}
#define P67(n)	{if(n) P6|=0x80;else P6&=~0x80;}
//
///*	P7	*/
#define P70(n)	{if(n) P7|=0x01;else P7&=~0x01;}
#define P71(n)	{if(n) P7|=0x02;else P7&=~0x02;}
#define P72(n)	{if(n) P7|=0x04;else P7&=~0x04;}
#define P73(n)	{if(n) P7|=0x08;else P7&=~0x08;}
#define P74(n)	{if(n) P7|=0x10;else P7&=~0x10;}
#define P75(n)	{if(n) P7|=0x20;else P7&=~0x20;}

#endif  
