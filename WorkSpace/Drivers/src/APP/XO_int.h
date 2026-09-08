#ifndef XO_H
#define XO_H

#include "../LIB/STD_TYPES.h"

#define XO_STATE_CONTINUE   1
#define XO_STATE_EXIT       0

void XO_vInit(void);
u8   XO_u8HandleInput(u8 A_u8Key);

#endif