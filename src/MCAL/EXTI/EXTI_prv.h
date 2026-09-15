#ifndef MCAL_EXTI_EXTI_PRV_H_
#define MCAL_EXTI_EXTI_PRV_H_

#define EXITI_BASE_ADDR 0x40010400U

typedef  struct {
	u32 IMR;
	u32 EMR;
	u32 RTSR;
	u32 FTSR;
	u32 SWIER;
	u32 PR;

}EXTI_MemMap_t;

#define EXTI ((volatile EXTI_MemMap_t*)(EXITI_BASE_ADDR))
#endif
