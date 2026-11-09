/******************************************************************************/
#include "include/stdint.h"
#include "include/ca51f2sfr.h"
#include "include/ca51f2xsfr.h"
#include "include/lcd.h"
#include "include/led.h"
#include "include/led_format.h"
#include "include/chars.h"
#include "include/debug.h"
#include "include/modes.h"          /* lock_flash/net_flash/trace_tail: LOCK_TRACE_EN */
/******************************************************************************/
/* Map source: doc_local/lcd_cell_map.md (24 cells x 4 rows).                  */
/*                                                                              */
/* The shadow buffer holds one byte per cell; drawing sets (cell,row) bits,     */
/* Lcd_Flush() pushes the buffer through INDEX/LXDAT. Never touch INDEX from    */
/* an ISR (shared with MDU and touch scan).                                     */

/* Frame clock = IRCH / ((LXDIV + 1) * 512): 3686400 / (112 * 512) ~ 64 Hz.   */
#define LCD_LXCON       0x40        /* LEN=010 (IRCH), LMOD=0 (LCD)              */
#define LCD_LXDIVL      0x6F
#define LCD_LXDIVH      0x00
#define LCD_LXCFG       0xE7        /* stock: DMOD=130uA, BIAS=1/3, LDRV=max     */

/* Stock pin set: P0.0..P0.3 = COM0..3 (3), every bonded SEG = 3, and P0.4 = 4  */
/* (SEG35 = 24th column on pad28). P04F = 3 would select COM4 instead: the     */
/* controller counts 5 rows and pad28's glyphs light up permanently.            */

/* r02 index-style positions (kept for Lcd_DigitIdx/Lcd_CharIdx).              */
#define LCD_DIG_T       3           /* big tens  = big digit pos 0               */
#define LCD_DIG_U       2           /* big units = big digit pos 1               */

/* Master switch for the screen output: 0 = no glyphs may reach the glass.      */
/* Boot sequence is then exactly Lcd_Init + Lcd_Clear + backlight; the shadow   */
/* buffer never changes after the boot clear, so every Lcd_Flush() just         */
/* rewrites zeros. Lcd_Init/Lcd_Clear/Lcd_Flush/Lcd_SetBrightness stay live.    */
/* 1 = normal UI output (composition draws; verify zone by zone on the glass).  */
#define LCD_DRAW_ENABLE 1

typedef struct {
	uint8_t cell;
	uint8_t mask;
} lcd_pair_t;

static xdata uint8_t lcd_buf[LCD_CELLS];

#if LCD_DRAW_ENABLE
/* SYM_* -> (cell, mask) of this glass; 0xFF = no such glyph on this glass      */
/* (no-op so the composition keeps its ids).                                    */
/* Clock specials are at COM0 of their columns (row0 bench probe + the          */
/* font-walk solve): AM = cell16, PM = cell20, colon = cell18, circled7 = cell22*/
static const uint8_t code sym_cell[SYM_COUNT] = {
	[SYM_SUNNY]  = 12,  [SYM_CLOUD]  = 0xFF, [SYM_RAIN]   = 0xFF, [SYM_SNOW]   = 12,
	[SYM_HEAT]   = 2,   /* heating indicator = smoke above the pipe             */
	[SYM_DEGC]   = 10,  [SYM_DP5]    = 10,  [SYM_MON]    = 1,   [SYM_TUE]    = 1,
	[SYM_WED]    = 1,   [SYM_THU]    = 1,   [SYM_FRI]    = 23,  [SYM_SAT]    = 23,
	[SYM_SUN]    = 22,  [SYM_NET]    = 11,  [SYM_SP1]    = 0,   [SYM_SP2]    = 0,
	[SYM_SP3]    = 0,   [SYM_SP4]    = 0,   [SYM_SP5]    = 23,  [SYM_SP6]    = 23,
	[SYM_PM]     = 20,  [SYM_AM]     = 16,  [SYM_MOON]   = 0xFF,[SYM_CLOCK]  = 13,
	[SYM_HAND]   = 13,  [SYM_HEATER] = 11,  [SYM_SET]    = 11,  [SYM_HOME]   = 0xFE,
	[SYM_LOCK]   = 12,  [SYM_ECO]    = 11,  [SYM_HUMID]  = 10,  [SYM_H]      = 14,
	[SYM_ON]     = 14,  [SYM_OFF]    = 14,  [SYM_THERMO] = 8,
};

