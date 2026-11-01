/******************************************************************************/
/* Hardware bench tests: every *_TEST_EN mode in one file, grouped in the     */
/* order of the config.h sections:                                            */
/*   10. LCD bring-up     (LCD_TEST_EN)      - LCD_TestMain()                  */
/*   11. Clock bench      (CLOCK_TEST_EN)    - Clock_TestMain()                */
/*   12. Brightness sweep (BRIGHT_TEST_EN)  - Bright_TestMain()               */
/*   13. Photo freeze     (PHOTO_TEST_EN)    - Photo_ModeMain()                */
/* Each mode is compiled only while its gate is ON; main() diverts into the   */
/* matching Xxx_TestMain() and never returns (no WDT, no normal UI).          */
/******************************************************************************/
#include "include/stdint.h"
#include "include/config.h"

#if LCD_TEST_EN || CLOCK_TEST_EN || BRIGHT_TEST_EN || PHOTO_TEST_EN

#include "include/ca51f2sfr.h"
#include "include/ca51f2xsfr.h"
#include "include/gpiodef_f2.h"
#include "include/lcd.h"
#include "include/led.h"
#include "include/modes.h"
#include "include/delay.h"
#include "include/hw_test.h"

#if LCD_TEST_EN
/******************************************************************************/
/* 10. LCD bring-up bench (config.h section 10).                               */
/* One-shot sequence for the glass board (002_WF1): drives the internal LCD   */
/* driver through the INDEX/LXDAT window. No markers, no relay activity, no   */
/* walking - a fixed three-phase run for a visual check:                       */
/*                                                                              */
/*   1  glass empty, backlight on (2 s)                                         */
/*   2  glass full (2 s)                                                        */
/*   3  font walk: the small post-dot digit counts 1..9,0 (1 s each); then     */
/*      halt.                                                                   */
/*                                                                              */
/* Real usable SEG columns on this glass = 24: 23 bonded SEG pins plus pad28,   */
/* which is P0.4 configured as SEG35 (P04F = 4, exactly as the stock firmware  */
/* does). P04F = 3 would instead make P0.4 COM4: the controller then counts 5  */
/* rows and drives pad28 with a COM waveform, so the four glyphs on that       */
/* column (houses 5/6, circles 5/6) light up permanently and faintly and no    */
/* buffer write can turn them off.                                              */

/* Frame clock = source / ((LXDIV + 1) * 512). The stock uses LEN=PLL (0xA0)   */
/* but starts the PLL itself (CKCON init) - this test runs from IRCH (3.6864 MHz)*/
/* with the PLL off, so the LCD clock MUST come from IRCH:                      */
/*   LEN=010 (IRCH), LMOD=0 (LCD) -> LXCON = 0x40;                             */
/*   3686400 / ((LXDIV + 1) * 512) ~ 64 Hz -> LXDIV = 111 (0x06F).             */
#define LXCON_LCD       0x40
#define LCD_LXDIV_L     0x6F
#define LCD_LXDIV_H     0x00

/* LXCFG (LCD mode, datasheet table 20-3-2): DMOD[7:6] current               */
/* (00=5uA 01=40uA 10=80uA 11=130uA), BIAS[5:4] (01=1/2, 10=1/3, else 1/4),   */
/* LDRV[2:0] drive strength. Reset value 0x00 = 5uA/weak - far too dim.        */
/* 0xE7 = the stock configuration, decoded from its three init helper calls     */
/* (args 3 -> DMOD=130uA, 2 -> BIAS=1/3, 7 -> LDRV=max): stock screen is       */
/* bright on this glass, so copy it verbatim.                                   */
#define LXCFG_STOCK      0xE7

