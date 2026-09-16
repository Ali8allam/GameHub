/*
 * SNAKE_int.h
 *
 *  Created on: Sep 16, 2026
 *      Author: bigor
 */

#ifndef APP_SNAKE_INT_H_
#define APP_SNAKE_INT_H_

#include "../LIB/STD_TYPES.h"

#define SNAKE_STATE_CONTINUE   1
#define SNAKE_STATE_EXIT       0

void HSNAKE_vInit(void);
u8   HSNAKE_u8Update(u8 A_u8Key);

#endif /* APP_SNAKE_INT_H_ */
