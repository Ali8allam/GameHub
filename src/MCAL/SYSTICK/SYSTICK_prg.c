#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "SYSTICK_int.h"
#include "SYSTICK_prv.h"

static void (*G_xFptr)(void) = NULL;
static u8 G_u8SingleFlag =0;



void MSYSTICK_vInit(MSYSTICK_Config_t *A_xcfg)
{	// stop
	CLR_BIT(SYSTICK->CTRL , ENABLE);
	if(A_xcfg->InterruptEnable == INT_ENABLE)
	{
		SET_BIT(SYSTICK->CTRL , TICKINT );
	}else if(A_xcfg->InterruptEnable == INT_DISABLE)
	{
		CLR_BIT(SYSTICK->CTRL , TICKINT );
	}

	if(A_xcfg->CLK_SRC == CLK_SRC_AHB)
		{
			SET_BIT(SYSTICK->CTRL ,CLKSOURCE );

		}else if(A_xcfg->CLK_SRC == CLK_SRC_AHB_8)
		{
			CLR_BIT(SYSTICK->CTRL , CLKSOURCE );
		}

}
void MSYSTICK_vStartTimer(u32 A_u32LoadValue)
{
	SYSTICK->LOAD = A_u32LoadValue;
	SYSTICK->VAL =0;

	SET_BIT(SYSTICK->CTRL , ENABLE);


}

void MSYSTICK_vStopTimer(void)
{

	CLR_BIT(SYSTICK->CTRL , ENABLE);
	SYSTICK->VAL =0;
}
u32 MSYSTICK_u32GetElapsedTime_SingleShot(void)
{
	return (SYSTICK->LOAD) - (SYSTICK->VAL);
}
u32 MSYSTICK_u32GetRemainingTime_SingleShot(void)
{
	return (SYSTICK->VAL);
}
void MSYSTICK_vSetDelay_ms(f64 A_f64Delay_ms)
{
	// calculation based on the clk system which is 8M & AHB/8


	u32 L_u32Ticks = (u32)(A_f64Delay_ms *1000.0 );

	SYSTICK->VAL =0;
	if(L_u32Ticks >= 0x00000001 && L_u32Ticks < 0x00FFFFFF )
	{
		 MSYSTICK_vStartTimer(L_u32Ticks);
		 while(!GET_BIT(SYSTICK->CTRL , COUNTFLAG))
		 {

		 }
		 MSYSTICK_vStopTimer();
	}


}
void MSYSTICK_vSetDelay_us(u32 A_u32Delay_us)
{
    u32 L_u32Ticks = A_u32Delay_us;

    SYSTICK->VAL = 0;

    if((L_u32Ticks >= 1) && (L_u32Ticks < 0x00FFFFFF))
    {
        MSYSTICK_vStartTimer(L_u32Ticks);

        while(GET_BIT(SYSTICK->CTRL, COUNTFLAG) == 0)
            ;

        MSYSTICK_vStopTimer();
    }
}
//void MSYSTICK_vSetDelay_us(f64 A_f64Delay_ms)
//{
//
//}
void MSYSTICK_vSetIntervalSingle(u32 A_u32Delay_ms , void (*Fptr)(void))
{
	G_u8SingleFlag = 1;
	u32 L_u23Ticks = (u32)(A_u32Delay_ms *1000.0 );

	G_xFptr = Fptr ;
	SYSTICK->VAL =0;
	if(L_u23Ticks >= 0x00000001 && L_u23Ticks < 0x00FFFFFF )
		{
			 MSYSTICK_vStartTimer(L_u23Ticks);
		}
}
void MSYSTICK_vSetIntervalMulti(u32 A_u32Delay_ms , void (*Fptr)(void))
{
	G_u8SingleFlag = 0;
	u32 L_u23Ticks = (u32)(A_u32Delay_ms *1000);

	G_xFptr = Fptr ;
	SYSTICK->VAL =0;
	if(L_u23Ticks >= 0x00000001 && L_u23Ticks < 0x00FFFFFF )
		{
			 MSYSTICK_vStartTimer(L_u23Ticks);
		}
}

//extern void xPortSysTickHandler(void); // MCAL driver requires keeping its ISR implementation,
//map FreeRTOS in FreeRTOSConfig.h to use xPortSysTickHandler explicitly,
//and invoke the FreeRTOS handler from within your SYSTICK_prg.c:

void SysTick_Handler (void)
{
	//xPortSysTickHandler();
	if(G_xFptr != NULL)
	{
		G_xFptr();
	}
	if(G_u8SingleFlag == 1)
	{
		MSYSTICK_vStopTimer();
		G_u8SingleFlag = 0;
	}
}
