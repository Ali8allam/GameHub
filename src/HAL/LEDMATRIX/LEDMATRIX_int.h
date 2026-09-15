#ifndef HAL_LEDMATRIX_INT_H_
#define HAL_LEDMATRIX_INT_H_


void HLEDMATRIX_vInit(GPIOx_PinConfig_t * A_xRows ,u8 A_u8RowsNo, GPIOx_PinConfig_t * A_xCols , u8 A_u8ColsNo);
void HLEDMATRIX_vDisplayFrame(u8 Au8Frame[] , u32 A_u32FrameDelay);
#endif