static const uint8_t code sym_mask[SYM_COUNT] = {
	[SYM_SUNNY]  = 0x02, [SYM_CLOUD]  = 0x00, [SYM_RAIN]   = 0x00, [SYM_SNOW]   = 0x01,
	[SYM_HEAT]   = 0x0E, /* smoke: cell2 rows COM1-3                              */
	[SYM_DEGC]   = 0x08, [SYM_DP5]    = 0x01, [SYM_MON]    = 0x08, [SYM_TUE]    = 0x04,
	[SYM_WED]    = 0x02, [SYM_THU]    = 0x01, [SYM_FRI]    = 0x01, [SYM_SAT]    = 0x02,
	[SYM_SUN]    = 0x01, [SYM_NET]    = 0x08, [SYM_SP1]    = 0x08, [SYM_SP2]    = 0x04,
	[SYM_SP3]    = 0x02, [SYM_SP4]    = 0x01, [SYM_SP5]    = 0x04, [SYM_SP6]    = 0x08,
	[SYM_PM]     = 0x01, [SYM_AM]     = 0x01, [SYM_MOON]   = 0x00,[SYM_CLOCK]  = 0x08,
	[SYM_HAND]   = 0x04, [SYM_HEATER] = 0x04, [SYM_SET]    = 0x02, [SYM_HOME]   = 0x00,
	[SYM_LOCK]   = 0x08, [SYM_ECO]    = 0x01, [SYM_HUMID]  = 0x02, [SYM_H]      = 0x08,
	[SYM_ON]     = 0x02, [SYM_OFF]    = 0x04, [SYM_THERMO] = 0x08,
};

/* SYM_HOME spans three pairs: roof+pipe, lower house part, thermometer inside. */
/* The thermometer pair is addressed alone as SYM_THERMO (value windows clear   */
/* it; the SYM_HOME writes in the temperature view light it again).             */
static const lcd_pair_t code sym_home_pairs[] = {
	{ 6, 0x08 }, { 13, 0x02 }, { 8, 0x08 }
};
#define SYM_HOME_N     (sizeof(sym_home_pairs) / sizeof(sym_home_pairs[0]))
#endif /* LCD_DRAW_ENABLE */

/* Per-segment (cell,row) pairs, order a,b,c,d,e,f,g (7-seg pattern bits).     */
/* Rows solved from the font-walk bench video (all 10 values per position):     */
/*   right cell (21/19/17/15): A=COM0, B=COM1, C=COM2, D=COM3  (A/D flipped     */
/*     vs the first assumption per the owner's row0 probe + re-read of the       */
/*     video; B/C swapped vs the first assumption);                              */
/*   left cell  (22/20/18/16): special glyph = COM0 (AM / colon / PM / circled7),*/
/*     F=COM1, G=COM2, E=COM3 (E lives in COM3 - it is never lit by the old      */
/*     mapping, which is why E vanished and the COM0 specials misfired).         */
static const lcd_pair_t code big_pairs[2][7] = {
	/* pos 0: left digit = cells 4 (E/G/F) + 5 (D/C/B/A) - big digits, rows    */
	/* measured during the map walk (unchanged)                                 */
	{ { 5, 0x08 }, { 5, 0x04 }, { 5, 0x02 }, { 5, 0x01 },
	  { 4, 0x01 }, { 4, 0x04 }, { 4, 0x02 } },
	/* pos 1: right digit = cells 6 (E/G/F) + 7 (D/C/B/A)                       */
	{ { 7, 0x08 }, { 7, 0x04 }, { 7, 0x02 }, { 7, 0x01 },
	  { 6, 0x01 }, { 6, 0x04 }, { 6, 0x02 } },
};

