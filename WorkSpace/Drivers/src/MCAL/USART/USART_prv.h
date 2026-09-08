/*
 * USART_prv.h
 *
 *  Created on: Aug 31, 2026
 *      Author: ??
 */

#ifndef MCAL_USART_USART_PRV_H_
#define MCAL_USART_USART_PRV_H_

#define USART1_BASE_ADDRESS	0x40011000U
#define USART2_BASE_ADDRESS	0x40004400U
#define USART6_BASE_ADDRESS	0x40011400U

typedef struct{
u32 SR;
u32 DR;
u32 BRR;
u32 CR1;
u32 CR2;
u32 CR3;
u32 GTRP;

}USARTx_MemMap_t;

#define USART1	((volatile USARTx_MemMap_t*)(USART1_BASE_ADDRESS))
#define USART2	((volatile USARTx_MemMap_t*)(USART2_BASE_ADDRESS))
#define USART6	((volatile USARTx_MemMap_t*)(USART6_BASE_ADDRESS))



#endif /* MCAL_USART_USART_PRV_H_ */
