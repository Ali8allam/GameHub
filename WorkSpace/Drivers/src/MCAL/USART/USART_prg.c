
#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "USART_int.h"
#include "USART_prv.h"

static volatile u8 G_data;
static const u8 *G_pu8TxString = NULL;
static volatile u32 G_u32TxIndex = 0;


void MUSART_vInit(void)
{
	// OVERSAMPLE BY 16
CLR_BIT(USART1->CR1, 15);

	// DATA LENGTH 8
CLR_BIT(USART1->CR1, 12);

	// NO PARITY
CLR_BIT(USART1->CR1, 10);

	//BAUD RATE 9600
	USART1->BRR = (162<<4) | 13;

	//1 STOP BIT
CLR_BIT(USART1->CR2, 12);
CLR_BIT(USART1->CR2, 13);

// TRANSIMTER ENABLE
SET_BIT(USART1->CR1, 3);

// RECEIVER ENABLE
SET_BIT(USART1->CR1, 2);

// USART ENABLE
SET_BIT(USART1->CR1, 13);


}

void MUSART_vSendData(u8 A_u8Data)
{
	while(!GET_BIT(USART1->SR, 7));

	USART1->DR = A_u8Data;

	while(!GET_BIT(USART1->SR, 6));

	CLR_BIT(USART1->SR, 6);

}
u8 MUSART_u8ReceiveData(void)
{
while(!GET_BIT(USART1->SR, 5));

return USART1->DR;
}

void MUSART_vEnable_TX_Interrupt(void)
{
    SET_BIT(USART1->CR1, 7);
}

void MUSART_vDisable_TX_Interrupt(void)
{
    CLR_BIT(USART1->CR1, 7);
}

void MUSART_vEnable_TC_Interrupt(void)
{
    SET_BIT(USART1->CR1, 6);
}

void MUSART_vDisable_TC_Interrupt(void)
{
    CLR_BIT(USART1->CR1, 6);
}

void MUSART_vEnable_RX_Interrupt(void)
{
    SET_BIT(USART1->CR1, 5);
}

void MUSART_vDisable_RX_Interrupt(void)
{
    CLR_BIT(USART1->CR1, 5);
}

void MUSART_vSendStringAsync(const u8 *A_pu8String)
{
    if (A_pu8String != NULL)
    {
        G_pu8TxString = A_pu8String;
        G_u32TxIndex = 0;


        USART1->DR = G_pu8TxString[G_u32TxIndex++];
        MUSART_vEnable_TX_Interrupt();
    }
}

void USART1_IRQHandler(void)
{

    if (GET_BIT(USART1->SR, 7) == 1 && GET_BIT(USART1->CR1, 7) == 1)
    {
        if (G_pu8TxString != NULL && G_pu8TxString[G_u32TxIndex] != '\0')
        {

            USART1->DR = G_pu8TxString[G_u32TxIndex++];
        }
        else
        {

            MUSART_vDisable_TX_Interrupt();
            MUSART_vEnable_TC_Interrupt();
        }
    }


    if (GET_BIT(USART1->SR, 6) == 1 && GET_BIT(USART1->CR1, 6) == 1)
    {
        CLR_BIT(USART1->SR, 6);
        MUSART_vDisable_TC_Interrupt();
        G_pu8TxString = NULL;
    }
}

