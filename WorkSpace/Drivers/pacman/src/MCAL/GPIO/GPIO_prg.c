/*
 * GPIO_prg.c
 *
 *  Created on: Aug 17, 2025
 *      Author: Nada Mamdouh
 *      Version: 0.2
 */
#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "GPIO_int.h"
#include "GPIO_prv.h"
#include "GPIO_cfg.h"
#include "../RCC/RCC_prv.h"
#include "../AFIO/AFIO_prv.h"

/* Reserved SWD pins. PA13 (SWDIO), PA14 (SWCLK) and PA15 (JTDI) are never
 * released because the on-chip debugger needs them. */
#define GPIO_PIN_RESERVED_PORT(PORT, PIN)                       \
    ((PORT) == GPIO_PORTA &&                                    \
     ((PIN) == 13U || (PIN) == 14U || (PIN) == 15U))

/* PB3 (JTDO) and PB4 (NJTRST) are JTAG-only pins. They stay forbidden until the
 * application remaps AFIO_MAPR to the SWD-only debug configuration, which is
 * what frees them for general GPIO use. MGPIO_vReleaseJTAGPins() performs that
 * remap; after it returns, PB3/PB4 behave like any other output.
 *
 * Without this, the DAC on PB3/PB4 would lose its two least-significant bits. */
static u8 g_jtag_pins_released = 0;

#define GPIO_PIN_FORBIDDEN(PORT, PIN)                                    \
    (GPIO_PIN_RESERVED_PORT(PORT, PIN) ||                                \
     (!g_jtag_pins_released && (PORT) == GPIO_PORTB &&                   \
      ((PIN) == 3U || (PIN) == 4U)))

/* Helper macros for STM32F1 CRL/CRH indexing */
#define GET_CR_SHIFT(PIN) ((PIN % 8) * 4)

void MGPIO_vReleaseJTAGPins(void)
{
    /* AFIO_MAPR SWJ_CFG[2:0] = 0b010 selects the SWD-only debug map, which
     * releases JTDO (PB3) and NJTRST (PB4) for GPIO use. AFIO must be clocked
     * for this write to take effect. */
    RCC->APB2ENR |= (1U << 0); /* AFIOEN */
    AFIO->MAPR = (AFIO->MAPR & ~(0b111U << 24)) | (0b010U << 24);
    g_jtag_pins_released = 1;
}

void MGPIO_vSetMode(u8 A_u8PortID, u8 A_u8PinNum, u8 A_u8Mode)
{
    if (GPIO_PIN_FORBIDDEN(A_u8PortID, A_u8PinNum)) { return; }

    u8 L_u8Shift = GET_CR_SHIFT(A_u8PinNum);
    volatile u32* CR_Reg = NULL;

    switch(A_u8PortID) {
        case GPIO_PORTA: CR_Reg = (A_u8PinNum < 8) ? &(GPIOA->CRL) : &(GPIOA->CRH); break;
        case GPIO_PORTB: CR_Reg = (A_u8PinNum < 8) ? &(GPIOB->CRL) : &(GPIOB->CRH); break;
        case GPIO_PORTC: CR_Reg = (A_u8PinNum < 8) ? &(GPIOC->CRL) : &(GPIOC->CRH); break;
    }

    if(CR_Reg) {
        if(A_u8Mode == GPIO_MODE_INPUT) {
            *CR_Reg &= ~(0b11 << L_u8Shift);
        } else if (A_u8Mode == GPIO_MODE_OUTPUT) {
            *CR_Reg &= ~(0b11 << L_u8Shift);
            *CR_Reg |= (GPIO_SPEED_2MHZ << L_u8Shift);
        }
    }
}

void MGPIO_vSetOutputType(u8 A_u8PortID, u8 A_u8PinNum, u8 A_u8OutputType)
{
    if (GPIO_PIN_FORBIDDEN(A_u8PortID, A_u8PinNum)) { return; }

    u8 L_u8Shift = GET_CR_SHIFT(A_u8PinNum);
    volatile u32* CR_Reg = NULL;

    switch(A_u8PortID) {
        case GPIO_PORTA: CR_Reg = (A_u8PinNum < 8) ? &(GPIOA->CRL) : &(GPIOA->CRH); break;
        case GPIO_PORTB: CR_Reg = (A_u8PinNum < 8) ? &(GPIOB->CRL) : &(GPIOB->CRH); break;
        case GPIO_PORTC: CR_Reg = (A_u8PinNum < 8) ? &(GPIOC->CRL) : &(GPIOC->CRH); break;
    }

    if(CR_Reg) {

        *CR_Reg &= ~(0b1100 << L_u8Shift);

        if(A_u8OutputType == GPIO_OT_PUSHPULL ) {
            *CR_Reg |= (0b0000 << L_u8Shift);
        } else if(A_u8OutputType == GPIO_OT_OPENDRAIN ) {
            *CR_Reg |= (0b0100 << L_u8Shift);
        }
    }
}

