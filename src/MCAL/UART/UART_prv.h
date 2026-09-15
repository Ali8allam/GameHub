/*
 * UART_prv.h
 *
 *  Created on: Aug 31, 2026
 *      Author: ALI & ADHM
 */

#ifndef MCAL_UART_UART_PRV_H_
#define MCAL_UART_UART_PRV_H_

//#define UART5_BASE_ADDR 0x40005000U
//#define UART4_BASE_ADDR 0x40004C00U
#define USART3_BASE_ADDR 0x40004800U
#define USART2_BASE_ADDR 0x40004400U


typedef struct {
	u32 SR;
	u32 DR;
	u32 BRR;
	u32 CR1;
	u32 CR2;
	u32 CR3;
	u32 GTPR;

}USARTx_MemMap_t;

#define USART3		((volatile USARTx_MemMap_t*)(USART3_BASE_ADDR))
#define USART2		((volatile USARTx_MemMap_t*)(USART2_BASE_ADDR))

#endif /* MCAL_UART_UART_PRV_H_ */
