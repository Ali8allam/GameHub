/*
 * TIM2_int.h
 *
 *  Created on: Sep 16, 2026
 *      Author: bigor
 */

#ifndef MCAL_TIM2_TIM2_INT_H_
#define MCAL_TIM2_TIM2_INT_H_


void MTIM2_vInit(void);
void MTIM2_vStartTimer(void);
void MTIM2_vStopTimer(void);
void MTIM2_vSetCallBack(void (*A_Fptr)(void));


#endif /* MCAL_TIM2_TIM2_INT_H_ */
