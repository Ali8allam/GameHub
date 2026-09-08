#include "../../LIB/STD_TYPES.h"
#include "../../MCAL/GPIO/GPIO_int.h"

#include "SEVENSEGMENT_int.h"
#include "SEVENSEGMENT_prv.h"


void HSEVSEG_vInit(const SevenSegment_t *A_SegCfg)
{
    GPIOx_PinConfig_t segPin = {
        .Port       = A_SegCfg->Port,
        .Mode       = GPIO_MODE_OUTPUT,
        .OutputType = GPIO_OT_PUSHPULL,
        .Speed      = GPIO_SPEED_LOW,
        .PullType   = GPIO_NO_PULL
    };

    //Output pins Init() for SevSeg
    for (u8 i = 0; i < 7; i++)
    {
        segPin.Pin = A_SegCfg->StartPin + i;
        MGPIO_vPinInit(&segPin);
    }
}

void HSEVSEG_vDisplayNumber(const SevenSegment_t *A_SegCfg, u8 A_u8Number)
{
    if (A_u8Number > 9) return;

    u8 L_u8Pattern = K_u8SevSegPatterns[A_u8Number];

    for (u8 i = 0; i < 7; i++)
    {
        u8 L_u8BitVal = (L_u8Pattern >> i) & 0x01;

        // Invert logic for Common Anode displays
        if (A_SegCfg->DisplayType == SEVSEG_COMMON_ANODE)
        {
            L_u8BitVal = !L_u8BitVal;
        }

        MGPIO_vSetPinValue(A_SegCfg->Port, A_SegCfg->StartPin + i, L_u8BitVal);
    }
}
