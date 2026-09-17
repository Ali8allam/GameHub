#include "../../LIB/STD_TYPES.h"
#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/SPI/SPI_int.h"
#include "../../MCAL/SYSTICK/SYSTICK_int.h"
#include "ST7735_int.h"

#define TFT_CS_PORT GPIO_PORTA
#define TFT_CS_PIN GPIO_PIN4
#define TFT_DC_PORT GPIO_PORTA
#define TFT_DC_PIN GPIO_PIN8
#define TFT_RST_PORT GPIO_PORTB
#define TFT_RST_PIN GPIO_PIN1
#define TFT_BL_PORT GPIO_PORTB
#define TFT_BL_PIN GPIO_PIN10

#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT 0x11
#define ST7735_COLMOD 0x3A
#define ST7735_MADCTL 0x36
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_INVON 0x21
#define ST7735_NORON 0x13
#define ST7735_DISPON 0x29
#define ST7735_RAMWR 0x2C

static void tft_select(void)
{
    MGPIO_vSetPinValue(TFT_CS_PORT, TFT_CS_PIN, GPIO_LOW);
}

static void tft_deselect(void)
{
    MGPIO_vSetPinValue(TFT_CS_PORT, TFT_CS_PIN, GPIO_HIGH);
}

static void tft_command(u8 command)
{
    MGPIO_vSetPinValue(TFT_DC_PORT, TFT_DC_PIN, GPIO_LOW);
    tft_select();
    MSPI_u8Transceive(command);
    tft_deselect();
}

static void tft_data(u8 data)
{
    MGPIO_vSetPinValue(TFT_DC_PORT, TFT_DC_PIN, GPIO_HIGH);
    tft_select();
    MSPI_u8Transceive(data);
    tft_deselect();
}

static void tft_data_begin(void)
{
    MGPIO_vSetPinValue(TFT_DC_PORT, TFT_DC_PIN, GPIO_HIGH);
    tft_select();
}

static void tft_data_end(void)
{
    tft_deselect();
}

static void tft_command_with_data(u8 command, const u8 *data, u8 length)
{
    MGPIO_vSetPinValue(TFT_DC_PORT, TFT_DC_PIN, GPIO_LOW);
    tft_select();
    MSPI_u8Transceive(command);
    MGPIO_vSetPinValue(TFT_DC_PORT, TFT_DC_PIN, GPIO_HIGH);
    for (u8 byte = 0; byte < length; byte++) {
        MSPI_u8Transceive(data[byte]);
    }
    tft_deselect();
}

static void tft_set_window(u8 x, u8 y, u8 width, u8 height)
{
    u8 column_data[4];
    u8 row_data[4];
    u8 x_end = (u8)(x + width - 1);
    u8 y_end = (u8)(y + height - 1);

    column_data[0] = 0;
    column_data[1] = x;
    column_data[2] = 0;
    column_data[3] = x_end;
    row_data[0] = 0;
    row_data[1] = y;
    row_data[2] = 0;
    row_data[3] = y_end;

    tft_command_with_data(ST7735_CASET, column_data, 4);
    tft_command_with_data(ST7735_RASET, row_data, 4);

    tft_command(ST7735_RAMWR);
}

static void tft_write_color(u16 color)
{
    MSPI_u8Transceive((u8)(color >> 8));
    MSPI_u8Transceive((u8)color);
}

void HST7735_vInit(void)
{
    GPIOx_PinConfig_t spi_pins[] = {
        {GPIO_PORTA, GPIO_PIN5, GPIO_MODE_ALF, GPIO_OT_PUSHPULL, GPIO_SPEED_10MHZ, GPIO_NO_PULL, 0},
        {GPIO_PORTA, GPIO_PIN6, GPIO_MODE_INPUT, GPIO_OT_PUSHPULL, GPIO_SPEED_2MHZ, GPIO_NO_PULL, 0},
        {GPIO_PORTA, GPIO_PIN7, GPIO_MODE_ALF, GPIO_OT_PUSHPULL, GPIO_SPEED_10MHZ, GPIO_NO_PULL, 0}
    };
    GPIOx_PinConfig_t control_pins[] = {
        {TFT_CS_PORT, TFT_CS_PIN, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL, GPIO_SPEED_2MHZ, GPIO_NO_PULL, 0},
        {TFT_DC_PORT, TFT_DC_PIN, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL, GPIO_SPEED_2MHZ, GPIO_NO_PULL, 0},
        {TFT_RST_PORT, TFT_RST_PIN, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL, GPIO_SPEED_2MHZ, GPIO_NO_PULL, 0},
        {TFT_BL_PORT, TFT_BL_PIN, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL, GPIO_SPEED_2MHZ, GPIO_NO_PULL, 0}
    };

    for (u8 pin = 0; pin < 3; pin++) {
        MGPIO_vPinInit(&spi_pins[pin]);
    }
    for (u8 pin = 0; pin < 4; pin++) {
        MGPIO_vPinInit(&control_pins[pin]);
    }

    tft_deselect();
    MGPIO_vSetPinValue(TFT_RST_PORT, TFT_RST_PIN, GPIO_LOW);
    MSYSTICK_vSetDelay_ms(10);
    MGPIO_vSetPinValue(TFT_RST_PORT, TFT_RST_PIN, GPIO_HIGH);
    MSYSTICK_vSetDelay_ms(120);

    tft_command(ST7735_SWRESET);
    MSYSTICK_vSetDelay_ms(150);
    tft_command(ST7735_SLPOUT);
    MSYSTICK_vSetDelay_ms(120);
    tft_command(ST7735_COLMOD);
    tft_data(0x05);
    tft_command(ST7735_MADCTL);
    tft_data(0xC8);
    tft_command(ST7735_INVON);
    tft_command(ST7735_NORON);
    MSYSTICK_vSetDelay_ms(10);
    tft_command(ST7735_DISPON);
    MSYSTICK_vSetDelay_ms(100);
    MGPIO_vSetPinValue(TFT_BL_PORT, TFT_BL_PIN, GPIO_HIGH);
}

