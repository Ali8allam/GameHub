#ifndef MCAL_S2P_S2P_INT_H_
#define MCAL_S2P_S2P_INT_H_
typedef struct{
	u8 DataPort;
	u8 DataPin;

	u8 ShiftCLKPort;
	u8 ShiftCLKPin;

	u8 LatchCLKPort;
	u8 LatchCLKPin;

}S2P_Init_t;

void HS2P_vInit(S2P_Init_t* A_xInit);
void HS2P_vSendData(S2P_Init_t* A_xInit , u32 A_u32Byte);

#endif
