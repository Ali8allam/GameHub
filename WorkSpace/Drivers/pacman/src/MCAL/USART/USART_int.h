/*
 * USART_int.h
 *
 *  Created on: Aug 31, 2026
 *      Author: Mohamed
 */

#ifndef MCAL_USART_USART_INT_H_
#define MCAL_USART_USART_INT_H_

#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

u8 G_u8Buffer[100];

void MUSART_vInit(void);
void MUSART_vSendData(u8 A_u8Data);
u8 MUSART_u8ReceiveData();
u8 MUSART_vReceive_synch(u8* A_pu8Byte);
u8* MUSART_u8ptrReceiveString(void);
//INTERRUPTS
void MUSART_vEnable_TX_Interrupt(void);
void MUSART_vDisable_TX_Interrupt(void);
void MUSART_vEnable_TC_Interrupt(void);
void MUSART_vDisable_TC_Interrupt(void);
void MUSART_vEnable_RX_Interrupt(void);
void MUSART_vDisable_RX_Interrupt(void);

void MUSART_vSendString(char* A_u8ptrStr);
void MUSART_vSendStringAsynch(char* A_ptrStr);


#endif /* MCAL_USART_USART_INT_H_ */