/* Final lit piece: the pair-by-pair attribution walk through the measured     */
/* sym_groups inventory - one (cell,row) pair per build, the glyph gets its    */
/* name recorded in AGENTS.md. Named so far: cells 4..7 = the big digits       */
/* (E/G/F | D/C/B + A), cell3+4 COM3 = fan (6 blades), cell0/COM3 = house 1,   */
/* cell23/COM2 = house 5, cell10 = dot/%RH/F-leg/degree-c column, cell6/COM3 = */
/* roof with the pipe, cell8/COM3 = thermometer inside the house,             */
/* cell13/COM0 = water shut-off valve (for the future, not in the UI),        */
/* cell12/COM1 = sun (for the weather sensor), cell12/COM0 = snowflake         */
/* (not used), cell11/COM3 = network icon, cell1/COM2 = circled 2 = Tuesday,   */
/* cell1/COM1 = circled 3 = Wednesday, cell1/COM0 = circled 4 = Thursday      */
/* (the weekdays live in cell1), cell12/COM2 = lower fan in the mode row       */
/* (heat = sun, cool = snowflake, fan-only = fan - not weather; only the sun   */
/* goes into the UI). Weekdays continue in cell23 - COM0 = circled 5 = Friday, */
/* COM1 = circled 6 = Saturday; COM3 = house 6; cell22/COM0 = circled 7 =      */
/* Sunday. The cell11 0x06 group turned out to be TWO symbols - SET and the    */
/* floor-heat pictogram (the stock lights SET alone) - being split row by row; */
/* cell11/COM1 measured = SET alone, COM2 = the external-sensor pictogram      */
/* (lit while the external probe is connected). This build lights    */
/* cell2 rows COM1-3 (mask 0x0E), the unattributed part of the group whose     */
/* cells 3+4 COM3 are the known fan; cell2 COM1-3 measured = smoke above the   */
/* pipe (a positioning element - the pipe itself has no glyph). This build     */
/* lights cell8 rows COM0-2 (mask 0x07), the next group of sym_groups;        */
/* measured = the left column E/F/G of the small post-dot digit (same         */
/* structure as the big digits). cell9 COM1-2 measured = B and C of its right   */
/* column (COM2 = B, COM1 = C; A and D still unlocated - candidates (9,COM0)   */
/* and (9,COM3), outside sym_groups). cell13/COM1 measured = the lower part    */
/* of the big house (left wall and floor, continuing down from the roof-with-  */
/* pipe at cell6/COM3). cell2/COM0 measured = AUTO (not used in this firmware).*/
/* cell13/COM3 measured = schedule clock (lit while the thermostat runs on     */
/* the schedule). cell12/COM3 measured = padlock (keypad lock), cell11/COM0 =  */
/* Eco. cell0/COM1 = house 3, cell0/COM2 = house 2 (COM3 was already known -   */
/* house 1). cell14/COM3 = the "h" of the 8888-hour counter on the right,      */
/* cell0 = houses 1..4 (COM3..COM0). The sym_groups queue is exhausted; this    */
/* (right column now D/C/B complete); cell9/COM3 measured = A - the small      */
/* post-dot digit is fully mapped (cell8 = E/F/G, cell9 = D/C/B + A). The clock */
/* digit hunt: cell15 = A/B/C/D and cell16 = E/F/G (4th clock digit complete)  */
/* plus the AM label in cell16. cell17 = A/B/C/D right column of the 3rd clock  */
/* digit; cell18 = E/F/G (3rd clock digit complete: {17:ABCD, 18:EFG}) plus    */
/* the colon between clock digits 2 and 3; cell19 = A/B/C/D right column of     */
/* the 2nd clock digit; cell20 = E/F/G (2nd clock digit complete: {19:ABCD,    */
/* 20:EFG}) plus PM above the 2nd digit; cell21 = A/B/C/D right column of the  */
/* 1st clock digit; cell22 = E/F/G (1st clock digit complete: {21:ABCD,        */
/* 22:EFG}) plus the known circled 7 - all four clock digits mapped. The walk  */
/* mapped: 1-4 in cell1, 5-6 in cell23, 7 in cell22). cell3 COM0-2 measured =  */
/* the fan hub dot (confirmed by the fan/dot alternation probe). cell13/COM2   */
/* measured = the hand (manual setpoint mode, opposite the schedule clock).    */
/* cell14 COM0-2 measured together = the ON and OFF labels right of the "h";    */
/* being split row by row - cell14/COM0 empty, COM1 = ON; this build lights    */
/* cell14/COM2 (expected OFF).                                                  */
/* Font walk on the SMALL post-dot digit (cells 8/9): position counts 1..9,0   */
/* with 1 s per value - then halt. Rendering uses the ASSUMED small-digit rows */
/* (right cell9: A=COM3,B=COM2,C=COM1,D=COM0; left cell8: E=COM0,F=COM2,G=COM1),*/
/* so any wrong/missing/phantom segment per value pinpoints the row error.      */
static void lcd_write(uint8_t cell, uint8_t val);   /* defined below            */

static const uint8_t code probe_seg7[10] = {
	0x3F, 0x06, 0x5B, 0x4F, 0x66,   /* 0 1 2 3 4 */
	0x6D, 0x7D, 0x07, 0x7F, 0x6F    /* 5 6 7 8 9 */
};