static const lcd_pair_t code small_pairs[7] = {
	{ 9, 0x08 }, { 9, 0x04 }, { 9, 0x02 }, { 9, 0x01 },
	{ 8, 0x01 }, { 8, 0x04 }, { 8, 0x02 },
};

/* Clock digits: digit1 = cells 21+22, digit2 = 19+20, digit3 = 17+18,        */
/* digit4 = 15+16 (visual order). SOLVED rows - see the header above.           */
static const lcd_pair_t code clk_pairs[4][7] = {
	/* digit 1: cells 21 (right) + 22 (left)                                   */
	{ { 21, 0x01 }, { 21, 0x02 }, { 21, 0x04 }, { 21, 0x08 },
	  { 22, 0x08 }, { 22, 0x02 }, { 22, 0x04 } },
	/* digit 2: cells 19 + 20                                                   */
	{ { 19, 0x01 }, { 19, 0x02 }, { 19, 0x04 }, { 19, 0x08 },
	  { 20, 0x08 }, { 20, 0x02 }, { 20, 0x04 } },
	/* digit 3: cells 17 + 18                                                   */
	{ { 17, 0x01 }, { 17, 0x02 }, { 17, 0x04 }, { 17, 0x08 },
	  { 18, 0x08 }, { 18, 0x02 }, { 18, 0x04 } },
	/* digit 4: cells 15 + 16                                                   */
	{ { 15, 0x01 }, { 15, 0x02 }, { 15, 0x04 }, { 15, 0x08 },
	  { 16, 0x08 }, { 16, 0x02 }, { 16, 0x04 } },
};

/* ':' between clock digits 2 and 3 - special glyph at COM0 of cell18          */
/* (measured: row0 probe lit the colon there).                                  */
static const lcd_pair_t code colon_pair = { 18, 0x01 };

/* Weekdays Mon..Sun, row-verified.                                            */
static const lcd_pair_t code week_pairs[7] = {
	{ 1, 0x08 },   /* Mon: cell1/COM3 */
	{ 1, 0x04 },   /* Tue */
	{ 1, 0x02 },   /* Wed */
	{ 1, 0x01 },   /* Thu */
	{ 23, 0x01 },  /* Fri */
	{ 23, 0x02 },  /* Sat */
	{ 22, 0x01 },  /* Sun */
};

/* 7-segment digit patterns (a=bit0 .. g=bit6), the r02 font.                  */
static const uint8_t code lcd_seg7[10] = {
	0x3F, 0x06, 0x5B, 0x4F, 0x66,   /* 0 1 2 3 4 */
	0x6D, 0x7D, 0x07, 0x7F, 0x6F    /* 5 6 7 8 9 */
};

static void lcd_pair_set(const lcd_pair_t code *p, uint8_t on) {
#if LCD_DRAW_ENABLE
	if(on) {
		lcd_buf[p->cell] |= p->mask;
	} else {
		lcd_buf[p->cell] &= (uint8_t)~p->mask;
	}
#else
	(void)p;
	(void)on;
#endif
}

static void lcd_draw7(const lcd_pair_t code (*pairs)[7], uint8_t seg7) {
	uint8_t i;

	for(i = 0; i < 7; i++) {
		lcd_pair_set(&(*pairs)[i], (uint8_t)((seg7 >> i) & 0x01));
	}
}

void Lcd_Init(void) {
	/* Bonded SEG/COM pins to their LCD function - the stock pin set.           */
	P00F = 3; P01F = 3; P02F = 3; P03F = 3; P04F = 4;
	P10F = 3; P11F = 3; P12F = 3; P13F = 3; P16F = 3; P17F = 3;
	P20F = 3; P21F = 3; P22F = 3; P23F = 3;
	P24F = 3; P25F = 3; P26F = 3; P27F = 3;
	P50F = 3; P51F = 3; P52F = 3;
	P60F = 3; P61F = 3; P62F = 3; P63F = 3; P64F = 3; P65F = 3;

	LXCON  = LCD_LXCON;
	LXDIVL = LCD_LXDIVL;
	LXDIVH = LCD_LXDIVH;
	LXCFG  = LCD_LXCFG;
}

