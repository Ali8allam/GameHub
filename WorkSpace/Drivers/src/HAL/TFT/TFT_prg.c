
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

