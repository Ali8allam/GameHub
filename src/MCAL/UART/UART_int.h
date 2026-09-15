/*
 * UART_int.h
 *
 *  Created on: Aug 31, 2026
 *      Author: ALI & ADHM
 */

#ifndef MCAL_UART_UART_INT_H_
#define MCAL_UART_UART_INT_H_


void MUSART_vInit();
void MUSART_vSend(u8 A_u8Data);
u8 MUSART_ReceiveData(void);

void MUART_vEnable_TX_Interrupt(void);
void MUART_vDisable_TX_Interrupt(void);
void MUART_vEnable_TC_Interrupt(void);
void MUART_vDISable_TC_Interrupt(void);

void MUSART_vEnable_TX_Interrupt(void);
void MUSART_vDisable_TX_Interrupt(void);
void MUSART_vEnable_TC_Interrupt(void);
void MUSART_vDisable_TC_Interrupt(void);
void MUSART_vEnable_RX_Interrupt(void);
void MUSART_vDisable_RX_Interrupt(void);

void MUSART_vSendString(u8* A_u8PtrData);
u8* MSUART_vRecieveString(void);

void MUSART_vSendchar(u8 A_u8Data);


#endif /* MCAL_UART_UART_INT_H_ */
