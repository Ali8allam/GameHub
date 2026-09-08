
#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"


#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/SYSTICK/SYSTICK_int.h"
#include "../../MCAL/SPI/SPI_int.h"

#include "TFT_int.h"

GPIOx_PinConfig_t TFT_RST_PIN = {
        .Port       = GPIO_PORTA,
        .Pin        = GPIO_PIN0,
        .Mode       = GPIO_MODE_OUTPUT,
        .Speed      = GPIO_SPEED_LOW,
        .OutputType = GPIO_OT_PUSHPULL,
        .PullType   = GPIO_NO_PULL
};


GPIOx_PinConfig_t TFT_A0_PIN = {
        .Port       = GPIO_PORTA,
        .Pin        = GPIO_PIN1,
        .Mode       = GPIO_MODE_OUTPUT,
        .Speed      = GPIO_SPEED_LOW,
        .OutputType = GPIO_OT_PUSHPULL,
        .PullType   = GPIO_NO_PULL
};


static void Reset_Seq(void);
static void Write_CMD(u8 A_u8CMD);
static void Write_Data(u8 A_u8Data);

static void Reset_Seq(void)
{
	MGPIO_vSetPinValue(TFT_RST_PIN.Port,TFT_RST_PIN.Pin,GPIO_HIGH);

	MSYSTICK_vSetDelay_ms(100);

	MGPIO_vSetPinValue(TFT_RST_PIN.Port,TFT_RST_PIN.Pin,GPIO_LOW);

	MSYSTICK_vSetDelay_us(1);

	MGPIO_vSetPinValue(TFT_RST_PIN.Port,TFT_RST_PIN.Pin,GPIO_HIGH);

	MSYSTICK_vSetDelay_us(100);

	MGPIO_vSetPinValue(TFT_RST_PIN.Port,TFT_RST_PIN.Pin,GPIO_LOW);

	MSYSTICK_vSetDelay_us(100);

	MGPIO_vSetPinValue(TFT_RST_PIN.Port,TFT_RST_PIN.Pin,GPIO_HIGH);

	MSYSTICK_vSetDelay_ms(120);
}

static void Write_CMD(u8 A_u8CMD)
{
	MGPIO_vSetPinValue(TFT_A0_PIN.Port,TFT_A0_PIN.Pin,GPIO_LOW);
	MSPI_vTransceive(A_u8CMD);


}


static void Write_Data(u8 A_u8Data)
{
	MGPIO_vSetPinValue(TFT_A0_PIN.Port,TFT_A0_PIN.Pin,GPIO_HIGH);
	MSPI_vTransceive(A_u8Data);
}

void HTFT_vInit(void)
{
	MGPIO_vPinInit(&TFT_RST_PIN);
	MGPIO_vPinInit(&TFT_A0_PIN);

    MSYSTICK_Config_t STK_cfg = {
        .InterruptEnable = INT_DISABLE,
        .CLK_SRC = CLK_SRC_AHB_8
    };
    MSYSTICK_vInit(&STK_cfg);

    MSPI_vInit();

    Reset_Seq();

    Write_CMD(0x11);

    MSYSTICK_vSetDelay_ms(15);

    Write_CMD(0x3A);

    Write_Data(0x05);

    Write_CMD(0x29);



}


void HTFT_vShowImage(const u16 A_u16ImgArr[], u16 A_u16ImgSize)
{
    u16 Local_u16Counter = 0;


    Write_CMD(0x2A);
    Write_Data(0);
    Write_Data(0);
    Write_Data(0x00);
    Write_Data(0x7F);


    Write_CMD(0x2B);
    Write_Data(0);
    Write_Data(0);
    Write_Data(0x00);
    Write_Data(0x9F);




    Write_CMD(0x2C);


    for (Local_u16Counter = 0; Local_u16Counter < A_u16ImgSize; Local_u16Counter++)
    {
        Write_Data((u8)(A_u16ImgArr[Local_u16Counter] >> 8));
        Write_Data((u8)A_u16ImgArr[Local_u16Counter]);
    }
}

void HTFT_vSetXPos(u16 A_u16xStart, u16 A_u16xEnd)
{

    Write_CMD(0x2A);
    Write_Data((u8)(A_u16xStart >> 8));
    Write_Data((u8)A_u16xStart);
    Write_Data((u8)(A_u16xEnd >> 8));
    Write_Data((u8)A_u16xEnd);
}

void HTFT_vSetYPos(u16 A_u16yStart, u16 A_u16yEnd)
{

    Write_CMD(0x2B);
    Write_Data((u8)(A_u16yStart >> 8));
    Write_Data((u8)A_u16yStart);
    Write_Data((u8)(A_u16yEnd >> 8));
    Write_Data((u8)A_u16yEnd);
}

void HTFT_vFillBackgroundColor(u16 A_u16Color)
{
    u32 Local_u32TotalPixels = 128 * 160;
    u32 Local_u32Counter = 0;


    HTFT_vSetXPos(0, 127);
    HTFT_vSetYPos(0, 159);


    Write_CMD(0x2C);


    for (Local_u32Counter = 0; Local_u32Counter < Local_u32TotalPixels; Local_u32Counter++)
    {
        Write_Data((u8)(A_u16Color >> 8));
        Write_Data((u8)A_u16Color);
    }
}

/* Standard 5x7 ASCII Font Array */
static const u8 Font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

void HTFT_vDrawPixel(u16 A_u16X, u16 A_u16Y, u16 A_u16Color)
{
    if (A_u16X >= 128 || A_u16Y >= 160) return;
    HTFT_vSetXPos(A_u16X, A_u16X);
    HTFT_vSetYPos(A_u16Y, A_u16Y);
    Write_CMD(0x2C);
    Write_Data((u8)(A_u16Color >> 8));
    Write_Data((u8)A_u16Color);
}

void HTFT_vDrawRect(u16 A_u16X, u16 A_u16Y, u16 A_u16Width, u16 A_u16Height, u16 A_u16Color)
{
    HTFT_vSetXPos(A_u16X, A_u16X + A_u16Width - 1);
    HTFT_vSetYPos(A_u16Y, A_u16Y + A_u16Height - 1);
    Write_CMD(0x2C);
    u32 Local_u32TotalPixels = (u32)A_u16Width * A_u16Height;
    for (u32 i = 0; i < Local_u32TotalPixels; i++)
    {
        Write_Data((u8)(A_u16Color >> 8));
        Write_Data((u8)A_u16Color);
    }
}

void HTFT_vDrawChar(u16 A_u16X, u16 A_u16Y, char A_char, u16 A_u16Color, u16 A_u16BgColor)
{
    if (A_char < ' ' || A_char > 'Z') return;
    u8 fontIdx = A_char - ' ';

    for (u8 col = 0; col < 5; col++)
    {
        u8 line = Font5x7[fontIdx][col];
        for (u8 row = 0; row < 7; row++)
        {
            if (line & 0x01)
            {
                HTFT_vDrawPixel(A_u16X + col, A_u16Y + row, A_u16Color);
            }
            else
            {
                HTFT_vDrawPixel(A_u16X + col, A_u16Y + row, A_u16BgColor);
            }
            line >>= 1;
        }
    }
}

void HTFT_vDrawString(u16 A_u16X, u16 A_u16Y, const char* A_str, u16 A_u16Color, u16 A_u16BgColor)
{
    while (*A_str)
    {
        HTFT_vDrawChar(A_u16X, A_u16Y, *A_str, A_u16Color, A_u16BgColor);
        A_u16X += 6;
        A_str++;
    }
}