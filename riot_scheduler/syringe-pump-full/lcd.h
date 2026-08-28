#ifndef LCD_H
#define LCD_H

/* ------------------------------------------------------------------ */
/* HD44780 command bytes (from LiquidCrystal.h)                       */
/* ------------------------------------------------------------------ */
#define LCD_CLEARDISPLAY    0x01
#define LCD_RETURNHOME      0x02
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT     0x10
#define LCD_FUNCTIONSET     0x20
#define LCD_SETCGRAMADDR    0x40
#define LCD_SETDDRAMADDR    0x80

/* Entry mode flags */
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

/* Display control flags */
#define LCD_DISPLAYON   0x04
#define LCD_DISPLAYOFF  0x00
#define LCD_CURSORON    0x02
#define LCD_CURSOROFF   0x00
#define LCD_BLINKON     0x01
#define LCD_BLINKOFF    0x00

/* Cursor/display shift flags */
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE  0x00
#define LCD_MOVERIGHT   0x04
#define LCD_MOVELEFT    0x00

/* Function set flags */
#define LCD_8BITMODE    0x10
#define LCD_4BITMODE    0x00
#define LCD_2LINE       0x08
#define LCD_1LINE       0x00
#define LCD_5x10DOTS    0x04
#define LCD_5x8DOTS     0x00

/* ------------------------------------------------------------------ */
/* MSP430 GPIO bit assignments for LCD control lines (P1OUT)          */
/* ------------------------------------------------------------------ */
#define LCD_RS_BIT  (1u << 0)  /* P1OUT[0] = Register Select */
#define LCD_EN_BIT  (1u << 1)  /* P1OUT[1] = Enable          */
#define LCD_RW_BIT  (1u << 2)  /* P1OUT[2] = Read/Write      */

/* ------------------------------------------------------------------ */
/* LCD state struct — C equivalent of the Arduino LiquidCrystal class */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t _displayfunction;
    uint8_t _displaycontrol;
    uint8_t _displaymode;
    uint8_t _numlines;
    uint8_t _row_offsets[4];
} lcd_t;

static void lcd_gpio_init(void);
static inline void lcd_set_rs(uint8_t val);
static inline void lcd_set_en(uint8_t val);
static void lcd_pulse_enable(void);
static void lcd_write8bits(uint8_t value);
static void lcd_write4bits(uint8_t value);
static void lcd_send(lcd_t *lcd, uint8_t value, uint8_t mode);
static void lcd_command(lcd_t *lcd, uint8_t value);
static void lcd_write_char(lcd_t *lcd, uint8_t value);
static void lcd_set_row_offsets(lcd_t *lcd, uint8_t r0, uint8_t r1, uint8_t r2, uint8_t r3);
static void lcd_begin(lcd_t *lcd, uint8_t cols, uint8_t lines, uint8_t dotsize);
static void lcd_clear(lcd_t *lcd);
static void lcd_home(lcd_t *lcd);
#endif // LCD_H
