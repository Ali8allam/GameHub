/*
 * DAC_prv.h
 *
 * Private hardware configuration for the 8-bit resistor-ladder (R-2R) DAC
 * driven by the STM32F103 GPIO port B.
 *
 * Pin map (must stay in sync with the other peripherals on this board):
 *   PA5 / PA6 / PA7  ST7735 SPI1 (SCK / MISO / MOSI)
 *   PA4 / PA8        ST7735 CS / DC
 *   PB0              IR receiver
 *   PB1 / PB10       ST7735 RST / backlight
 *
 * The DAC therefore uses PB3..PB9 and PB11, ordered DAC bit 0 (LSB) -> bit 7
 * (MSB). PB3 and PB4 are JTAG pins on the STM32F103 (JTDO / NJTRST); they are
 * released for GPIO use by remapping the SWD-only debug mode in AFIO before the
 * pins are configured as outputs.
 */

#ifndef HAL_DAC_PRV_H_
#define HAL_DAC_PRV_H_

#include "../../LIB/STD_TYPES.h"
#include "../../MCAL/GPIO/GPIO_int.h"

#define HDAC_PIN_COUNT  8U

/* DAC bit 0 = LSB ... DAC bit 7 = MSB. */
#define HDAC_PINS                                                       \
    {                                                                   \
        {GPIO_PORTB, GPIO_PIN3, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,     \
         GPIO_SPEED_50MHZ, GPIO_NO_PULL, 0},                            \
        {GPIO_PORTB, GPIO_PIN4, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,     \
         GPIO_SPEED_50MHZ, GPIO_NO_PULL, 0},                            \
        {GPIO_PORTB, GPIO_PIN5, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,     \
         GPIO_SPEED_50MHZ, GPIO_NO_PULL, 0},                            \
        {GPIO_PORTB, GPIO_PIN6, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,     \
         GPIO_SPEED_50MHZ, GPIO_NO_PULL, 0},                            \
        {GPIO_PORTB, GPIO_PIN7, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,     \
         GPIO_SPEED_50MHZ, GPIO_NO_PULL, 0},                            \
        {GPIO_PORTB, GPIO_PIN8, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,     \
         GPIO_SPEED_50MHZ, GPIO_NO_PULL, 0},                            \
        {GPIO_PORTB, GPIO_PIN9, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,     \
         GPIO_SPEED_50MHZ, GPIO_NO_PULL, 0},                            \
        {GPIO_PORTB, GPIO_PIN11, GPIO_MODE_OUTPUT, GPIO_OT_PUSHPULL,    \
         GPIO_SPEED_50MHZ, GPIO_NO_PULL, 0}                             \
    }

#endif /* HAL_DAC_PRV_H_ */