void HST7735_vFillRect(u8 x, u8 y, u8 width, u8 height, u16 color)
{
    u16 pixels = (u16)width * height;

    tft_set_window(x, y, width, height);
    tft_data_begin();
    while (pixels-- > 0) {
        tft_write_color(color);
    }
    tft_data_end();
}

void HST7735_vFillScreen(u16 color)
{
    HST7735_vFillRect(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

static const u8 font_5x7_digits[10][7] = {
    { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E }, /* 0 */
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }, /* 1 */
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F }, /* 2 */
    { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E }, /* 3 */
    { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 }, /* 4 */
    { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E }, /* 5 */
    { 0x0E, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x0E }, /* 6 */
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }, /* 7 */
    { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E }, /* 8 */
    { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C }  /* 9 */
};

static const u8 font_5x7_letters[26][7] = {
    { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }, /* A */
    { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E }, /* B */
    { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E }, /* C */
    { 0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C }, /* D */
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }, /* E */
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 }, /* F */
    { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F }, /* G */
    { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }, /* H */
    { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E }, /* I */
    { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C }, /* J */
    { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }, /* K */
    { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }, /* L */
    { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }, /* M */
    { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }, /* N */
    { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }, /* O */
    { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }, /* P */
    { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x09, 0x16 }, /* Q */
    { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }, /* R */
    { 0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E }, /* S */
    { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }, /* T */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }, /* U */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 }, /* V */
    { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A }, /* W */
    { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 }, /* X */
    { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 }, /* Y */
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F }  /* Z */
};

static const u8 font_colon[7] = { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 };
static const u8 font_excl[7]  = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 };
static const u8 font_blank[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const u8 *get_font_glyph(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return font_5x7_digits[ch - '0'];
    }
    if (ch >= 'A' && ch <= 'Z') {
        return font_5x7_letters[ch - 'A'];
    }
    if (ch >= 'a' && ch <= 'z') {
        return font_5x7_letters[ch - 'a'];
    }
    if (ch == ':') {
        return font_colon;
    }
    if (ch == '!') {
        return font_excl;
    }
    return font_blank;
}

void HST7735_vDrawChar(u8 x, u8 y, char ch, u16 color, u16 bg_color)
{
    const u8 *glyph;
    u8 row;
    u8 col;

    if (x + 6 > ST7735_WIDTH || y + 7 > ST7735_HEIGHT) {
        return;
    }

    glyph = get_font_glyph(ch);
    tft_set_window(x, y, 6, 7);
    tft_data_begin();
    for (row = 0; row < 7; row++) {
        u8 mask = glyph[row];
        for (col = 0; col < 5; col++) {
            if (mask & (0x10 >> col)) {
                tft_write_color(color);
            } else {
                tft_write_color(bg_color);
            }
        }
        tft_write_color(bg_color);
    }
    tft_data_end();
}

void HST7735_vDrawString(u8 x, u8 y, const char *str, u16 color, u16 bg_color)
{
    while (*str) {
        if (x + 6 > ST7735_WIDTH) {
            break;
        }
        HST7735_vDrawChar(x, y, *str, color, bg_color);
        x += 6;
        str++;
    }
}

void HST7735_vDrawIcon7x7(u8 x, u8 y, const u8 *icon, u16 color, u16 bg_color)
{
    u8 row;
    u8 col;

    if (x + 7 > ST7735_WIDTH || y + 7 > ST7735_HEIGHT) {
        return;
    }

    tft_set_window(x, y, 7, 7);
    tft_data_begin();
    for (row = 0; row < 7; row++) {
        u8 mask = icon[row];
        for (col = 0; col < 7; col++) {
            if (mask & (0x40 >> col)) {
                tft_write_color(color);
            } else {
                tft_write_color(bg_color);
            }
        }
    }
    tft_data_end();
}