#ifndef IR_INT_H
#define IR_INT_H

#include "../../LIB/STD_TYPES.h"

/* Car MP3 Mini Remote Control Command Codes (NEC Protocol) */
#define IR_KEY_NONE         0xFF

#define IR_KEY_POWER        0x45
#define IR_KEY_MODE         0x46
#define IR_KEY_MUTE         0x47

#define IR_KEY_PLAY_PAUSE   0x44
#define IR_KEY_PREV         0x40
#define IR_KEY_NEXT         0x43

#define IR_KEY_EQ           0x07
#define IR_KEY_VOL_MINUS    0x15
#define IR_KEY_VOL_PLUS     0x09

#define IR_KEY_0            0x16
#define IR_KEY_RPT          0x19
#define IR_KEY_USD          0x0D

#define IR_KEY_1            0x0C
#define IR_KEY_2            0x18
#define IR_KEY_3            0x5E
#define IR_KEY_4            0x08
#define IR_KEY_5            0x1C
#define IR_KEY_6            0x5A
#define IR_KEY_7            0x42
#define IR_KEY_8            0x52
#define IR_KEY_9            0x4A

/* Control Aliases */
#define IR_NAV_UP           IR_KEY_VOL_PLUS
#define IR_NAV_DOWN         IR_KEY_VOL_MINUS
#define IR_NAV_LEFT         IR_KEY_PREV
#define IR_NAV_RIGHT        IR_KEY_NEXT
#define IR_NAV_OK           IR_KEY_PLAY_PAUSE
#define IR_NAV_EXIT         IR_KEY_POWER

void HIR_vInit(void);
u8   HIR_u8GetKey(void);

#endif