void Lcd_Flush(void) {
	uint8_t i;

	DEBUG(LOCK_TRACE_EN, { if(lock_flash || net_flash || trace_tail) debug_putc('.'); });
	for(i = 0; i < LCD_CELLS; i++) {
		INDEX = i;
		LXDAT = (uint8_t)(lcd_buf[i] & LCD_SEG_ALL);
	}
}

void Lcd_Clear(void) {
	uint8_t i;

	DEBUG(LOCK_TRACE_EN, { if(lock_flash || net_flash || trace_tail) debug_putc('='); });
	for(i = 0; i < LCD_CELLS; i++) {
		lcd_buf[i] = 0;
	}
	Lcd_Flush();
}

/* Clear the shadow buffer WITHOUT pushing it to the panel. A caller that     */
/* draws a whole screen right after must use this instead of Lcd_Clear(): the  */
/* latter flushes the empty frame itself, and in a periodic repaint (the       */
/* schedule editor blinks its field twice a second) that shows up as the whole */
/* screen blanking for a frame - "cleared and redrawn" flicker.               */
void Lcd_ClearBuf(void) {
	uint8_t i;

	for(i = 0; i < LCD_CELLS; i++) {
		lcd_buf[i] = 0;
	}
}

void Lcd_WriteCell(uint8_t cell, uint8_t val) {
#if LCD_DRAW_ENABLE
	lcd_buf[cell] = (uint8_t)(val & LCD_SEG_ALL);
#else
	(void)cell;
	(void)val;
#endif
}

void Lcd_SetBrightness(uint8_t level) {
	if(level > LCD_BRIGHTNESS_MAX) {
		level = LCD_BRIGHTNESS_MAX;
	}
	DEBUG(LOCK_TRACE_EN, { if(lock_flash || net_flash || trace_tail) debug_putc((uint8_t)('0' + level)); });

	/* Brightness policy: the CONTENT stays visible - the panel keeps the       */
	/* constant drive set up by Lcd_Init (LXCON/LXCFG are not touched here: no  */
	/* washout on a level change, the buffer is never affected). The configured  */
	/* level dims ONLY the P3.0 backlight through the edge-PWM in led.c: UI      */
	/* levels 0..8 map linearly onto raw steps 0..128 (level 1 = 1/128 = the     */
	/* calibrated minimum, level 8 = full duty, 0 = off). The hold-expiry drops  */
	/* the backlight to the day/night level while the clock stays on screen.      */
	{
		/* linear map: 5 + (level-1)*123/7 (rounded), index 0 = off. The minimum  */
		/* is 5/128: below that the once-per-second LCD refresh (clock draw /      */
		/* colon blink) couples an increasingly visible impulse into the backlight */
		/* spark - pure hardware, no firmware modulation (the level write path is  */
		/* cached, the relay pin is a bit write). Calibration ladder: 1/128 and    */
		/* 2/128 clearly pulsed, 4/128 was already borderline, 5/128 is the        */
		/* accepted minimum.                                                        */
		/* The `code` qualifier is load-bearing: without it a function-local       */
		/* static lands in .mcs51.xdata.init, whose load image this project never  */
		/* copies (xdata_clear() zeroes XRAM) - the map would read as all zeros.   */
		static const uint8_t code bright_map[9] = {0, 5, 23, 40, 58, 75, 93, 110, 128};

		Led_SetLevel(bright_map[level]);
	}
}

void Lcd_SymSet(uint8_t id, uint8_t on) {
#if LCD_DRAW_ENABLE
	uint8_t i;

	if(id >= SYM_COUNT) {
		return;
	}
	if(sym_cell[id] == 0xFE) {          /* SYM_HOME: three pairs                */
		for(i = 0; i < (uint8_t)SYM_HOME_N; i++) {
			lcd_pair_set(&sym_home_pairs[i], on);
		}
		return;
	}
	if(sym_cell[id] == 0xFF) {          /* no such glyph on this glass          */
		return;
	}
	if(on) {
		lcd_buf[sym_cell[id]] |= sym_mask[id];
	} else {
		lcd_buf[sym_cell[id]] &= (uint8_t)~sym_mask[id];
	}
#else
	(void)id;
	(void)on;
#endif
}

