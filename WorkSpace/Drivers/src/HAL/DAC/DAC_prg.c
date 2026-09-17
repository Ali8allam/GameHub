/*
 * DAC_prg.c
 *
 * 8-bit resistor-ladder (R-2R) DAC driven by bit-banged GPIO.
 * See DAC_int.h for the API and DAC_cfg.h for the pin map and timing.
 */

#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "../../MCAL/GPIO/GPIO_int.h"

#include "DAC_int.h"
#include "DAC_cfg.h"

/* Sample period in microseconds, derived from the configured sample rate. */
#define DAC_SAMPLE_PERIOD_US    (1000000UL / (u32)HDAC_SAMPLE_RATE_HZ)

/* How many iterations of dac_vDelaySample()'s loop fill one sample period.
 *
 * At -O2 the compiler emits this body for the volatile counter below:
 *
 *     nop                     1 cycle
 *     ldr   r3, [r7, #4]      2
 *     adds  r3, #1            1
 *     str   r3, [r7, #4]      2
 *     ldr   r3, [r7, #4]      2
 *     cmp.w r3, #N            2
 *     bcc.n loop              3 taken / 1 not taken
 *
 * i.e. HDAC_DELAY_LOOP_CYCLES cycles per iteration. At 25 MHz one 250 us
 * sample period is 6250 cycles, so the loop runs 6250 / 13 = 480 times. */
#define DAC_DELAY_ITERATIONS                                            \
    (((HDAC_CPU_CLOCK_HZ / 1000000UL) * DAC_SAMPLE_PERIOD_US)           \
     / HDAC_DELAY_LOOP_CYCLES)

static GPIOx_PinConfig_t s_xDacPins[HDAC_PIN_COUNT];
static u8 s_u8DacPinsNum = 0;
static u8 s_u8DacReady = 0;

/* Hold the current ladder code for exactly one sample period.
 *
 * This must NOT use SysTick: the IR receiver measures every IR pulse with
 * MSYSTICK_vSetIntervalSingle() and decodes from SysTick_Handler, so
 * reprogramming SysTick here would corrupt every captured pulse width and no
 * remote key would decode while audio was playing. */
static void dac_vDelaySample(void)
{
    for (volatile u32 i = 0; i < DAC_DELAY_ITERATIONS; i++)
    {
        __asm__("nop");
    }
}

void HDAC_vInit(const GPIOx_PinConfig_t *A_xPins, u8 A_u8PinsNo)
{
    if ((A_xPins == NULL) || (A_u8PinsNo == 0))
    {
        return;
    }

    s_u8DacPinsNum = (A_u8PinsNo > HDAC_PIN_COUNT) ? HDAC_PIN_COUNT : A_u8PinsNo;

    for (u8 i = 0; i < s_u8DacPinsNum; i++)
    {
        s_xDacPins[i] = A_xPins[i];
        MGPIO_vPinInit(&s_xDacPins[i]);
    }

    s_u8DacReady = 1;

    /* Park the ladder at mid-rail so the speaker stays quiet until playback. */
    HDAC_vOutputByte(HDAC_SILENCE_LEVEL);
}

void HDAC_vOutputByte(u8 A_u8Sample)
{
    for (u8 i = 0; i < s_u8DacPinsNum; i++)
    {
        /* Bit i drives pin i: the configured order must be LSB-first. */
        MGPIO_vSetPinValue(s_xDacPins[i].Port, s_xDacPins[i].Pin,
                           GET_BIT(A_u8Sample, i));
    }
}

void HDAC_vSendSample(const u8 *A_u8Ptr, u32 A_u32Index)
{
    if (A_u8Ptr != NULL)
    {
        HDAC_vOutputByte(A_u8Ptr[A_u32Index]);
    }
}

void HDAC_vSilence(void)
{
    HDAC_vOutputByte(HDAC_SILENCE_LEVEL);
}

u8 HDAC_u8IsReady(void)
{
    return s_u8DacReady;
}

void HDAC_vPlaySoundSync(const u8 *A_u8Samples, u32 A_u32Length)
{
    if (!s_u8DacReady || (A_u8Samples == NULL) || (A_u32Length == 0))
    {
        return;
    }

    for (u32 i = 0; i < A_u32Length; i++)
    {
        HDAC_vOutputByte(A_u8Samples[i]);
        dac_vDelaySample();
    }

    HDAC_vOutputByte(HDAC_SILENCE_LEVEL);
}

void HDAC_vStreamSamples(const u8 *A_u8Samples, u32 A_u32Length,
                         u32 *A_pu32Index, u16 A_u16SampleCount)
{
    if (!s_u8DacReady || (A_u8Samples == NULL) || (A_u32Length == 0) ||
        (A_pu32Index == NULL))
    {
        return;
    }

    if (*A_pu32Index >= A_u32Length)
    {
        *A_pu32Index = 0;
    }

    for (u16 i = 0; i < A_u16SampleCount; i++)
    {
        HDAC_vOutputByte(A_u8Samples[*A_pu32Index]);

        (*A_pu32Index)++;
        if (*A_pu32Index >= A_u32Length)
        {
            *A_pu32Index = 0;
        }

        dac_vDelaySample();
    }
}

void HDAC_vHoldSilence(u16 A_u16SampleCount)
{
    if (!s_u8DacReady)
    {
        return;
    }

    HDAC_vOutputByte(HDAC_SILENCE_LEVEL);

    for (u16 i = 0; i < A_u16SampleCount; i++)
    {
        dac_vDelaySample();
    }
}
