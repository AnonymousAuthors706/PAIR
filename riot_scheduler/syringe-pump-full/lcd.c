/**
 * Muhammad Wasif Kamran

 *
 * RIOT Example: LiquidCrystal (HD44780) Simulation
 *
 * Ports the Arduino LiquidCrystal library to C for MSP430/RIOT.
 * No physical LCD is present; the HD44780 parallel bus protocol is
 * simulated via MSP430 GPIO registers observable in Vivado waveforms:
 *
 *   P5OUT[0] = RS  (Register Select: 0=command, 1=data)
 *   P5OUT[1] = EN  (Enable strobe, toggled per byte/nibble)
 *   P5OUT[2] = RW  (Read/Write: always 0=write)
 *   P4OUT    = D[7:0] data bus (8-bit mode)
 *
 * Expected behaviour (observable in Vivado):
 *   P4OUT : 0x50 (start marker), then HD44780 8-bit init sequence
 *           commands, then ASCII character writes for "Hello, World!"
 *           and "RIOT on MSP430", ending with 0xFF (done marker)
 *   P5OUT : RS/EN bit-toggling faithful to HD44780 protocol
 *
 * Approx. sim time: 25ms
 */

#include <inttypes.h>
#include <stddef.h>
#include "lcd.h"
#include "hardware.h"
#include "thread.h"


/* ------------------------------------------------------------------ */
/* GPIO helpers — replace Arduino pinMode()/digitalWrite()            */
/* ------------------------------------------------------------------ */

static void lcd_gpio_init(void)
{
    P5DIR |= LCD_RS_BIT | LCD_EN_BIT | LCD_RW_BIT;
    P4DIR  = 0xFF;
    P5OUT &= ~(LCD_RS_BIT | LCD_EN_BIT | LCD_RW_BIT);
    P4OUT  = 0x00;
}

static inline void lcd_set_rs(uint8_t val)
{
    if (val) P5OUT |=  LCD_RS_BIT;
    else     P5OUT &= ~LCD_RS_BIT;
}

static inline void lcd_set_en(uint8_t val)
{
    if (val) P5OUT |=  LCD_EN_BIT;
    else     P5OUT &= ~LCD_EN_BIT;
}

/* ------------------------------------------------------------------ */
/* HD44780 low-level bus primitives                                   */
/* ------------------------------------------------------------------ */

static void lcd_pulse_enable(void)
{
    lcd_set_en(0);
    __delay_cycles(5);
    lcd_set_en(1);
    __delay_cycles(5);    /* EN high must be > 450 ns */
    lcd_set_en(0);
    //__delay_cycles(250);  /* commands need > 37 µs to settle */
	thread_yield();
}

static void lcd_write8bits(uint8_t value)
{
    P4OUT = value;
    lcd_pulse_enable();
}

static void lcd_write4bits(uint8_t value)
{
    /* lower nibble of value placed in lower 4 bits of P4OUT */
    P4OUT = (P4OUT & 0xF0u) | (value & 0x0Fu);
    lcd_pulse_enable();
}

/* send() from LiquidCrystal.cpp: mode=0 → command, mode=1 → data */
static void lcd_send(lcd_t *lcd, uint8_t value, uint8_t mode)
{
    lcd_set_rs(mode);
    P5OUT &= ~LCD_RW_BIT;  /* always write */

    if (lcd->_displayfunction & LCD_8BITMODE) {
        lcd_write8bits(value);
    } else {
        lcd_write4bits(value >> 4);
        lcd_write4bits(value);
    }
}

static void lcd_command(lcd_t *lcd, uint8_t value)
{
    lcd_send(lcd, value, 0);
}

static void lcd_write_char(lcd_t *lcd, uint8_t value)
{
    lcd_send(lcd, value, 1);
}

/* ------------------------------------------------------------------ */
/* High-level LCD API — mirrors every public method of LiquidCrystal  */
/* ------------------------------------------------------------------ */

static void lcd_set_row_offsets(lcd_t *lcd,
                                uint8_t r0, uint8_t r1,
                                uint8_t r2, uint8_t r3)
{
    lcd->_row_offsets[0] = r0;
    lcd->_row_offsets[1] = r1;
    lcd->_row_offsets[2] = r2;
    lcd->_row_offsets[3] = r3;
}

