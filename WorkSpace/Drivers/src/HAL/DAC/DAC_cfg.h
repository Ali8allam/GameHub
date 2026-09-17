/*
 * DAC_cfg.h
 *
 * Board configuration for the 8-bit resistor-ladder (R-2R) DAC.
 *
 * ---------------------------------------------------------------------------
 * Pin map
 * ---------------------------------------------------------------------------
 * The DAC needs 8 free push-pull outputs on one port. The pins below are the
 * ones left over once every other peripheral on this board is accounted for:
 *
 *   PA0             IR receiver        (HAL/IR, EXTI line 0)
 *   PA1             TFT A0 / DC        (HAL/TFT)
 *   PA2             TFT RST            (HAL/TFT)
 *   PA5 / PA7       SPI1 SCK / MOSI    (MCAL/SPI, drives the TFT)
 *   PA13/14/15      SWD debug pins     (blocked by MCAL/GPIO)
 *   PB3  / PB4      blocked by MCAL/GPIO (see the forbidden-pin guard in
 *                   GPIO_prg.c - every MGPIO_* call silently ignores them)
 *
 * PB5..PB12 is therefore used, ordered DAC bit 0 (LSB) -> bit 7 (MSB).
 *
 * NOTE for anyone porting the original STM32F103 prototype wiring: that build
 * used PB3..PB9 + PB11. PB3/PB4 cannot be driven through this project's GPIO
 * driver, so the two least-significant DAC bits moved to PB5/PB6 and the rest
 * shifted up by one. Only the two LSB wires need re-seating; everything from
 * PB7 up keeps the same electrical position in the ladder.
 *
 * ---------------------------------------------------------------------------
 * Timing
 * ---------------------------------------------------------------------------
 * Playback is bit-banged and paced by a calibrated busy-wait, NOT by SysTick.
 * SysTick is owned by the IR receiver (HAL/IR/IR_prg.c measures every IR pulse
 * with MSYSTICK_vSetIntervalSingle() + MSYSTICK_u32GetElapsedTime_SingleShot()
 * and decodes from SysTick_Handler). Touching SYSTICK->LOAD/VAL/CTRL from the
 * DAC would corrupt every captured pulse width, so no remote key would decode
 * while audio was playing.
 *
 * HDAC_CPU_CLOCK_HZ must match the real core clock. This project runs the
 * STM32F401 from HSE with no AHB prescaler (MRCC_vInit(), RCC_cfg.h), i.e.
 * 25 MHz - which is also what MSYSTICK_vSetDelay_ms() assumes (25 MHz / 8).
 *
 * HDAC_DELAY_LOOP_CYCLES is the measured cost of one iteration of the DAC's
 * busy-wait body. Retune these two constants (and only these two) if the
 * clock, the optimisation level or the compiler changes. Verify by timing a
 * long clip: a 6400-sample clip at 4 kHz must take about 1.6 s.
 */

#ifndef HAL_DAC_DAC_CFG_H_
#define HAL_DAC_DAC_CFG_H_

#include "../../LIB/STD_TYPES.h"
#include "../../MCAL/GPIO/GPIO_int.h"

/* Number of data lines in the resistor ladder. */
#define HDAC_PIN_COUNT          8U

/* DAC bit 0 = LSB ... DAC bit 7 = MSB. */
#define HDAC_PINS                                                       \
    {                                                                   \
        {GPIO_PORTB, GPIO_PIN5,  GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,    \
         GPIO_SPEED_HIGH, GPIO_NO_PULL, GPIO_AF0},                      \
        {GPIO_PORTB, GPIO_PIN6,  GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,    \
         GPIO_SPEED_HIGH, GPIO_NO_PULL, GPIO_AF0},                      \
        {GPIO_PORTB, GPIO_PIN7,  GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,    \
         GPIO_SPEED_HIGH, GPIO_NO_PULL, GPIO_AF0},                      \
        {GPIO_PORTB, GPIO_PIN8,  GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,    \
         GPIO_SPEED_HIGH, GPIO_NO_PULL, GPIO_AF0},                      \
        {GPIO_PORTB, GPIO_PIN9,  GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,    \
         GPIO_SPEED_HIGH, GPIO_NO_PULL, GPIO_AF0},                      \
        {GPIO_PORTB, GPIO_PIN10, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,    \
         GPIO_SPEED_HIGH, GPIO_NO_PULL, GPIO_AF0},                      \
        {GPIO_PORTB, GPIO_PIN11, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,    \
         GPIO_SPEED_HIGH, GPIO_NO_PULL, GPIO_AF0},                      \
        {GPIO_PORTB, GPIO_PIN12, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,    \
         GPIO_SPEED_HIGH, GPIO_NO_PULL, GPIO_AF0}                       \
    }

/* Sample rate every clip handed to this driver must be encoded at. */
#define HDAC_SAMPLE_RATE_HZ     4000U

/* Core clock the sample-hold busy-wait is calibrated against. */
#define HDAC_CPU_CLOCK_HZ       25000000UL

/* Measured cycles per iteration of the busy-wait body (see DAC_prg.c). */
#define HDAC_DELAY_LOOP_CYCLES  13UL

#endif /* HAL_DAC_DAC_CFG_H_ */
