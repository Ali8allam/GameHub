#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "../../MCAL/GPIO/GPIO_int.h"
#include "DAC_int.h"
#include "pacman_audio.h"

#define DAC_SILENCE_LEVEL   128U
#define DAC_PIN_COUNT       8U
/* Timing.
 *
 * The clips in pacman_audio.h are sampled at AUDIO_SAMPLE_RATE (4000 Hz), so
 * each sample must be held for exactly 250 us.
 *
 * IMPORTANT: this delay must NOT use SysTick. The IR receiver driver
 * (src/HAL/IR/IR_prg.c) uses MSYSTICK_vSetIntervalSingle() together with
 * MSYSTICK_u32GetElapsedTime_SingleShot() to measure the width of every IR
 * pulse, and SysTick_Handler() is what calls its ir_decode() completion
 * callback. Driving SYSTICK->LOAD / VAL / CTRL from here would cancel the IR
 * timeout and corrupt every captured pulse width, so no IR command would ever
 * decode while audio was playing. The DAC therefore uses a private busy-wait.
 *
 * The CPU runs on HSI with no AHB prescaler (see RCC_cfg.h RCC_SYS_CLK and
 * MRCC_vInit, which never programs HPRE), so the core clock is 8 MHz. The
 * loop below is calibrated for that clock. At -O2 the compiler emits this body
 * for a volatile counter (see the disassembly of dac_delay_sample):
 *
 *     nop                     1 cycle
 *     ldr  r3, [r7, #4]       2
 *     adds r3, #1             1
 *     str  r3, [r7, #4]       2
 *     ldr  r3, [r7, #4]       2
 *     cmp.w r3, #400          2
 *     bcc.n loop              3 taken / 1 not taken
 *
 * i.e. 13 cycles per iteration. 250 us at 8 MHz is 2000 cycles, so the loop
 * needs 2000 / 13 = 154 iterations.
 *
 * These constants are the only things to retune if the clock, the optimisation
 * level or the compiler changes. Verify by timing a long clip: AUDIO_DEATH is
 * 6400 samples and must take about 1.6 s (6400 x 250 us). */
#define DAC_CPU_CLOCK_HZ        8000000UL
#define DAC_DELAY_LOOP_CYCLES   13UL
#define DAC_SAMPLE_PERIOD_US    (1000000UL / (u32)AUDIO_SAMPLE_RATE)
#define DAC_DELAY_ITERATIONS    \
    (((DAC_CPU_CLOCK_HZ / 1000000UL) * DAC_SAMPLE_PERIOD_US) / DAC_DELAY_LOOP_CYCLES)

static void dac_delay_sample(void)
{
    for (volatile u32 i = 0; i < DAC_DELAY_ITERATIONS; i++) {
        __asm__("nop");
    }
}

static GPIOx_PinConfig_t s_dac_pins[DAC_PIN_COUNT];
static u8 s_dac_pins_num = 0;
static u8 s_dac_ready = 0;

/* Loop cursors for the streaming (per-tick) sounds. */
static u32 s_waka_index = 0;
static u32 s_power_index = 0;

void HDAC_vInit(const GPIOx_PinConfig_t *A_xPins, u8 A_u8PinsNo)
{
    if (A_xPins == NULL || A_u8PinsNo == 0) {
        return;
    }

    s_dac_pins_num = (A_u8PinsNo > DAC_PIN_COUNT) ? DAC_PIN_COUNT : A_u8PinsNo;
    for (u8 i = 0; i < s_dac_pins_num; i++) {
        s_dac_pins[i] = A_xPins[i];
        MGPIO_vPinInit(&s_dac_pins[i]);
    }

    s_dac_ready = 1;
    s_waka_index = 0;
    s_power_index = 0;

    /* Park the DAC at mid-rail so the speaker stays silent before playback. */
    HDAC_vOutputByte(DAC_SILENCE_LEVEL);
}

void HDAC_vOutputByte(u8 A_u8Sample)
{
    for (u8 i = 0; i < s_dac_pins_num; i++) {
        /* Bit i drives DAC pin i: the configured order must be LSB-first. */
        MGPIO_vSetPinValue(s_dac_pins[i].Port, s_dac_pins[i].Pin,
                           (A_u8Sample >> i) & 1U);
    }
}

void HDAC_vSendSample(const u8 *A_u8Ptr, u32 A_u32Index)
{
    if (A_u8Ptr != NULL) {
        HDAC_vOutputByte(A_u8Ptr[A_u32Index]);
    }
}

void HDAC_vPlaySoundSync(const u8 *A_u8Samples, u32 A_u32Length)
{
    if (A_u8Samples == NULL || A_u32Length == 0) {
        return;
    }

    for (u32 i = 0; i < A_u32Length; i++) {
        HDAC_vOutputByte(A_u8Samples[i]);
        dac_delay_sample();
    }

    /* Return to idle silence. */
    HDAC_vOutputByte(DAC_SILENCE_LEVEL);
}

static void dac_stream(const u8 *A_u8Samples, u32 A_u32Length, u32 *A_u32Index,
                       u16 A_u16SampleCount)
{
    for (u16 s = 0; s < A_u16SampleCount; s++) {
        HDAC_vOutputByte(A_u8Samples[*A_u32Index]);
        (*A_u32Index)++;
        if (*A_u32Index >= A_u32Length) {
            *A_u32Index = 0;
        }
        dac_delay_sample();
    }
}

void HDAC_vPlayStart(void)
{
    HDAC_vPlaySoundSync(AUDIO_START, AUDIO_START_LEN);
}

void HDAC_vPlayDeath(void)
{
    HDAC_vPlaySoundSync(AUDIO_DEATH, AUDIO_DEATH_LEN);
}

void HDAC_vPlayPowerPellet(void)
{
    s_power_index = 0;
    HDAC_vPlaySoundSync(AUDIO_POWER_PELLET, AUDIO_POWER_PELLET_LEN);
}

void HDAC_vStopStreaming(void)
{
    s_waka_index = 0;
    s_power_index = 0;
    HDAC_vOutputByte(DAC_SILENCE_LEVEL);
}

void HDAC_vTickAudio(u8 pacman_moving, u8 frightened_mode)
{
    /* One game tick is 50 ms. At AUDIO_SAMPLE_RATE there are 200 samples/tick,
     * so streaming this many samples keeps the clip in real time. */
    const u16 SAMPLES_PER_TICK = (u16)(AUDIO_SAMPLE_RATE / 20U);

    if (!s_dac_ready) {
        return;
    }

    if (frightened_mode) {
        /* Power-pellet siren loops for the whole (10 s) frightened window. */
        dac_stream(AUDIO_POWER_PELLET, AUDIO_POWER_PELLET_LEN, &s_power_index,
                   SAMPLES_PER_TICK);
    } else if (pacman_moving) {
        /* Waka-waka loops while Pac-Man is actually moving. */
        dac_stream(AUDIO_WAKA, AUDIO_WAKA_LEN, &s_waka_index, SAMPLES_PER_TICK);
    } else {
        /* Standing still: hold the DAC at mid-rail for the whole tick. */
        HDAC_vOutputByte(DAC_SILENCE_LEVEL);
        for (u16 s = 0; s < SAMPLES_PER_TICK; s++) {
            dac_delay_sample();
        }
    }
}