
#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"
#include "../MCAL/SYSTICK/SYSTICK_int.h"

u8 SevenSegment[] =
{
    0b00111111, //0
    0b00000110, //1
    0b01011011, //2
    0b01001111, //3
    0b01100110, //4
    0b01101101, //5
    0b01111101, //6
    0b00000111, //7
    0b01111111, //8
    0b01101111  //9
};

int main(void)
{
    u8 i;
    u8 number;
    u8 segment;
    u8 bit_value;

    MRCC_vInit();
    MRCC_vEnableClk(RCC_APB2, RCC_GPIOA);

    GPIOx_PinConfig_t Seven_Segment_Pins[7];

    for(i = 0; i < 7; i++)
    {
        Seven_Segment_Pins[i].Port = GPIO_PORTA;
        Seven_Segment_Pins[i].Pin = i + 1;
        Seven_Segment_Pins[i].Mode = GPIO_MODE_OUTPUT;
        Seven_Segment_Pins[i].OutputType = GPIO_OT_PUSHPULL;
        Seven_Segment_Pins[i].Speed = GPIO_SPEED_50MHZ;
        Seven_Segment_Pins[i].PullType = GPIO_NO_PULL;

        MGPIO_vPinInit(&Seven_Segment_Pins[i]);
    }

    MSYSTIC_Config_t STK_cfg = {
        .InterruptEnable = INT_DISABLE,
        .CLK_SRC = CLK_SRC_AHB_8
    };
    MSYSTICK_vInit(&STK_cfg);

    while(1)
    {
        for(number = 0; number <= 9; number++)
        {
            for(segment = 0; segment < 7; segment++)
            {
                bit_value = (SevenSegment[number] >> segment) & 0x01;

                MGPIO_vSetPinValue(Seven_Segment_Pins[segment].Port, Seven_Segment_Pins[segment].Pin, !bit_value);
            }

            MSYSTICK_vSetDelay_ms(2000);
        }
    }

    return 0;
}