static void probe_small_draw(uint8_t seg7) {
	uint8_t v;

	v = 0;
	if(seg7 & 0x01) v |= 0x08;         /* A: assumed cell9 COM3                 */
	if(seg7 & 0x02) v |= 0x04;         /* B: COM2                              */
	if(seg7 & 0x04) v |= 0x02;         /* C: COM1                              */
	if(seg7 & 0x08) v |= 0x01;         /* D: COM0                              */
	lcd_write(9, v);

	v = 0;
	if(seg7 & 0x10) v |= 0x01;         /* E: assumed cell8 COM0                 */
	if(seg7 & 0x40) v |= 0x02;         /* G: COM1                              */
	if(seg7 & 0x20) v |= 0x04;         /* F: COM2                              */
	lcd_write(8, v);
}

static void lcd_write(uint8_t cell, uint8_t val) {
	INDEX = cell;
	LXDAT = (uint8_t)(val & 0x0F);
}

static void lcd_init(void) {
	/* Bonded SEG/COM pins to their LCD function - the EXACT stock pin set:      */
	/* P0.0..P0.3 = COM0..COM3 (value 3), every bonded SEG = 3, and P0.4 = 4     */
	/* (SEG35 = the 24th column on pad28; stock writes 4 there). Value 3 on      */
	/* P0.4 would select COM4 instead and break the 4th column.                  */
	P00F = 3; P01F = 3; P02F = 3; P03F = 3; P04F = 4;
	P10F = 3; P11F = 3; P12F = 3; P13F = 3; P16F = 3; P17F = 3;
	P20F = 3; P21F = 3; P22F = 3; P23F = 3;
	P24F = 3; P25F = 3; P26F = 3; P27F = 3;
	P50F = 3; P51F = 3; P52F = 3;
	P60F = 3; P61F = 3; P62F = 3; P63F = 3; P64F = 3; P65F = 3;

	LXCON  = LXCON_LCD;
	LXDIVL = LCD_LXDIV_L;
	LXDIVH = LCD_LXDIV_H;
	LXCFG  = LXCFG_STOCK;                /* stock 1/3 bias for a 4-row duty     */

	/* Backlight control: P3.0 (pin 47) -> R25 -> Q3 -> R26 -> backlight A      */
	/* (K tied to +5V). Active-HIGH: level HIGH = light ON (confirmed: the       */
	/* adapter's idle HIGH on J1.2 lights it). P30F must be 2 (push-pull) first: */
	/* the reset default is not a strong output. Start ON.                       */
	P3 |= 0x01;
}

