/********************************************************************************/
/* Power-key LED / backlight module (board 002_WF1): the single P3.0-driven    */
/* backlight. The r02 LED-matrix driver and the PWM channels are gone - P5.0..  */
/* P5.2 are LCD SEG4..SEG6 of the glass.                                        */
/*                                                                              */
/* Brightness = timer-ISR soft PWM, autonomous from the main loop:              */
/* T2 (16-bit auto-reload, vendor Timer2 mode) counts at FOSC with a 192-count  */
/* base tick (0xFF40); the carrier ISR (src/timer2_backlight_isr.s, hand asm)   */
/* divides the tick into 128 software phases: P3.0 is high for the first        */
/* `level` phases of each 128-tick carrier period (duty = level/128). The       */
/* reload register is written ONCE at init - this chip ignores runtime          */
/* rewrites of T2CH/T2CL (the value is latched at TR2; an edge-scheduled        */
/* variant that rewrote the reload at every edge produced a constant 50%        */
/* square at the base-tick rate instead of the scheduled edges). One raw duty   */
/* step is 1/128 of the period. UI levels 0..8 reach Led_SetLevel as raw steps  */
/* through the linear map in Lcd_SetBrightness (level 1 = 1/128 = the          */
/* calibrated minimum, level 8 = full duty). The ISR runs every tick (~19 kHz) */
/* and is hand-written asm to keep its cost near ~12% CPU (the C ISR with the   */
/* backend's register-save prologue costs ~40% at this rate).                   */
/* T1 was tried first but measures as PRESCALED by 256 on this chip (level.sr  */
/* capture: 256 counts ticked ~1.2 kHz -> a 4.7 Hz visible blink) - T2 counts */
/* at FOSC per the vendor example, no prescaler. P3.0 has no hardware-PWM pin  */
/* function (PWM outputs are P5.0..3 / P3.2 / P3.3); the ISR RMW of P3 races   */
/* only with Relay_Set's own P3.1 bit writes (one carrier period of skew -     */
/* invisible).                                                                 */
/********************************************************************************/
#include "include/stdint.h"
#include "include/config.h"
#include "include/ca51f2sfr.h"
#include "include/ca51f2xsfr.h"
#include "include/gpiodef_f2.h"
#include "include/led.h"

#define PWM_TICK_RELOAD  0xFF40
#define PWM_DUTY_MAX     128         /* raw step range for Led_SetLevel          */

/* Power-key LED (P3.2 -> R18 -> LED): hardware PWM7, pair clock via CH6.       */
/* Programming copied from the stock firmware (bin/orig dump @0x4810): the pair */
/* clock is CKS_IH (IRCH, not SYS), period = 122 counts (FOSC/30 kHz), and     */
/* full ON is DUT == period. Duty 0..122; the pin function is P32_PWM7 (5).     */
#define PWRLED_PERIOD    122                /* stock value: FOSC/30k, fits byte */

/* Phase/level live in FIXED XRAM cells shared with the asm ISR                  */
/* (src/timer2_backlight_isr.s addresses them by literal). The cells sit at the  */
/* top of XRAM, above every C bss object (verified in the link map).             */
#define PWM_PHASE_CELL   (*(volatile uint8_t xdata *)0x07F0)
#define PWM_LEVEL_CELL   (*(volatile uint8_t xdata *)0x07F1)

/* Sentinel: the asm ISR has not run yet (its first action turns this into a    */
/* real phase 0..127).                                                           */
#define PWM_PHASE_INIT   0xFF

/* Raw duty 0..PWM_DUTY_MAX (1/128 of the carrier period each): 0 = off. Called  */
/* from Lcd_SetBrightness (UI levels 0..8 via the linear map) and from the      */
/* brightness bench test (raw steps). This is the ONLY place that decides how   */
/* bright the backlight is on this board.                                       */
void Led_SetLevel(uint8_t level) {
	if(level > PWM_DUTY_MAX) {
		level = PWM_DUTY_MAX;
	}
	PWM_LEVEL_CELL = level;
}

