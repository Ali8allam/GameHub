/*
 * UART_prg.c
 *
 *  Created on: Aug 31, 2026
 *      Author: ALI & ADHM
 */
#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "../../MCAL/SYSTICK/SYSTICK_int.h"

#include "UART_int.h"
#include "UART_prv.h"


void MUSART_vInit()
{
	// data length 8
	CLR_BIT(USART2->CR1 , 12);
	// no parity
	CLR_BIT(USART2->CR1 , 10);
	// baud rate 9600

	// 1 stop bit
	CLR_BIT(USART2->CR2 , 12);
	CLR_BIT(USART2->CR2 , 13);

	// 234.375
	USART2->BRR = (52<<4) | 2;
	// enable transmitter and enable
	SET_BIT(USART2->CR1  , 3);
	SET_BIT(USART2->CR1  , 2);

	// usart enable
	SET_BIT(USART2->CR1  , 13);

}
void MUSART_vSend(u8 A_u8Data)
{
	while (!GET_BIT(USART2-> SR , 7));

	 USART2->DR = A_u8Data;

	 while (!GET_BIT(USART2-> SR , 6));
	 CLR_BIT(USART2-> SR , 6);
}
u8 MUSART_ReceiveData(void)
{
	while (!GET_BIT(USART2-> SR , 5)); //keep receiving data while the RXNE is not 1
		return USART2->DR ;
}


void MUSART_vEnable_TX_Interrupt(void)
{
    SET_BIT(USART2->CR1, 7);
}

void MUSART_vDisable_TX_Interrupt(void)
{
    CLR_BIT(USART2->CR1, 7);
}

void MUSART_vEnable_TC_Interrupt(void)
{
    SET_BIT(USART2->CR1, 6);
}

void MUSART_vDisable_TC_Interrupt(void)
{
    CLR_BIT(USART2->CR1, 6);
}

void MUSART_vEnable_RX_Interrupt(void)
{
    SET_BIT(USART2->CR1, 5);
}

void MUSART_vDisable_RX_Interrupt(void)
{
    CLR_BIT(USART2->CR1, 5);
}

//const u8* G_data;
u8 G_Buffer[50];
//
//u8 G_u8len=0;
//u8 G_u8Indx=0;


// Add a state variable at the top of your file
typedef enum {
    UART_IDLE,
    UART_BUSY
} UART_State_t;

volatile UART_State_t G_UartTxState = UART_IDLE;
const u8* G_data;
u8 G_u8Indx = 0;

volatile u8 G_RxBuffer[100];
volatile u8 G_RxIndex = 0;
volatile u8 G_RxReadyFlag = 0;

const u8* G_TxBuffer = NULL;
volatile u8 G_TxIndex = 0;

void MUSART_vSendString(u8* A_u8PtrData)

{
    // Only accept new data if the UART is idle
    if (G_UartTxState == UART_IDLE)
    {
        G_UartTxState = UART_BUSY;
        G_TxBuffer = A_u8PtrData; // assign input string to G_data
        G_TxIndex = 0;
        MUSART_vEnable_TX_Interrupt();
    }
    // Optional: Return an error code (e.g., NOK) if BUSY
}
u8* MSUART_vRecieveString(void)
{
    u8 i = 0;
    u8 ch = 0;

    while (i < 49)
    {
        ch = MUSART_ReceiveData();

        if (ch == '\r' || ch == '\n')
        {
            break; // Stop receiving on enter or newline
        }

        G_Buffer[i] = ch;
        i++;
    }
    // Properly null-terminate the string
    G_Buffer[i] = '\0';

    return G_Buffer;
}


void USART2_IRQHandler(void)
{
    /* --- 1. RXNE Interrupt Handling (Data Received) --- */
    if((GET_BIT(USART2->SR, 5) == 1) && (GET_BIT(USART2->CR1, 5) == 1))
    {
        u8 ch = USART2->DR;

        // Ignore leading \r or \n if the buffer is empty
        if((ch == '\r' || ch == '\n') && G_RxIndex == 0)
        {
            // Skip empty carriage returns
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

            // Prevent buffer overflow (assumes G_RxBuffer size >= 100)
            if(G_RxIndex >= 99)
            {
                G_RxBuffer[G_RxIndex] = '\0';
                G_RxIndex = 0;
                G_RxReadyFlag = 1;
            }
        }
    }

    /* --- 2. TXE Interrupt Handling (Transmit Data Register Empty) --- */
    if((GET_BIT(USART2->SR, 7) == 1) && (GET_BIT(USART2->CR1, 7) == 1))
    {
        if(G_TxBuffer[G_TxIndex] != '\0')
        {
            USART2->DR = G_TxBuffer[G_TxIndex];
            G_TxIndex++;
        }
        else
        {
            MUSART_vDisable_TX_Interrupt();
            MUSART_vEnable_TC_Interrupt();
        }
    }

    /* --- 3. TC Interrupt Handling (Transmission Complete) --- */
    if((GET_BIT(USART2->SR, 6) == 1) && (GET_BIT(USART2->CR1, 6) == 1))
    {
        CLR_BIT(USART2->SR, 6);
        MUSART_vDisable_TC_Interrupt();
        G_TxBuffer = NULL;

        G_UartTxState = UART_IDLE;
    }
}
