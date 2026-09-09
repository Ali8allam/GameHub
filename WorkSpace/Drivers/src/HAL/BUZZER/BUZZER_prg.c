#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/SYSTICK/SYSTICK_int.h"

#include "BUZZER_int.h"

void HBUZZER_vInit(const BUZZER_Config_t* A_pxConfig)
{
    GPIOx_PinConfig_t PinCfg = {
        .Port       = A_pxConfig->Port,
        .Pin        = A_pxConfig->Pin,
        .Mode       = GPIO_MODE_OUTPUT,
        .OutputType = GPIO_OT_PUSHPULL,
        .Speed      = GPIO_SPEED_LOW,
        .PullType   = GPIO_NO_PULL
    };

    MGPIO_vPinInit(&PinCfg);
    MGPIO_vSetPinValue(A_pxConfig->Port, A_pxConfig->Pin, GPIO_LOW);
}

void HBUZZER_vPlayTone(const BUZZER_Config_t* A_pxConfig, u32 A_u32FreqHz, u32 A_u32DurationMs)
{
    if (A_u32FreqHz == 0)
    {
        // Rest | Pause
        MGPIO_vSetPinValue(A_pxConfig->Port, A_pxConfig->Pin, GPIO_LOW);
        MSYSTICK_vSetDelay_ms(A_u32DurationMs);
        return;
    }


    f64 L_f64HalfPeriodUs = 500000.0 / (f64)A_u32FreqHz;


    u32 L_u32Cycles = (A_u32FreqHz * A_u32DurationMs) / 1000;

    for (u32 i = 0; i < L_u32Cycles; i++)
    {
        MGPIO_vSetPinValue(A_pxConfig->Port, A_pxConfig->Pin, GPIO_HIGH);
        MSYSTICK_vSetDelay_us(L_f64HalfPeriodUs);

        MGPIO_vSetPinValue(A_pxConfig->Port, A_pxConfig->Pin, GPIO_LOW);
        MSYSTICK_vSetDelay_us(L_f64HalfPeriodUs);
    }
}

void HBUZZER_vPlaySFX(const BUZZER_Config_t* A_pxConfig, BUZZER_SFX_t A_xSFX)
{
    switch (A_xSFX)
    {
    case SFX_BUTTON_CLICK:
        HBUZZER_vPlayTone(A_pxConfig, NOTE_C6, 20);
        break;

    case SFX_VICTORY:
        HBUZZER_vPlayTone(A_pxConfig, NOTE_C5, 100);
        HBUZZER_vPlayTone(A_pxConfig, NOTE_E5, 100);
        HBUZZER_vPlayTone(A_pxConfig, NOTE_G5, 100);
        HBUZZER_vPlayTone(A_pxConfig, NOTE_C6, 300);
        break;
    }
}

void HBUZZER_vPlayMelody(const BUZZER_Config_t* A_pxConfig, const Note_t* A_pxMelody, u16 A_u16Length)
{
    for (u16 i = 0; i < A_u16Length; i++)
    {
        HBUZZER_vPlayTone(A_pxConfig, A_pxMelody[i].frequency, A_pxMelody[i].duration);

        MSYSTICK_vSetDelay_ms(15);
    }
}
