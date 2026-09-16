/*
 * TIM2_prg.c
 */

#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "TIM2_int.h"
#include "TIM2_prv.h"

static void (*G_TIM2_Fptr)(void) = NULL;

void MTIM2_vInit(void)
{
    // 1. Stop counter while configuring
    CLR_BIT(TIM2->CR1, CEN);

    // 2. Set Prescaler for 1 kHz clock (25 MHz / 25000 = 1000 Hz)
    TIM2->PSC = 24999;

    // 3. Set Auto-Reload value for 1 second (1000 ticks of 1 ms)
    TIM2->ARR = 999;

    // 4. Force an update event to reload PSC and ARR into shadow registers
    SET_BIT(TIM2->EGR, UG);

    // 5. Clear update flag caused by UG bit
    CLR_BIT(TIM2->SR, UIF);

    // 6. Enable Update Interrupt
    SET_BIT(TIM2->DIER, UIE);
}

void MTIM2_vStartTimer(void)
{
    TIM2->CNT = 0;
    SET_BIT(TIM2->CR1, CEN);
}

void MTIM2_vStopTimer(void)
{
    CLR_BIT(TIM2->CR1, CEN);
    TIM2->CNT = 0;
}

void MTIM2_vSetCallBack(void (*A_Fptr)(void))
{
    if (A_Fptr != NULL)
    {
        G_TIM2_Fptr = A_Fptr;
    }
}

// TIM2 Global Interrupt Handler (IRQ position 28)
void TIM2_IRQHandler(void)
{
    // Check if Update Interrupt Flag is set
    if (GET_BIT(TIM2->SR, UIF) == 1)
    {
        // Clear flag
        CLR_BIT(TIM2->SR, UIF);

        // Execute Callback
        if (G_TIM2_Fptr != NULL)
        {
            G_TIM2_Fptr();
        }
    }
}
