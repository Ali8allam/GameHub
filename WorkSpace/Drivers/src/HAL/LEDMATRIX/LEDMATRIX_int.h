#ifndef HAL_LEDMATRIX_INT_H_
#define HAL_LEDMATRIX_INT_H_

#include "../S2P/S2P_int.h"

void HLEDMATRIX_vInit(S2P_Init_t* A_xS2PConfig);
void HLEDMATRIX_vDisplayFrame(u8 A_u8Frame[], u32 A_u32FrameDelay);

#endif /* HAL_LEDMATRIX_INT_H_ */