/* Equivalent to LiquidCrystal::begin() */
static void lcd_begin(lcd_t *lcd, uint8_t cols, uint8_t lines, uint8_t dotsize)
{
    if (lines > 1)
        lcd->_displayfunction |= LCD_2LINE;
    lcd->_numlines = lines;

    lcd_set_row_offsets(lcd,
        0x00, 0x40, (uint8_t)(0x00 + cols), (uint8_t)(0x40 + cols));

    if ((dotsize != LCD_5x8DOTS) && (lines == 1))
        lcd->_displayfunction |= LCD_5x10DOTS;

    lcd_gpio_init();

    /* HD44780 spec: wait > 40 ms after VDD rises (scaled for simulation) */
    //__delay_cycles(500);
	thread_yield();

    lcd_set_rs(0);
    lcd_set_en(0);

    if (!(lcd->_displayfunction & LCD_8BITMODE)) {
        /* 4-bit init sequence — HD44780 datasheet figure 24, pg 46 */
        lcd_write4bits(0x03);
        __delay_cycles(100);
        lcd_write4bits(0x03);
        __delay_cycles(100);
        lcd_write4bits(0x03);
        __delay_cycles(20);
        lcd_write4bits(0x02);   /* set 4-bit interface */
    } else {
        /* 8-bit init sequence — HD44780 datasheet figure 23, pg 45 */
        lcd_command(lcd, LCD_FUNCTIONSET | lcd->_displayfunction);
        __delay_cycles(100);
        lcd_command(lcd, LCD_FUNCTIONSET | lcd->_displayfunction);
        __delay_cycles(20);
        lcd_command(lcd, LCD_FUNCTIONSET | lcd->_displayfunction);
    }

    /* Final function set: lines, font */
    lcd_command(lcd, LCD_FUNCTIONSET | lcd->_displayfunction);

    /* Display on, cursor off, blink off */
    lcd->_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->_displaycontrol);

    /* Clear display */
    lcd_command(lcd, LCD_CLEARDISPLAY);
    __delay_cycles(50);

    /* Entry mode: left-to-right, no display shift */
    lcd->_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->_displaymode);
}

static void lcd_clear(lcd_t *lcd)
{
    lcd_command(lcd, LCD_CLEARDISPLAY);
    __delay_cycles(50);
}

static void lcd_home(lcd_t *lcd)
{
    lcd_command(lcd, LCD_RETURNHOME);
    __delay_cycles(50);
}

static void lcd_set_cursor(lcd_t *lcd, uint8_t col, uint8_t row)
{
    const uint8_t max_lines =
        (uint8_t)(sizeof(lcd->_row_offsets) / sizeof(lcd->_row_offsets[0]));
    if (row >= max_lines)
        row = max_lines - 1;
    if (row >= lcd->_numlines)
        row = lcd->_numlines - 1;
    lcd_command(lcd, LCD_SETDDRAMADDR | (col + lcd->_row_offsets[row]));
}

static void lcd_no_display(lcd_t *lcd)
{
    lcd->_displaycontrol &= ~LCD_DISPLAYON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->_displaycontrol);
}

static void lcd_display(lcd_t *lcd)
{
    lcd->_displaycontrol |= LCD_DISPLAYON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->_displaycontrol);
}

static void lcd_no_cursor(lcd_t *lcd)
{
    lcd->_displaycontrol &= ~LCD_CURSORON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->_displaycontrol);
}

static void lcd_cursor(lcd_t *lcd)
{
    lcd->_displaycontrol |= LCD_CURSORON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->_displaycontrol);
}

static void lcd_no_blink(lcd_t *lcd)
{
    lcd->_displaycontrol &= ~LCD_BLINKON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->_displaycontrol);
}

static void lcd_blink(lcd_t *lcd)
{
    lcd->_displaycontrol |= LCD_BLINKON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->_displaycontrol);
}

static void lcd_scroll_left(lcd_t *lcd)
{
    lcd_command(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

static void lcd_scroll_right(lcd_t *lcd)
{
    lcd_command(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

static void lcd_left_to_right(lcd_t *lcd)
{
    lcd->_displaymode |= LCD_ENTRYLEFT;
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->_displaymode);
}

static void lcd_right_to_left(lcd_t *lcd)
{
    lcd->_displaymode &= ~LCD_ENTRYLEFT;
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->_displaymode);
}

static void lcd_autoscroll(lcd_t *lcd)
{
    lcd->_displaymode |= LCD_ENTRYSHIFTINCREMENT;
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->_displaymode);
}

static void lcd_no_autoscroll(lcd_t *lcd)
{
    lcd->_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->_displaymode);
}

static void lcd_create_char(lcd_t *lcd, uint8_t location, uint8_t charmap[8])
{
    location &= 0x07u;  /* only 8 CGRAM slots (0-7) */
    lcd_command(lcd, LCD_SETCGRAMADDR | (location << 3));
    uint8_t i;
    for (i = 0; i < 8; i++)
        lcd_write_char(lcd, charmap[i]);
}

static void lcd_print(lcd_t *lcd, const char *str)
{
    while (*str){
        lcd_write_char(lcd, (uint8_t)*str++);		
	}
}

static void lcd_print_raw_int(lcd_t *lcd, int value)
{
    uint8_t *bytes = (uint8_t *)&value;
    int i;
    for (i = 1; i >= 0; i--) {
        lcd_write_char(lcd, bytes[i]);
		thread_yield();
    }
}

void run_lcd(int latest_temp, int syringe_status){
	static lcd_t g_lcd;
	g_lcd._displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
	lcd_begin(&g_lcd, 16, 2, LCD_5x8DOTS);   /* sets up GPIO + HD44780 init */
	lcd_set_cursor(&g_lcd, 0, 0);            /* position: col 0, row 0 */
	
	lcd_print_raw_int(&g_lcd, latest_temp);
	lcd_write_char(&g_lcd, ' ');
	lcd_print_raw_int(&g_lcd, syringe_status);
	thread_zombify();
}