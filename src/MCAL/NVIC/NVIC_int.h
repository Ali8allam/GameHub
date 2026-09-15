#ifndef MCAL_NVIC_INT_H_
#define MCAL_NVIC_INT_H_

typedef enum{

	Group16sub0 = 3,
	Group8sub2,//4
	Group4sub4,//5
	Group2sub8,//6
	Group0sub16//7


}NVIC_Group_t;

void MNVIC_vEnable_Prephiral_Init(u8 A_u8Position);
void MNVIC_vDisable_Prephiral_Init(u8 A_u8Position);

void MNVIC_vSetPendeningFlag(u8 A_u8Position);


void MNVIC_vCLRPendeningFlag(u8 A_u8Position);
u8 MNVIC_vGetFlagStatus(u8 A_u8Position);

void MNVIC_SetGroupPriority(NVIC_Group_t A_xGroupPriority);

void MNVIC_SetPeripheralPriority(u8 A_u8Position, u8 A_u8Group , u8 A_u8SubGroup);


#endif
