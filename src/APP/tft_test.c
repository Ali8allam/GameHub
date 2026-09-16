#include "../LIB/STD_TYPES.h"
#include "../MCAL/RCC/RCC_int.h"
#include "../MCAL/SPI/SPI_int.h"
#include "../MCAL/SYSTICK/SYSTICK_int.h"
#include "../HAL/ST7735/ST7735_int.h"

int main(void)
{
    MSYSTIC_Config_t systick_config = {INT_DISABLE, CLK_SRC_AHB_8};

    MRCC_vInit();
    MRCC_vEnableClk(RCC_APB2, RCC_GPIOA);
    MRCC_vEnableClk(RCC_APB2, RCC_GPIOB);
    MRCC_vEnableClk(RCC_APB2, 12);
    MSYSTICK_vInit(&systick_config);
    MSPI_vInit();
    HST7735_vInit();
    HST7735_vFillScreen(0xF800);

    while (1) {
    }
}