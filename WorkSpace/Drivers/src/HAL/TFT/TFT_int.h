/*
 * TFT_int.h
 *
 *  Created on: Sep 3, 2026
 *      Author: bigor
 */

#ifndef HAL_TFT_TFT_INT_H_
#define HAL_TFT_TFT_INT_H_

void HTFT_vInit(void);

void HTFT_vShowImage(const u16 A_u16ImgArr[], u16 A_u16ImgSize);

void HTFT_vSetXPos(u16 A_u16xStart, u16 A_u16xEnd);
void HTFT_vSetYPos(u16 A_u16yStart, u16 A_u16yEnd);
void HTFT_vFillBackgroundColor(u16 A_u16Color);
void HTFT_vFillRectangle(u16 A_u16Color);
void HTFT_vDrawPixel(u16 A_u16X, u16 A_u16Y, u16 A_u16Color);
void HTFT_vDrawRect(u16 A_u16X, u16 A_u16Y, u16 A_u16Width, u16 A_u16Height, u16 A_u16Color);
void HTFT_vDrawChar(u16 A_u16X, u16 A_u16Y, char A_char, u16 A_u16Color, u16 A_u16BgColor);
void HTFT_vDrawString(u16 A_u16X, u16 A_u16Y, const char* A_str, u16 A_u16Color, u16 A_u16BgColor);


#endif /* HAL_TFT_TFT_INT_H_ */