void MGPIO_vSetOutputSpeed(u8 A_u8PortID, u8 A_u8PinNum, u8 A_u8OutputSpeed)
{
    if (GPIO_PIN_FORBIDDEN(A_u8PortID, A_u8PinNum)) { return; }

    u8 L_u8Shift = GET_CR_SHIFT(A_u8PinNum);
    volatile u32* CR_Reg = NULL;

    switch(A_u8PortID) {
        case GPIO_PORTA: CR_Reg = (A_u8PinNum < 8) ? &(GPIOA->CRL) : &(GPIOA->CRH); break;
        case GPIO_PORTB: CR_Reg = (A_u8PinNum < 8) ? &(GPIOB->CRL) : &(GPIOB->CRH); break;
        case GPIO_PORTC: CR_Reg = (A_u8PinNum < 8) ? &(GPIOC->CRL) : &(GPIOC->CRH); break;
    }

    if(CR_Reg) {
        *CR_Reg &= ~(0b11 << L_u8Shift);
        *CR_Reg |= (A_u8OutputSpeed << L_u8Shift);
    }
}

void MGPIO_vSetPULLType(u8 A_u8PortID, u8 A_u8PinNum, u8 A_u8PushPull)
{
    if (GPIO_PIN_FORBIDDEN(A_u8PortID, A_u8PinNum)) { return; }

    u8 L_u8Shift = GET_CR_SHIFT(A_u8PinNum);
    volatile u32* CR_Reg = NULL;

    switch(A_u8PortID) {
        case GPIO_PORTA: CR_Reg = (A_u8PinNum < 8) ? &(GPIOA->CRL) : &(GPIOA->CRH); break;
        case GPIO_PORTB: CR_Reg = (A_u8PinNum < 8) ? &(GPIOB->CRL) : &(GPIOB->CRH); break;
        case GPIO_PORTC: CR_Reg = (A_u8PinNum < 8) ? &(GPIOC->CRL) : &(GPIOC->CRH); break;
    }

    if(CR_Reg) {

        *CR_Reg &= ~(0b1111 << L_u8Shift);

        if(A_u8PushPull == GPIO_NO_PULL) {
            *CR_Reg |=  (0b0100 << L_u8Shift);
        } else {
            *CR_Reg |=  (0b1000 << L_u8Shift);


            if(A_u8PushPull == GPIO_PULL_UP) {
                MGPIO_vSetPinValue(A_u8PortID, A_u8PinNum, GPIO_HIGH);
            } else {
                MGPIO_vSetPinValue(A_u8PortID, A_u8PinNum, GPIO_LOW);
            }
        }
    }
}

void MGPIO_vSetPinValue(u8 A_u8PortID, u8 A_u8PinNum, u8 A_u8Value)
{
    if (GPIO_PIN_FORBIDDEN(A_u8PortID, A_u8PinNum)) { return; }

    if (A_u8Value == GPIO_HIGH) {
        switch(A_u8PortID) {
            case GPIO_PORTA: SET_BIT(GPIOA -> ODR, A_u8PinNum); break;
            case GPIO_PORTB: SET_BIT(GPIOB -> ODR, A_u8PinNum); break;
            case GPIO_PORTC: SET_BIT(GPIOC -> ODR, A_u8PinNum); break;
        }
    } else if(A_u8Value == GPIO_LOW) {
        switch(A_u8PortID) {
            case GPIO_PORTA: CLR_BIT(GPIOA -> ODR, A_u8PinNum); break;
            case GPIO_PORTB: CLR_BIT(GPIOB -> ODR, A_u8PinNum); break;
            case GPIO_PORTC: CLR_BIT(GPIOC -> ODR, A_u8PinNum); break;
        }
    }
}

u8 MGPIO_u8GetPinValue(u8 A_u8PortID, u8 A_u8PinNum)
{
    u8 L_u8Value = 0;
    if (GPIO_PIN_FORBIDDEN(A_u8PortID, A_u8PinNum)) { return 0; }

    switch(A_u8PortID) {
        case GPIO_PORTA: L_u8Value = GET_BIT(GPIOA -> IDR, A_u8PinNum); break;
        case GPIO_PORTB: L_u8Value = GET_BIT(GPIOB -> IDR, A_u8PinNum); break;
        case GPIO_PORTC: L_u8Value = GET_BIT(GPIOC -> IDR, A_u8PinNum); break;
    }
    return L_u8Value;
}