void Pwm_Backlight_Init(void) {
	/* Push-pull, active-HIGH; boot sequence = LCD init -> clear -> backlight ON */
	/* at full duty until the UI applies the configured level.                   */
	P30F = 2;
	P3 |= 0x01;
	PWM_LEVEL_CELL = PWM_DUTY_MAX;
	PWM_PHASE_CELL = PWM_PHASE_INIT;

	/* Vendor Timer2 auto-reload wiring (Timer2_Auto_Reload_Mode example):       */
	/* T2R(2) = auto-reload mode0, T2IE on; CCFG/T2M = timer function. The       */
	/* reload pair is written once here and never at runtime (see the header).   */
	T2CON = 0x28;                        /* T2R=2 (0x20) | T2IE (0x08)           */
	T2MOD = 0x00;                        /* timer mode, no capture               */
	T2CH = (uint8_t)(PWM_TICK_RELOAD >> 8);
	T2CL = (uint8_t)(PWM_TICK_RELOAD);
	TH2  = (uint8_t)(PWM_TICK_RELOAD >> 8);
	TL2  = (uint8_t)(PWM_TICK_RELOAD);
	T2CON |= 0x40;                       /* TR2                                  */
	ET2 = 1;

	/* Power-key LED: hardware PWM7 follows the same UI level as the LCD         */
	/* backlight (see PowerLed_SetLevel). Boot = full until the UI applies.       */
	/* Register access is INDEXed; every other INDEX user (LCD) sets INDEX first, */
	/* so sharing the register is safe. Sequence = the stock firmware's.          */
	INDEX = 6;                           /* PWM_CH6: clock of the PWM6/7 pair     */
	PWMCON = 0x01;                       /* CKS_IH (IRCH), IRQs off              */
	PWMCFG = 0;                          /* TOG = 0 (normal polarity)            */
	INDEX = 7;                           /* PWM_CH7: the LED channel             */
	PWMCON = 0;                          /* MS(0) edge-aligned, MOD(0), no IRQs  */
	PWMCFG = 0;
	PWMDIVH = (uint8_t)(PWRLED_PERIOD >> 8);
	PWMDIVL = (uint8_t)(PWRLED_PERIOD);
	PWMDUTH = (uint8_t)(PWRLED_PERIOD >> 8);
	PWMDUTL = (uint8_t)(PWRLED_PERIOD);  /* full duty until Bright_Apply          */
	P32F = P32_PWM7_SETTING;             /* pin function = PWM7                   */
	PWMUPD |= 0x80;                      /* latch DIV/DUT of CH7                 */
	while(PWMUPD) {
	}
	PWMEN |= 0x80;                       /* enable CH7                           */
}

/* Power-key LED level 0..8 (the UI scale, same numbers as the LCD backlight).  */
/* Calibrated by eye through a camera: the LED's first VISIBLE step sits where  */
/* the old curve's level 3 was (duty 22/122), so the new ladder starts there   */
/* and runs linearly to full: {0,22,36,51,65,79,93,108,122} (steps ~14.3).     */
/* Hardware PWM - no ISR cost. The R18/LED chain is the ceiling (A/B vs GPIO    */
/* measured equal): the curve delivers what the hardware can show.              */
void PowerLed_SetLevel(uint8_t level) {
	static const uint8_t code pwled_map[9] = {0, 22, 36, 51, 65, 79, 93, 108, 122};

	if(level > LED_BRIGHTNESS_MAX) {
		level = LED_BRIGHTNESS_MAX;
	}
	INDEX = 7;                           /* PWM_CH7 register window               */
	PWMDUTH = 0;
	PWMDUTL = pwled_map[level];
	PWMUPD |= 0x80;
	while(PWMUPD) {
	}
}

/* The r02 four-channel API stays for the composition, but this board has ONE   */
/* backlight (P3.0) whose level comes from Led_SetLevel - channel writes are    */
/* ignored (composite channel sequences used to glitch the shared pin).         */
void Led_Backlight(uint8_t ch, uint8_t level) {
	(void)ch;
	(void)level;
}
