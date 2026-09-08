#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/SYSTICK/SYSTICK_int.h"

#include "../S2P/S2P_int.h"
#include "LEDMATRIX_int.h"
#include "LEDMATRIX_prv.h"
#include "LEDMATRIX_cfg.h"

static S2P_Init_t* Global_S2PConfig;

void HLEDMATRIX_vInit(S2P_Init_t* A_xS2PConfig)
{
    Global_S2PConfig = A_xS2PConfig;

    /* Initialize S2P GPIO Pins */
    HS2P_vInit(Global_S2PConfig);

    /* Initialize SysTick Timer */
    MSYSTICK_Config_t STK_cfg = {
        .InterruptEnable = INT_DISABLE,
        .CLK_SRC = CLK_SRC_AHB_8
    };
    MSYSTICK_vInit(&STK_cfg);
}

void HLEDMATRIX_vDisplayFrame(u8 A_u8Frame[], u32 A_u32FrameDelay)
{
    u32 Local_u32CombinedData = 0;

    for(u32 j = 0; j < A_u32FrameDelay; j++)
    {
        for(u8 i = 0; i < 8; i++)
        {
            /*
             * Byte 1 (Bits 8-15): Active LOW Row/Column scan selection (~(1 << i))
             * Byte 0 (Bits 0-7) : Active HIGH LED Data (A_u8Frame[i])
             */
            Local_u32CombinedData = (((u32)(~(1 << i)) & 0xFF) << 8) | A_u8Frame[i];

            /* Send 16 bits to cascaded 74HC595 shift registers */
            HS2P_vSendData(Global_S2PConfig, Local_u32CombinedData);

            /* Multiplex Delay */
            MSYSTICK_vSetDelay_ms(SCAN_TIME);

            /* Anti-ghosting clear */
            HS2P_vSendData(Global_S2PConfig, 0xFF00);
        }
    }
}
