#ifndef HAL_ST7735_INT_H
#define HAL_ST7735_INT_H

#include "../../LIB/STD_TYPES.h"

#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160

void HST7735_vInit(void);
void HST7735_vFillScreen(u16 color);
void HST7735_vFillRect(u8 x, u8 y, u8 width, u8 height, u16 color);
void HST7735_vDrawChar(u8 x, u8 y, char ch, u16 color, u16 bg_color);
void HST7735_vDrawString(u8 x, u8 y, const char *str, u16 color, u16 bg_color);
void HST7735_vDrawIcon7x7(u8 x, u8 y, const u8 *icon, u16 color, u16 bg_color);

#endif