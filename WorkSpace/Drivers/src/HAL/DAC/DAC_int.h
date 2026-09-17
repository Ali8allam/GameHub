/*
 * DAC_int.h
 *
 * 8-bit resistor-ladder (R-2R) DAC driven by bit-banged GPIO.
 *
 * The driver is deliberately content-agnostic: it knows how to hold a sample
 * on the ladder for exactly one sample period and nothing about what is being
 * played. Game-specific sound effects live with the game that owns them.
 *
 * Every clip handed to this driver must be unsigned 8-bit PCM encoded at
 * HDAC_SAMPLE_RATE_HZ (see DAC_cfg.h). 0x80 is mid-rail / silence.
 *
 * Playback is blocking and paced by a calibrated busy-wait, not by SysTick -
 * SysTick belongs to the IR receiver. See DAC_cfg.h for the full rationale.
 */

#ifndef HAL_DAC_DAC_INT_H_
#define HAL_DAC_DAC_INT_H_

#include "../../LIB/STD_TYPES.h"
#include "../../MCAL/GPIO/GPIO_int.h"

/* Mid-rail code: what the ladder is parked at when nothing is playing. */
#define HDAC_SILENCE_LEVEL      128U

/* Configure the ladder.
 * A_xPins lists the data pins ordered LSB -> MSB: bit 0 of every sample drives
 * A_xPins[0]. Pass HDAC_PINS / HDAC_PIN_COUNT from DAC_cfg.h for this board.
 * The RCC clock of the pins' port must already be enabled by the caller.
 * Leaves the DAC parked at silence. */
void HDAC_vInit(const GPIOx_PinConfig_t *A_xPins, u8 A_u8PinsNo);

/* Drive one raw code onto the ladder and return immediately (no delay). */
void HDAC_vOutputByte(u8 A_u8Sample);

/* Drive A_u8Ptr[A_u32Index] onto the ladder (no delay). */
void HDAC_vSendSample(const u8 *A_u8Ptr, u32 A_u32Index);

/* Park the ladder at mid-rail. */
void HDAC_vSilence(void);

/* 1 once HDAC_vInit() has run, 0 otherwise. */
u8 HDAC_u8IsReady(void);

/* Play a whole clip. Blocks for A_u32Length / HDAC_SAMPLE_RATE_HZ seconds and
 * parks the ladder at silence afterwards. */
void HDAC_vPlaySoundSync(const u8 *A_u8Samples, u32 A_u32Length);

/* Play exactly A_u16SampleCount samples of a clip, resuming from
 * *A_pu32Index and wrapping back to 0 at the end of the clip. Blocks for
 * A_u16SampleCount sample periods. Lets a caller spread one looping clip
 * across many fixed-length ticks without ever stalling longer than a tick. */
void HDAC_vStreamSamples(const u8 *A_u8Samples, u32 A_u32Length,
                         u32 *A_pu32Index, u16 A_u16SampleCount);

/* Hold mid-rail for A_u16SampleCount sample periods. The silent counterpart
 * of HDAC_vStreamSamples(), so a caller's tick length stays constant whether
 * or not something is playing. */
void HDAC_vHoldSilence(u16 A_u16SampleCount);

#endif /* HAL_DAC_DAC_INT_H_ */
