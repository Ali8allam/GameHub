#ifndef MCAL_SYSCFG_SYSCFG_PRV_H_
#define MCAL_SYSCFG_SYSCFG_PRV_H_

#define SYSCFG_BASE_ADDR 0x40010000U

typedef struct {
	u32 EVCR;
	u32 MAPR;
	u32 EXTICRx[4];
	u32 MAPR2;

}SYSCFG_MemMap_t;

#define SYSCFG ((volatile SYSCFG_MemMap_t*)(SYSCFG_BASE_ADDR))

//#define SYSCFG_PORTA 0b0000
//#define SYSCFG_PORTB 0b0001
//#define SYSCFG_PORTC 0b0010

#endif
