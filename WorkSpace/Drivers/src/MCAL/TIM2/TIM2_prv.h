/*
 * TIM2_prv.h
 *
 *  Created on: Sep 16, 2026
 *      Author: bigor
 */

#ifndef MCAL_TIM2_TIM2_PRV_H_
#define MCAL_TIM2_TIM2_PRV_H_

#define TIM2_BASE_ADDR   0x40000000U

typedef struct {
    u32 CR1;
    u32 CR2;
    u32 SMCR;
    u32 DIER;
    u32 SR;
    u32 EGR;
    u32 CCMR1;
    u32 CCMR2;
    u32 CCER;
    u32 CNT;
    u32 PSC;
    u32 ARR;
    u32 RCR;
    u32 CCR1;
    u32 CCR2;
    u32 CCR3;
    u32 CCR4;
    u32 BDTR;
    u32 DCR;
    u32 DMAR;
} TIM2_MemMap_t;

#define TIM2    ((volatile TIM2_MemMap_t*)(TIM2_BASE_ADDR))

/* Register bit definitions */
#define CEN     0   // Counter enable (CR1)
#define UIE     0   // Update interrupt enable (DIER)
#define UIF     0   // Update interrupt flag (SR)
#define UG      0   // Update generation (EGR)



#endif /* MCAL_TIM2_TIM2_PRV_H_ */
