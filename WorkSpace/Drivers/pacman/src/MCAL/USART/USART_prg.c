/*
 * USART_prg.c
 *
 *  Created on: Aug 31, 2026
 *      Author: Mohamed
 */
#include "USART_int.h"
#include "USART_prv.h"
#include "USART_cfg.h"
/* Transmit Interrupt Buffers */
static volatile char* G_TxBuffer = NULL;
static volatile u32 G_TxIndex = 0;

/* Receive Interrupt Buffers */
static volatile char G_RxBuffer[100];
static volatile u8 G_RxIndex = 0;
static volatile u8 G_RxReadyFlag = 0;
u8* MUSART_u8ptrReceiveString(void) // check '\r' | '\n'
{
	u8 L_u8index = 0;
    u8 ch = 0;

    while(1)
    {

        ch = MUSART_u8ReceiveData();
        if((ch == '\r' || ch == '\n') && L_u8index == 0)
        {
        	continue;
        }
        if(ch == '\r' || ch == '\n')
        {

            G_u8Buffer[L_u8index] = '\0';

            break;
        }
        else
        {
            G_u8Buffer[L_u8index] = ch;
            L_u8index++;
            if(L_u8index >= 99)
            {
                G_u8Buffer[L_u8index] = '\0';
                break;
            }
        }
    }

    return G_u8Buffer;
}

void MUSART_vInit(void){

CLR_BIT(USART1->CR1, 12);

// NO Parity Bit
CLR_BIT(USART1->CR1, 10);

// STOP BIT
CLR_BIT(USART1->CR2, 12);
CLR_BIT(USART1->CR2, 13);

//TRANSIMTER ENABLE
SET_BIT(USART1->CR1, 3);

//RECIVER ENABLE
SET_BIT(USART1->CR1, 2);

//USART ENABLE
SET_BIT(USART1->CR1, 13);

//BAUD RATE 9600
USART1->BRR = (52 << 4) | 1;
}

void MUSART_vSendData(u8 A_u8Data){

	while (GET_BIT(USART1->SR, 7) == 0);
	USART1->DR = A_u8Data;
	while(GET_BIT(USART1->SR, 6));
	CLR_BIT(USART1->SR, 6);

}

u8 MUSART_u8ReceiveData(){
	while (GET_BIT(USART1->SR, 5) == 0);

	return USART1->DR;
}

/* ================= ADDED MISSING FUNCTIONS ================= */

static volatile u8 G_data;

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

void MUSART_vSendStringAsynch(char* A_charPtrData)
{
    if(A_charPtrData != NULL)
    {
    	G_TxBuffer = A_charPtrData;
        G_TxIndex = 0;

        MUSART_vEnable_TX_Interrupt();
    }
}
u8 MUSART_u8GetRxReadyFlag(void)
{
    return G_RxReadyFlag;
}

void MUSART_vClearRxReadyFlag(void)
{
    G_RxReadyFlag = 0;
}

char* MUSART_ptrGetRxBuffer(void)
{
    return (char*)G_RxBuffer;
}

void USART1_IRQHandler(void)
{
    if((GET_BIT(USART1->SR, 5) == 1) && (GET_BIT(USART1->CR1, 5) == 1))
    {
        u8 ch = USART1->DR;
        if((ch == '\r' || ch == '\n') && G_RxIndex == 0)
        {
        }
        else if(ch == '\r' || ch == '\n')
        {
            G_RxBuffer[G_RxIndex] = '\0';
            G_RxIndex = 0;
            G_RxReadyFlag = 1;
        }
        else
        {
            G_RxBuffer[G_RxIndex] = ch;
            G_RxIndex++;

            if(G_RxIndex >= 99)
            {
                G_RxBuffer[G_RxIndex] = '\0';
                G_RxIndex = 0;
                G_RxReadyFlag = 1;
            }
        }
    }

    if((GET_BIT(USART1->SR, 7) == 1) && (GET_BIT(USART1->CR1, 7) == 1))
    {
        if(G_TxBuffer[G_TxIndex] != '\0')
        {
            USART1->DR = G_TxBuffer[G_TxIndex];
            G_TxIndex++;
        }
        else
        {
            MUSART_vDisable_TX_Interrupt();
            MUSART_vEnable_TC_Interrupt();
        }
    }

    if((GET_BIT(USART1->SR, 6) == 1) && (GET_BIT(USART1->CR1, 6) == 1))
    {
        CLR_BIT(USART1->SR, 6);
        MUSART_vDisable_TC_Interrupt();
        G_TxBuffer = NULL;
    }
}
