#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "SYSCFG_int.h"
#include "SYSCFG_prv.h"

void MSYSCFG_vSetLinePort(u8 A_u8LineNo, u8 A_u8PortNo)
{
    if (A_u8LineNo > 15) return;

    u8 RegNo = A_u8LineNo / 4;
    u8 shift_value = (A_u8LineNo % 4) * 4;

    // FIX 1: Shift the 4-bit mask by shift_value (not just A_u8LineNo % 4)
    SYSCFG->EXTICRx[RegNo] &= ~(0b1111 << shift_value);

    // FIX 2: Write port number shifted to the correct field position
    SYSCFG->EXTICRx[RegNo] |= (A_u8PortNo << shift_value);
}