/* Measured per-icon (cell, COM-row mask) pairs from the mapping videos - the   */
/* map work data. NOT referenced by the current one-shot sequence; kept as the  */
/* only copy of the measurements in this tree.                                  */
const uint8_t code sym_groups[][9] = {
	{  4, 0x07,  5, 0x0F,  6, 0x07,  7, 0x0F, 0xFF }, /* big digits "88"        */
	{  8, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 13, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 12, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 12, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 11, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{  1, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 12, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{  1, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{  1, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 23, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 23, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 23, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 22, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	/* second pass over the same glass, measured with a tighter frame inset    */
	{  6, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 11, 0x06, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{  2, 0x0E,  3, 0x08,  4, 0x08, 0xFF, 0xFF, 0xFF },
	{  8, 0x07,  9, 0x06, 10, 0x02, 0xFF, 0xFF, 0xFF },
	{ 13, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{  2, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 13, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 10, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 12, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 11, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{  0, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{  0, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{  0, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 14, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{  0, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 10, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	{ 23, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};

static void lcd_fill_n(uint8_t ncells, uint8_t val) {
	uint8_t i;

	for(i = 0; i < ncells; i++) {
		lcd_write(i, val);
	}
}

static void lcd_clear_all(void) {
	lcd_fill_n(LCD_CELLS, 0);
}

void LCD_TestMain(void) {
	/* P3.0/P3.1 push-pull (datasheet ch.15.3 example: PxF = 2). Backlight is   */
	/* active-HIGH; the relay driver is active-HIGH too, so hold P3.1 at 0:     */
	/* no clicks, and no floating-on from the reset default.                     */
	P30F = 2;
	P31F = 2;
	P3 &= (uint8_t)~0x02;

	lcd_init();                           /* backlight starts ON and stays on    */

	/* Phase 1: glass empty - nothing may stay lit after a clear.                */
	lcd_clear_all();
	Delay_ms(2000);

	/* Phase 2: glass full - one frame with every glyph on.                      */
	lcd_fill_n(LCD_CELLS, LCD_SEG_ALL);
	Delay_ms(2000);

	/* Phase 3: hold ONE value on the small digit - "4" (B,C,F,G) - nothing    */
	/* else on the glass, then halt; isolates the digit from any UI overwrite.  */
	lcd_clear_all();
	probe_small_draw(probe_seg7[4]);

	for(;;) {
	}
}
#endif /* LCD_TEST_EN */

#if CLOCK_TEST_EN
/******************************************************************************/
/* 11. Clock measurement bench (config.h section 11).                          */
/* For a logic analyzer on J1: P3.0 = J1 pin 2, GND = J1 pin 1 (J1 pin 4 is  */
/* +5V - do not clip the analyzer onto it).                                     */
/*                                                                              */
/* The whole cycle is ~3 s so that even a short capture gets every phase, and   */
/* the UART burst - the absolute timing anchor - comes FIRST. Segmentation of a */
/* trace needs nothing but the >=100 ms static markers between the phases:       */
/*                                                                              */
/*   [HIGH 300 ms]  phase C  UART0 TX burst  - Timer2 baud generator, reload     */
/*               0xFFFF, so the line rate is exactly f_SYS / 32: measure one     */
/*               bit (every run of 0x55 is exactly one bit) -> f_SYS. Also shows */
/*               the real baud the firmware would put on the wire.              */
/*   [LOW 300 ms]   phase B  toggle via Delay_ms(1) - measured half period /     */
/*               1 ms is the exact Delay_ms scale error                          */
/*   [HIGH 300 ms]  phase A  raw toggle loop, no delay calls - f_SYS cross-check */
/*               against the loop's instruction count (needs the core timing     */
/*               model, so it only corroborates phase C)                        */
/*                                                                              */
/* P3.0 also feeds the backlight chain (R25 -> Q3), so phases A/B just strobe it */
/* - it looks dimmer, harmless.                                                  */
/*                                                                              */
/* NOTE: phase A inverts the SFR bit directly - `P30 = !P30;` lowers to the     */
/* 2-byte `cpl P30`. An older toolchain build turned bit inversion into a plain  */
/* copy (load + store, no CPL): a static level instead of a square wave in the   */
/* trace means suspect the compiler, not the board.                              */

/* Phase A: no time base, only an iteration counter - a few hundred ms at a      */
/* nominal 3.6864 MHz clock (2 x 0xFFFF iterations; the 16-bit loop counter      */
/* dominates the per-iteration cost, the toggle itself is a single CPL).         */
#define CT_RAW_OUTER   2

/* Phase B: 300 periods of 2 ms nominal = 600 measured half periods (~0.84 s).   */
#define CT_DELAY_LOOPS 300

/* Phase C: 1000 bytes of 0x55 = 10000 bit intervals, ~0.12 s of traffic.       */
#define CT_UART_BYTES   1000

/* Markers: 300 ms nominal (x1.4 real) - far above the 100 ms the trace          */
/* segmentation treats as static, and far below any logic-analyzer buffer.       */
#define CT_MARKER_MS    300

/* Bounded wait for the TX flag: a stuck UART must not stall the whole bench.    */
#define CT_TI_TIMEOUT   0xFFFFU

/* Same reload formula as src/debug.c: baud = FOSC / (32 * (0x10000 - reload)). */
/* At FOSC 3686400 and 115200 it folds to 0xFFFF, so baud = FOSC / 32 and        */
/* f_SYS = 32 * (measured baud).                                                 */
#define CT_UART_BAUD    115200UL
#define CT_UART_RELOAD  ((uint16_t)(0x10000UL - (FOSC / (CT_UART_BAUD * 32UL))))

static void ct_phase_raw(void) {
	uint16_t outer;
	uint16_t inner;

	for(outer = 0; outer < CT_RAW_OUTER; outer++) {
		for(inner = 0; inner < 0xFFFF; inner++) {
			P30 = !P30;
		}
	}
}

static void ct_phase_delay(void) {
	uint16_t i;

	for(i = 0; i < CT_DELAY_LOOPS; i++) {
		P30 = 1;
		Delay_ms(1);
		P30 = 0;
		Delay_ms(1);
	}
}

static void ct_uart_putc(uint8_t c) {
	uint16_t t = CT_TI_TIMEOUT;

	S0BUF = c;
	while(!TI0) {
		if(--t == 0) {
			break;
		}
	}
	TI0 = 0;
}

static void ct_phase_uart(void) {
	uint16_t i;

	/* UART0 on P3.0 (J1-2), Timer2 as baud generator (T2CON: reload mode0 +   */
	/* UCKS=1 -> UART0 clock from Timer2 overflow, T2P=00 -> Timer2 counts the   */
	/* system clock directly), REN left clear so nothing listens on the relay    */
	/* pin.                                                                       */
	P30F = P30_UART0_TX_SETTING;
	T2CON = 0x24;
	T2CH  = (uint8_t)(CT_UART_RELOAD >> 8);
	T2CL  = (uint8_t)(CT_UART_RELOAD);
	TH2   = (uint8_t)(CT_UART_RELOAD >> 8);
	TL2   = (uint8_t)(CT_UART_RELOAD);
	TR2   = 1;
	S0CON = 0x40;                     /* mode 1 (8-bit UART), receiver disabled */

	for(i = 0; i < CT_UART_BYTES; i++) {
		ct_uart_putc(0x55);           /* start + 101010101 + stop: every run = 1 bit */
	}

	TR2   = 0;
	S0CON = 0x40;
	P30F  = 2;                        /* hand the pin back to GPIO (latch = 1 here) */
}

void Clock_TestMain(void) {
	uint8_t i;

	/* Push-pull output (datasheet ch.15.3: PxF = 2): the reset default (0) is   */
	/* not a strong drive, which would round the edges off for the analyzer.      */
	P30F = 2;
	P32F = 2;

	/* Marker: the POWER LED (P3.2) blinks twice - execution reached this point. */
	for(i = 0; i < 2; i++) {
		P32 = 1;
		Delay_ms(300);
		P32 = 0;
		Delay_ms(300);
	}

	for(;;) {
		P30 = 1;                       /* park HIGH: the UART takes the pin from   */
		Delay_ms(CT_MARKER_MS);        /* this level without an edge                */
		ct_phase_uart();
		P30 = 0;
		Delay_ms(CT_MARKER_MS);
		ct_phase_delay();
		P30 = 1;
		Delay_ms(CT_MARKER_MS);
		ct_phase_raw();
	}
}
#endif /* CLOCK_TEST_EN */

#if BRIGHT_TEST_EN
/******************************************************************************/
/* 12. Brightness sweep bench (config.h section 12).                           */
/* The glass shows the UI level under test as a big digit while Bright_Apply  */
/* walks 1..8 - the production fan-out (screen backlight via the linear map   */
/* + asm ISR, power-key LED via hardware PWM7), dwelling BRIGHT_TEST_SECS     */
/* seconds per level and looping forever - one video records the whole ladder */
/* and the screen/LED synchronization. The LED's ceiling is the R18/LED chain */
/* (A/B vs GPIO-HIGH measured equal), so the ladder tops out at whatever the  */
/* hardware can give.                                                          */
/* main() diverts here BEFORE Uart1/EA, so this function brings up the pieces */
/* it needs itself: the carriers (Pwm_Backlight_Init arms ET2 + PWM7) and the */
/* interrupt system (EA). Nothing else runs.                                   */
void Bright_TestMain(void) {
	uint8_t level;

	Lcd_Init();
	Lcd_Clear();
	Pwm_Backlight_Init();             /* T2 carrier + PWM7 power LED, full      */
	EA = 1;                           /* the carrier ISR may drive P3.0 now    */

	for(;;) {
		for(level = 1; level <= 8; level++) {
			Lcd_Clear();
			Lcd_DigitIdx(2, level);   /* big units digit = level under test    */
			Lcd_Flush();
			Bright_Apply(level);       /* screen + power LED, production path   */
			Delay_ms((uint16_t)BRIGHT_TEST_SECS * 1000u);
		}
	}
}
#endif /* BRIGHT_TEST_EN */

#if PHOTO_TEST_EN
/******************************************************************************/
/* 13. Photo/documentation freeze (config.h section 13).                       */
/* The panel and the Power LED sit at FULL brightness, every segment of the   */
/* glass is lit, and the device freezes in that state until the next reset -  */
/* for documentation photos. The divert runs before WDT_Init and before       */
/* Uart1/EA, so this function brings up the carriers itself (Pwm_Backlight   */
/* Init arms ET2 + PWM7; EA lets the T2 ISR drive P3.0). No watchdog, no      */
/* link traffic.                                                               */
void Photo_ModeMain(void) {
	uint8_t i;

	Pwm_Backlight_Init();             /* T2 carrier + PWM7, both at full        */
	EA = 1;                           /* the carrier ISR may drive P3.0 now     */
	Lcd_SetBrightness(LED_BRIGHTNESS_MAX);
	PowerLed_SetLevel(LED_BRIGHTNESS_MAX);

	/* every segment of the glass (same fill as the boot window's photo phase) */
	for(i = 0; i < LCD_CELLS; i++) {
		Lcd_WriteCell(i, LCD_SEG_ALL);
	}
	Lcd_Flush();

	for(;;) {
		/* frozen until reset */
	}
}
#endif /* PHOTO_TEST_EN */

#endif /* any test gate */
