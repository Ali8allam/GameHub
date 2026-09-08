#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/GPIO/GPIO_int.h"
#include "../MCAL/DMA/DMA_int.h"




arr1[] = {1,2,3,4,5,6,7,8,9,10};
arr2[10] = {0};

void turnOnLed(void)
{
	MGPIO_vSetPinValueAtomic(GPIO_PORTA, GPIO_PIN0,1);
}

int main(void)
{

    MRCC_vInit();
    MRCC_vEnableCLK(RCC_AHB1, GPIO_PORTA);
    MRCC_vEnableCLK(RCC_AHB1, 22);


    GPIOx_PinConfig_t led =
    {
        .Port       = GPIO_PORTA,
        .Pin        = GPIO_PIN0,
        .Mode       = GPIO_MODE_OUTPUT,
        .OutputType = GPIO_OT_PUSHPULL,
        .Speed      = GPIO_SPEED_LOW,
        .PullType   = GPIO_NO_PULL
    }; MGPIO_vPinInit(&led);

    MNVIC_vEnable_Peripheral_INT(56);

    MDMA2_vCallBack(0,turnOnLed);

    MDMA2_vInit(0);

    MDMA2_vSetStreamCfg(0,arr1,arr2,2,2,10,3);

    MDMA2_vEnableStream(0);




    while(1)
    {


    }

    return 0;
}