void Lcd_DigitBig(uint8_t pos, uint8_t seg7) {
	if(pos > 1) {
		return;
	}
	lcd_draw7(&big_pairs[pos], seg7);
}

void Lcd_DigitSmall(uint8_t seg7) {
	lcd_draw7(&small_pairs, seg7);
}

void Lcd_ClockDigit(uint8_t pos, uint8_t seg7) {
	if(pos < 1 || pos > 4) {
		return;
	}
	lcd_draw7(&clk_pairs[pos - 1], seg7);
}

void Lcd_Colon(uint8_t on) {
	lcd_pair_set(&colon_pair, on);
}

void Lcd_Weekday(uint8_t week) {
	uint8_t i;

	for(i = 0; i < 7; i++) {
		lcd_pair_set(&week_pairs[i], (uint8_t)(week == (uint8_t)(i + 1)));
	}
}

/*******************************************************************************/
/* r02 index-style helpers: idx 3/2 = big tens/units, 4..7 = clock H10..M1.   */
void Lcd_DigitIdx(uint8_t idx, uint8_t digit) {
	uint8_t seg7 = 0x00;

	if(digit <= 9) {
		seg7 = lcd_seg7[digit];
	}
	switch(idx) {
	case LCD_DIG_T: Lcd_DigitBig(0, seg7);  break;
	case LCD_DIG_U: Lcd_DigitBig(1, seg7);  break;
	case 4:         Lcd_ClockDigit(1, seg7); break;
	case 5:         Lcd_ClockDigit(2, seg7); break;
	case 6:         Lcd_ClockDigit(3, seg7); break;
	case 7:         Lcd_ClockDigit(4, seg7); break;
	default: break;
	}
}

void Lcd_CharIdx(uint8_t idx, uint8_t seg7) {
	seg7 &= 0x7F;
	switch(idx) {
	case LCD_DIG_T: Lcd_DigitBig(0, seg7);  break;
	case LCD_DIG_U: Lcd_DigitBig(1, seg7);  break;
	case 4:         Lcd_ClockDigit(1, seg7); break;
	case 5:         Lcd_ClockDigit(2, seg7); break;
	case 6:         Lcd_ClockDigit(3, seg7); break;
	case 7:         Lcd_ClockDigit(4, seg7); break;
	default: break;
	}
}

void Lcd_Letters(uint8_t tens_seg7, uint8_t units_seg7) {
	Lcd_CharIdx(LCD_DIG_T, tens_seg7);
	Lcd_CharIdx(LCD_DIG_U, units_seg7);
}

/* IN / OU / AL in the big digits (shared table, one definition).              */
void Lcd_SensorLetters(uint8_t src) {
	static const uint8_t code letters[3][2] = {
		{ CH_I, CH_N }, { CH_O, CH_U }, { CH_A, CH_L }
	};

	Lcd_Letters(letters[src][0], letters[src][1]);
}

void Lcd_HalfDot(uint8_t on) {
	Lcd_SymSet(SYM_DP5, on);
}

void Lcd_Clock(uint8_t hour, uint8_t min, uint8_t colon_on, uint8_t h24) {
	uint8_t disp_h;
	uint8_t h_t;
	uint8_t h_o;
	uint8_t m_t;
	uint8_t m_o;
	uint8_t am;
	uint8_t pm;

	am = 0;
	pm = 0;
	if(h24) {
		disp_h = hour;
	}
	else {
		if(hour < 12) {
			am = 1;
		}
		else {
			pm = 1;
		}
		disp_h = (uint8_t)(hour % 12);
		if(disp_h == 0) {
			disp_h = 12;
		}
	}
	h_t = (uint8_t)(disp_h / 10);
	h_o = (uint8_t)(disp_h % 10);
	m_t = (uint8_t)(min / 10);
	m_o = (uint8_t)(min % 10);

	if(h_t) {
		Lcd_ClockDigit(1, lcd_seg7[h_t]);
	}
	else {
		Lcd_ClockDigit(1, 0);            /* blank leading zero                   */
	}
	Lcd_ClockDigit(2, lcd_seg7[h_o]);
	Lcd_ClockDigit(3, lcd_seg7[m_t]);
	Lcd_ClockDigit(4, lcd_seg7[m_o]);

	Lcd_Colon(colon_on);

	Lcd_SymSet(SYM_AM, am);
	Lcd_SymSet(SYM_PM, pm);
}

