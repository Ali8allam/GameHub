/*
 * TFT_int.h
 *
 *  Created on: Sep 3, 2026
 *      Author: ALI & ADHM
 */

#ifndef HAL_TFT_TFT_INT_H_
#define HAL_TFT_TFT_INT_H_

void HTFT_vInit(void);
void HTFT_vShowImage(const u16 A_u16ImgArr[] , u16 A_u16ImgSize);
void HTFT_vSetXPos(u16 A_u16xStart , u16 A_u16xEnd);
void HTFT_vSetYPos(u16 A_u16yStart , u16 A_u16yEnd);
void HTFT_FillBackground(u16 A_u16Color);
void HTFT_FillRectangle(u16 A_u16Color);
void HTFT_vDrawChar(u8 A_u8X, u8 A_u8Y, char A_chChar, u16 A_u16Color, u16 A_u16BgColor);
void HTFT_vPrintString(u8 A_u8X, u8 A_u8Y, const char *A_pcString, u16 A_u16Color, u16 A_u16BgColor);


#endif /* HAL_TFT_TFT_INT_H_ */
