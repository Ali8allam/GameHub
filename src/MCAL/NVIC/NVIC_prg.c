#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "NVIC_int.h"
#include "NVIC_prv.h"


void MNVIC_vEnable_Prephiral_Init(u8 A_u8Position)
{
	SET_BIT(NVIC->ISERx[ A_u8Position/32], A_u8Position%32 );
}
void MNVIC_vDisable_Prephiral_Init(u8 A_u8Position)
{
	SET_BIT(NVIC->ICERx[ A_u8Position/32], A_u8Position%32 );
}

void MNVIC_vSetPendeningFlag(u8 A_u8Position)
{
	SET_BIT(NVIC ->ISPRx[ A_u8Position/32], A_u8Position%32 );
}
void MNVIC_vCLRPendeningFlag(u8 A_u8Position)
{
	SET_BIT(NVIC ->ICPRx[ A_u8Position/32], A_u8Position%32 );
}

u8 MNVIC_vGetFlagStatus(u8 A_u8Position)
{
	return GET_BIT(NVIC->IABRx[A_u8Position/32], A_u8Position%32);
}

u8 G_u8GroupPriority =0;
// SCB
void MNVIC_SetGroupPriority(NVIC_Group_t A_xGroupPriority)
{
	G_u8GroupPriority = A_xGroupPriority;
	SCB_AIRCR = VECTKEY | ( A_xGroupPriority <<8);
}


void MNVIC_SetPeripheralPriority(u8 A_u8Position, u8 A_u8Group , u8 A_u8SubGroup)
{
	switch (G_u8GroupPriority)
	{
	case Group16sub0:
		NVIC ->IPRx[A_u8Position] = A_u8Group <<4;
		break;
	case Group8sub2:
			NVIC ->IPRx[A_u8Position] = A_u8Group <<5 | A_u8Group <<4 ;
			break;
	case Group4sub4:
			NVIC ->IPRx[A_u8Position] = A_u8Group <<6 | A_u8Group <<4 ; // sub grp start from any bit after the reserved bits  3 2 1 0
			break;
	case Group2sub8:
			NVIC ->IPRx[A_u8Position] = A_u8Group <<7 | A_u8Group <<4 ;
			break;
	case Group0sub16:
			NVIC ->IPRx[A_u8Position] =  A_u8Group <<4 ;
			break;

	}
}