void Lcd_Clock_Blank(uint8_t field) {
	if(field == 0) {
		Lcd_ClockDigit(1, 0);
		Lcd_ClockDigit(2, 0);
	}
	else {
		Lcd_ClockDigit(3, 0);
		Lcd_ClockDigit(4, 0);
	}
}

/* menu=0 also lights the °C symbol and flushes; menu=1 leaves the unit and    */
/* the flush to the caller (the network windows light °C themselves, the local  */
/* menu draws a bare number). The glass has a full small 7-seg digit after the  */
/* decimal point (cells 8/9) - richer than the r02 half-dot: the tenths of the  */
/* absolute value are shown whenever present, while the value is in range.      */
static void lcd_draw_value(int16_t value_c100, uint8_t menu) {
	XDATA_TMP(led_value_parts_t, parts);
	uint16_t av;

	Led_FormatC100(value_c100, &parts);
	Lcd_DigitSmall(0);                 /* tenths blank unless in range          */
	if(parts.range) {
		/* outside the displayable range: say so, never show a wrong number     */
		if(parts.range == LED_RANGE_UP) Lcd_Letters(CH_U, CH_P);
		else                            Lcd_Letters(CH_L, CH_O);
		Lcd_HalfDot(0);
	}
	else {
		if(parts.negative) {
			Lcd_CharIdx(LCD_DIG_T, CH_MINUS);
		}
		else if(parts.tens) {
			Lcd_DigitIdx(LCD_DIG_T, parts.tens);
		}
		else {
			Lcd_DigitIdx(LCD_DIG_T, 10); /* blank the tens cell                  */
		}
		Lcd_DigitIdx(LCD_DIG_U, parts.units);
		av = (uint16_t)(value_c100 < 0 ? -value_c100 : value_c100);
		av = (av % 100) / 10;            /* tenths                                */
		/* The fraction follows the value, not the caller: a dot + the small      */
		/* tenths digit whenever tenths are present - menu tenths values          */
		/* (calibration, hysteresis, min/max limits) show them like the           */
		/* temperature does, an x.0 value shows just the big digits, and a        */
		/* whole-percent value (humidity, zero tail) never shows a fraction.      */
		Lcd_HalfDot(av ? 1 : 0);
		if(av) {
			/* Lcd_DigitSmall takes a 7-seg PATTERN, not a digit value           */
			Lcd_DigitSmall(lcd_seg7[av]);
		}
	}
	if(!menu) {
		Lcd_SymSet(SYM_DEGC, 1);        /* MenuValue callers light it themselves */
	}
}

void Lcd_Temp(int value_c100) {
	lcd_draw_value((int16_t)value_c100, 0);
	Lcd_Flush();
}

void Lcd_MenuValue(int16_t value_c100) {
	lcd_draw_value(value_c100, 1);
}

void Lcd_Temp_Blank(void) {
	Lcd_DigitBig(0, 0);
	Lcd_DigitBig(1, 0);
}

/* Draw "Er" in the big temperature digits (sensor error / no reading).        */
void Lcd_TempErr(void) {
	Lcd_Letters(CH_E, CH_R);
	Lcd_SymSet(SYM_DEGC, 1);             /* the unit must show even on a cold   */
	                                     /* buffer, not only survive a warm one */
	Lcd_HalfDot(0);                      /* no stale dot                        */
	Lcd_DigitSmall(0);                   /* no stale tenths digit              */
	Lcd_Flush();
}
