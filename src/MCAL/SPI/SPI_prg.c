/*
 * SPI_prg.c
 *
 *  Created on: Sep 2, 2026
 *      Author: ALI & ADHM
 */
#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/SYSTICK/SYSTICK_int.h"

#include"SPI_int.h"
#include"SPI_prv.h"

void MSPI_vInit()
{


	    // Set as Master Mode
	    SET_BIT(SPI1->CR1, MSTR);

	    /* 3. Set Clock Prescaler to fPCLK/8 (BR[2:0] = 010) */
	    CLR_BIT(SPI1->CR1, 3);
	    SET_BIT(SPI1->CR1, 4);
	    CLR_BIT(SPI1->CR1, 5);

	    CLR_BIT(SPI1->CR1, DFF); // Set Data Frame Format to 8-bit

	    //  MSB for tft configurations
	    CLR_BIT(SPI1->CR1, LSBFIRST);

	    // 6. Enable Software Slave Management (SSM = 1, SSI = 1)
	   //  This prevents the dreaded Multi-Master Mode Fault!
	    SET_BIT(SPI1->CR1, SSM);
	    SET_BIT(SPI1->CR1, SSI);

		 // Set Clock Phase and Polarity (CPHA = 0, CPOL = 0)
		    CLR_BIT(SPI1->CR1, CPHA);
		    CLR_BIT(SPI1->CR1, CPOL);

		    // Enable the SPI Peripheral  (any enable must be in the last of configuration )
		    SET_BIT(SPI1->CR1,SPE);
}
u8 MSPI_vTranscieve(u8 A_u8Data)
{
	while(!GET_BIT(SPI1->SR , TXE));
	SPI1->DR =A_u8Data;

	while(!GET_BIT(SPI1->SR , RXNE));
	return SPI1->DR ;

}



