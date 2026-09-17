#include "../LIB/STD_TYPES.h"
#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"

int main(void)
{
    GPIOx_PinConfig_t led_config = {
        .Port = GPIO_PORTA,
        .Pin = GPIO_PIN0,
        .Mode = GPIO_MODE_OUTPUT,
        .OutputType = GPIO_OT_PUSHPULL,
        .Speed = GPIO_SPEED_2MHZ,
        .PullType = GPIO_NO_PULL,
        .Altfunc = 0
    };

    MRCC_vInit();
    MRCC_vEnableClk(RCC_APB2, RCC_GPIOA);
    MGPIO_vPinInit(&led_config);
    MGPIO_vSetPinValue(GPIO_PORTA, GPIO_PIN0, GPIO_HIGH);

    while (1)
    {
    }
}