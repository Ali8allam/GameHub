#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "../../MCAL/RCC/RCC_int.h"
#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/SYSTICK/SYSTICK_int.h"
#include "../../MCAL/EXTI/EXTI_int.h"
#include "../../MCAL/NVIC/NVIC_int.h"

#include "IR_cfg.h"
#include "IR_int.h"

static volatile u8  G_u8StartingFlag = 0;
static volatile u32 G_u32Arr[50]     = {0};
static volatile u8  G_u8Counter      = 0;
static volatile u8  G_u8DecodedValue = IR_KEY_NONE;
static volatile u8  G_u8NewKeyReady  = 0;

static void HIR_vGetPulseTime(void);
static void HIR_vDecodeBits(void);

void HIR_vInit(void)
{
    /* 1. Configure Pin PA0 as Input */
    GPIOx_PinConfig_t IR_Pin = {
        .Port = IR_PORT,
        .Pin  = IR_PIN,
        .Mode = GPIO_MODE_INPUT
    };
    MGPIO_vPinInit(&IR_Pin);

    /* 2. Setup EXTI Line 0 on Falling Edge */
    MEXTI_vInit();
    MEXTI_vEnableINT(IR_EXTI_LINE);
    MEXTI_vSetTrigger(IR_EXTI_LINE, EXTI_FALLING_EDGE);
    MEXTI_vSetCallBack(HIR_vGetPulseTime, IR_EXTI_LINE);

    /* 3. Enable Interrupt in NVIC */
    MNVIC_vEnable_Peripheral_INT(IR_NVIC_POS);

    /* 4. Configure SysTick Timer */
    MSYSTICK_Config_t STK_CFG = {
        .InterruptEnable = INT_ENABLE,
        .CLK_SRC         = CLK_SRC_AHB_8
    };
    MSYSTICK_vInit(&STK_CFG);
}

u8 HIR_u8GetKey(void)
{
    if (G_u8NewKeyReady)
    {
        G_u8NewKeyReady = 0;
        return G_u8DecodedValue;
    }
    return IR_KEY_NONE;
}

static void HIR_vGetPulseTime(void)
{
    if (G_u8StartingFlag == 0)
    {
        G_u8StartingFlag = 1;
        G_u8Counter = 0;
        MSYSTICK_vSetIntervalSingle(15, HIR_vDecodeBits);
    }
    else
    {
        if (G_u8Counter < 50)
        {
            G_u32Arr[G_u8Counter++] = MSYSTICK_u32GetElapsedTime_SingleShot() / 3.125;
        }
        MSYSTICK_vSetIntervalSingle(4, HIR_vDecodeBits);
    }
}

static void HIR_vDecodeBits(void)
{
    u8 Local_u8Temp = 0;

    if (G_u8Counter >= 25)
    {
        for (u8 i = 0; i < 8; i++)
        {
            if (G_u32Arr[17 + i] >= 900 && G_u32Arr[17 + i] <= 1400)
            {
                CLR_BIT(Local_u8Temp, i);
            }
            else if (G_u32Arr[17 + i] >= 1800 && G_u32Arr[17 + i] <= 2600)
            {
                SET_BIT(Local_u8Temp, i);
            }
        }

        G_u8DecodedValue = Local_u8Temp;
        G_u8NewKeyReady  = 1;
    }

    G_u8StartingFlag = 0;
    G_u8Counter      = 0;

    for (u8 i = 0; i < 50; i++)
    {
        G_u32Arr[i] = 0;
    }
}
