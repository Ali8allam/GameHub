/*
 * TFT_ptg.c
 *
 *  Created on: Sep 3, 2026
 *      Author: ALI & ADHM
 */

#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/SPI/SPI_int.h"
#include "../../MCAL/SYSTICK/SYSTICK_int.h"

#include"TFT_int.h"

static u16 CurrentXStart ;
static u16 CurrentYStart ;

static u16 CurrentXEnd ;
static u16 CurrentYEnd ;

static const u8 Font5x7[][5] = {
	{0x00, 0x00, 0x00, 0x00, 0x00},
	{0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
	{0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
	{0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
	{0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
	{0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
	{0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
	{0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
	{0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
	{0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
	{0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
	{0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
	{0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
	{0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
	{0x3E, 0x45, 0x49, 0x51, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
	{0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
	{0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
	{0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
	{0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}
};


GPIOx_PinConfig_t TFT_RST_PIN={
		.Port = GPIO_PORTA,
		.Pin = GPIO_PIN0,
		.Mode = GPIO_MODE_OUTPUT,
		.Speed = GPIO_SPEED_HIGH,
		.OutputType = GPIO_OT_PUSHPULL
};
GPIOx_PinConfig_t TFT_A0_PIN={
		.Port = GPIO_PORTA,
		.Pin = GPIO_PIN1,
		.Mode = GPIO_MODE_OUTPUT,
		.Speed = GPIO_SPEED_HIGH,
		.OutputType = GPIO_OT_PUSHPULL
};

static void Reset_Seq(void)
{
	// RST PIN = 1
	MGPIO_vSetPinValue(TFT_RST_PIN.Port, TFT_RST_PIN.Pin, GPIO_HIGH);
	//DELAY 100usec
	MSYSTICK_vSetDelay_us(100);
	//RST PIN 0
	MGPIO_vSetPinValue(TFT_RST_PIN.Port, TFT_RST_PIN.Pin, GPIO_LOW);
	// delay 1usec
	MSYSTICK_vSetDelay_us(1);
	// Pin high
	MGPIO_vSetPinValue(TFT_RST_PIN.Port, TFT_RST_PIN.Pin, GPIO_HIGH);
	// delay 100usec
	MSYSTICK_vSetDelay_us(100);
	// pin low
	MGPIO_vSetPinValue(TFT_RST_PIN.Port, TFT_RST_PIN.Pin, GPIO_LOW);
	// delay 100usec
	MSYSTICK_vSetDelay_us(100);
	// pin high
	MGPIO_vSetPinValue(TFT_RST_PIN.Port, TFT_RST_PIN.Pin, GPIO_HIGH);
	// delay 120 msec
	MSYSTICK_vSetDelay_ms(120);
}
static void Write_cmd(u8 A_u8cmd)
{
	MGPIO_vSetPinValue(TFT_A0_PIN.Port , TFT_A0_PIN.Pin,GPIO_LOW);
	MSPI_vTranscieve( A_u8cmd);
}
static void Write_data(u8 A_u8data)
{
	MGPIO_vSetPinValue(TFT_A0_PIN.Port , TFT_A0_PIN.Pin,GPIO_HIGH);
	MSPI_vTranscieve( A_u8data);
}

void HTFT_vInit(void)
{
	MGPIO_vPinInit(&TFT_RST_PIN);
	MGPIO_vPinInit(&TFT_A0_PIN);
	MGPIO_vSetPinValue(TFT_A0_PIN.Port, TFT_A0_PIN.Pin, GPIO_LOW);

	MSPI_vInit();
	MSYSTICK_Config_t STK_cfg={
			.InterruptEnable = INT_DISABLE,
			.CLK_SRC= CLK_SRC_AHB_8
	};
	MSYSTICK_vInit(&STK_cfg);
	//RST
	Reset_Seq();
	//SLEEP OUT
	Write_cmd(0x11);
	MSYSTICK_vSetDelay_ms(120);
	// Memory access control: RGB order, normal orientation.
	Write_cmd(0x36);
	Write_data(0x00);
	// Pixel format: RGB565.
	Write_cmd(0x3A);
	Write_data(0x05);
	MSYSTICK_vSetDelay_ms(10);
	Write_cmd(0x29);
	MSYSTICK_vSetDelay_ms(20);
}
void HTFT_vShowImage(const u16 A_u16ImgArr[] , u16 A_u16ImgSize)
{
	u8 MSB=0;
	u8 LSB=0;
	// set rows position
	Write_cmd(0x2A);

	// 1 send x start
	//msb 0x0000
	Write_data(0);
	//least 0x0000
	Write_data(0);

	// 2 send xEnd
	Write_data(0);
	Write_data(0x7F); //127

	Write_cmd(0x2B);
	// 1 send y start
	//msb 0x0000
	Write_data(0);
	//least 0x0000
	Write_data(0);

	// 2 send yEnd
	Write_data(0);
	Write_data(0x9F); //159

	// send image
	Write_cmd(0x2C);

	for(u16 i =0; i<A_u16ImgSize; i++)
	{
		MSB = (u8) (A_u16ImgArr[i] >>8 );
		LSB = A_u16ImgArr[i] & 0x00FF;

		Write_data( MSB);
		Write_data(LSB);
	}
}
void HTFT_vSetXPos(u16 A_u16xStart , u16 A_u16xEnd)
{
	CurrentXStart =A_u16xStart;
	CurrentXEnd =A_u16xEnd;


	// set rows position
		Write_cmd(0x2A);

		// 1 send x start
		//msb 0x0000
		Write_data(0);
		//least 0x0000
		Write_data(A_u16xStart);

		// 2 send xEnd
		Write_data(0);
		Write_data(A_u16xEnd); //127

}
void HTFT_vSetYPos(u16 A_u16yStart , u16 A_u16yEnd)
{
	CurrentYStart =A_u16yStart;
	CurrentYEnd =A_u16yEnd;
	Write_cmd(0x2B);
		// 1 send y start
		//msb 0x0000
		Write_data(0);
		//least 0x0000
		Write_data(A_u16yStart);

		// 2 send yEnd
		Write_data(0);
		Write_data(A_u16yEnd); //159
}
void HTFT_FillBackground(u16 A_u16Color)
{
	
	HTFT_vSetXPos(0 ,127);
	HTFT_vSetYPos(0 ,159);

	HTFT_FillRectangle(A_u16Color);
}

void HTFT_FillRectangle(u16 A_u16Color)
{
	u8 MSB =0;
	u8 LSB =0;
	u32 Pixels = (CurrentXEnd - CurrentXStart + 1) * (CurrentYEnd - CurrentYStart + 1);
	Write_cmd(0x2C);

	for(u32 i = 0; i < Pixels; i++)
	{
		MSB = (u8) ( A_u16Color >>8 );
		LSB =  A_u16Color & 0x00FF;

		Write_data( MSB);
		Write_data(LSB);
	}
}

void HTFT_vDrawChar(u8 x, u8 y, char ch, u16 color, u16 bg_color) {
	u8 font_idx = 0;

	if (ch >= 'A' && ch <= 'Z') font_idx = (u8)(ch - 'A' + 1);
	else if (ch >= '0' && ch <= '9') font_idx = (u8)(ch - '0' + 27);

    HTFT_vSetXPos(x, x + 4);
    HTFT_vSetYPos(y, y + 6);
	// send image
    Write_cmd(0x2C);

    for (u8 row = 0; row < 7; row++) {
        for (u8 col = 0; col < 5; col++) {
            u8 pixel = (Font5x7[font_idx][col] >> row) & 0x01;
            u16 pixel_color = pixel ? color : bg_color;
            Write_data((u8)(pixel_color >> 8));
            Write_data((u8)(pixel_color & 0xFF));
        }
    }
}

void HTFT_vPrintString(u8 x, u8 y, const char* str, u16 color, u16 bg_color) {
    while (*str) {
        HTFT_vDrawChar(x, y, *str, color, bg_color);
        x += 6; // Move 5 pixels + 1 space spacing
        if (x > 122) { x = 0; y += 8; } // Line wrap
        str++;
    }
}


