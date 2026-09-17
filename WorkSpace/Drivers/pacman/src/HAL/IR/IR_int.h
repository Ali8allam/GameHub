#ifndef HAL_IR_INT_H
#define HAL_IR_INT_H

#include "../../LIB/STD_TYPES.h"

#define IR_CODE_UP 24
#define IR_CODE_DOWN 82
#define IR_CODE_LEFT 8
#define IR_CODE_RIGHT 90
#define IR_CODE_START 28

void HIR_vInit(void);
u8 HIR_u8GetCommand(u8 *command);

#endif