void MGPIO_vPinInit(GPIOx_PinConfig_t* A_xPinCfg)
{
    if (GPIO_PIN_FORBIDDEN(A_xPinCfg->Port, A_xPinCfg->Pin)) { return; }

    u8 L_u8Shift = GET_CR_SHIFT(A_xPinCfg->Pin);
    u8 L_u8Config = 0;

    if(A_xPinCfg->Mode == GPIO_MODE_INPUT || A_xPinCfg->Mode == GPIO_MODE_ANALOG)
    {
        if(A_xPinCfg->Mode == GPIO_MODE_ANALOG){
            L_u8Config = 0b0000;
        } else if(A_xPinCfg->PullType == GPIO_NO_PULL) {
            L_u8Config = 0b0100;
        } else {
            L_u8Config = 0b1000;
            if(A_xPinCfg->PullType == GPIO_PULL_UP)
                MGPIO_vSetPinValue(A_xPinCfg->Port, A_xPinCfg->Pin, GPIO_HIGH);
            else
                MGPIO_vSetPinValue(A_xPinCfg->Port, A_xPinCfg->Pin, GPIO_LOW);
        }
    }
    else
    {
        u8 L_u8Speed = A_xPinCfg->Speed;
        if(L_u8Speed == 0) L_u8Speed = GPIO_SPEED_2MHZ;

        if(A_xPinCfg->Mode == GPIO_MODE_OUTPUT) {
            L_u8Config = (A_xPinCfg->OutputType == GPIO_OT_OPENDRAIN) ? (0b0100 | L_u8Speed) : (0b0000 | L_u8Speed);
        } else if(A_xPinCfg->Mode == GPIO_MODE_ALF) {
            L_u8Config = (A_xPinCfg->OutputType == GPIO_OT_OPENDRAIN) ? (0b1100 | L_u8Speed) : (0b1000 | L_u8Speed);
        }
    }

    volatile u32* CR_Reg = NULL;
    switch(A_xPinCfg->Port)
    {
        case GPIO_PORTA: CR_Reg = (A_xPinCfg->Pin < 8) ? &(GPIOA->CRL) : &(GPIOA->CRH); break;
        case GPIO_PORTB: CR_Reg = (A_xPinCfg->Pin < 8) ? &(GPIOB->CRL) : &(GPIOB->CRH); break;
        case GPIO_PORTC: CR_Reg = (A_xPinCfg->Pin < 8) ? &(GPIOC->CRL) : &(GPIOC->CRH); break;
    }

    if(CR_Reg) {
        *CR_Reg &= ~(0b1111 << L_u8Shift);
        *CR_Reg |=  (L_u8Config << L_u8Shift);
    }
}

void MGPIO_TogPinValue(u8 A_u8PortID, u8 A_u8PinNum)
{
    switch(A_u8PortID) {
        case GPIO_PORTA: TOG_BIT(GPIOA -> ODR, A_u8PinNum); break;
        case GPIO_PORTB: TOG_BIT(GPIOB -> ODR, A_u8PinNum); break;
        case GPIO_PORTC: TOG_BIT(GPIOC -> ODR, A_u8PinNum); break;
    }
}

void MGPIO_vSetAlt(u8 A_u8PortID, u8 A_u8PinNum, u8 A_u8AFx)
{
    if (GPIO_PIN_FORBIDDEN(A_u8PortID, A_u8PinNum)) { return; }

    u8 L_u8Shift = GET_CR_SHIFT(A_u8PinNum);
    volatile u32* CR_Reg = NULL;

    switch(A_u8PortID) {
        case GPIO_PORTA: CR_Reg = (A_u8PinNum < 8) ? &(GPIOA->CRL) : &(GPIOA->CRH); break;
        case GPIO_PORTB: CR_Reg = (A_u8PinNum < 8) ? &(GPIOB->CRL) : &(GPIOB->CRH); break;
        case GPIO_PORTC: CR_Reg = (A_u8PinNum < 8) ? &(GPIOC->CRL) : &(GPIOC->CRH); break;
    }

    if(CR_Reg) {
        *CR_Reg &= ~(0b1111 << L_u8Shift);

        *CR_Reg |= (0b1011 << L_u8Shift);
    }
}
