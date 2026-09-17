/*
 * RCC_prg.c
 *
 *  Created on: Aug 14, 2025
 *      Author: Nada Mamdouh
 *      Version: 0.1
 */

#include "../../LIB/BIT_MATH.h"
#include "../../LIB/STD_TYPES.h"

#include "RCC_int.h"
#include "RCC_prv.h"
#include "RCC_cfg.h"

void MRCC_vInit(void)
{
#if RCC_SYS_CLK == HSE_CLK
	// choose bypass
#if HSE_BYPASS == RC_CLK
	SET_BIT(RCC->CR, HSEBYP);
#elif HSE_BYPASS == MECHANICAL_CLK
	CLR_BIT(RCC->CR, HSEBYP);
#endif
	// HSE on
	SET_BIT(RCC->CR, 16);

	// wait till flag is set
	while(GET_BIT(RCC->CR,17) == 0);

	// switch to hse
	CLR_BIT(RCC->CFGR, 1);
	SET_BIT(RCC->CFGR, 0);

#elif RCC_SYS_CLK == HSI_CLK
	// HSI on
	SET_BIT(RCC->CR, 0);

	// wait till flag is set
	while(GET_BIT(RCC->CR,1) == 0);

	// switch to hsi
	CLR_BIT(RCC->CFGR, 1);
	CLR_BIT(RCC->CFGR, 0);

#elif RCC_SYS_CLK == PLL_CLK
	// 1. Turn on HSE or HSI depending on PLL source (assuming HSE for standard BluePill 72MHz)
	SET_BIT(RCC->CR, 16);
	while(GET_BIT(RCC->CR,17) == 0); // Wait for HSE to be ready

	// 2. Configure PLL (Example: HSE * 9 = 72MHz)
	// Clear PLLSRC and PLLMUL bits first
	RCC->CFGR &= ~(0xF << 18); // Clear PLLMUL
	RCC->CFGR &= ~(1 << 16);   // Clear PLLSRC

	SET_BIT(RCC->CFGR, 16);    // Set PLLSRC to HSE
	RCC->CFGR |= (0b0111 << 18); // Set PLLMUL to x9 (0111 means x9 in CFGR)

	// 3. Turn on PLL
	SET_BIT(RCC->CR, 24); // PLLON

	// 4. Wait till PLL is ready
	while(GET_BIT(RCC->CR, 25) == 0); // PLLRDY

	// 5. Switch system clock to PLL
	CLR_BIT(RCC->CFGR, 0);
	SET_BIT(RCC->CFGR, 1);

#else
#error "Invalid option"
#endif

}

void MRCC_vEnableClk(u8 A_u8BusID, u8 A_u8PeripheralID)
{
	switch(A_u8BusID)
	{
	case RCC_AHB:
		SET_BIT(RCC->AHBENR, A_u8PeripheralID);
		break;
	case RCC_APB1:
		SET_BIT(RCC->APB1ENR, A_u8PeripheralID);
		break;
	case RCC_APB2:
		SET_BIT(RCC->APB2ENR, A_u8PeripheralID);
		break;
	}
}

void MRCC_vDisableClk(u8 A_u8BusID, u8 A_u8PeripheralID)
{
	switch(A_u8BusID)
	{
	case RCC_AHB:
		CLR_BIT(RCC->AHBENR, A_u8PeripheralID);
		break;
	case RCC_APB1:
		CLR_BIT(RCC->APB1ENR, A_u8PeripheralID);
		break;
	case RCC_APB2:
		CLR_BIT(RCC->APB2ENR, A_u8PeripheralID);
		break;
	}
}
