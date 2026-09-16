#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"
#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/AFIO/AFIO_int.h"
#include "../../MCAL/EXTI/EXTI_int.h"
#include "../../MCAL/NVIC/NVIC_int.h"
#include "../../MCAL/SYSTICK/SYSTICK_int.h"
#include "IR_int.h"

#define IR_PORT GPIO_PORTB
#define IR_PIN GPIO_PIN0
#define IR_CAPTURE_COUNT 50

static volatile u8 ir_starting;
static volatile u8 ir_count;
static volatile u8 ir_command_ready;
static volatile u8 ir_command;
static volatile u32 ir_timings[IR_CAPTURE_COUNT];

static void ir_reset_capture(void)
{
    ir_starting = 0;
    ir_count = 0;
    for (u8 index = 0; index < IR_CAPTURE_COUNT; index++) {
        ir_timings[index] = 0;
    }
}

static void ir_decode(void)
{
    u8 decoded_command = 0;

    for (u8 bit = 0; bit < 8; bit++) {
        u32 timing = ir_timings[17 + bit];

        if (timing >= 1000 && timing <= 1250) {
            CLR_BIT(decoded_command, bit);
        } else if (timing >= 2000 && timing <= 2450) {
            SET_BIT(decoded_command, bit);
        } else {
            ir_reset_capture();
            return;
        }
    }

    ir_command = decoded_command;
    ir_command_ready = 1;
    ir_reset_capture();
}

static void ir_edge_callback(void)
{
    if (ir_starting == 0) {
        ir_starting = 1;
        ir_count = 0;
        MSYSTICK_vSetIntervalSingle(15, ir_decode);
    } else {
        if (ir_count < IR_CAPTURE_COUNT) {
            ir_timings[ir_count++] =
                MSYSTICK_u32GetElapsedTime_SingleShot();
        }
        MSYSTICK_vSetIntervalSingle(4, ir_decode);
    }
}

void HIR_vInit(void)
{
    GPIOx_PinConfig_t ir_pin = {
        IR_PORT,
        IR_PIN,
        GPIO_MODE_INPUT,
        GPIO_OT_PUSHPULL,
        GPIO_SPEED_2MHZ,
        GPIO_PULL_UP,
        0
    };

    MGPIO_vPinInit(&ir_pin);
    MAFIO_vSetEXTIPort(EXTI_LINE0, AFIO_PORTB);
    MEXTI_vInit();
    MEXTI_vSetTrigger(EXTI_LINE0, EXTI_FALLING_EDGE);
    MEXTI_vSetCallBack(ir_edge_callback, EXTI_LINE0);
    MEXTI_vEnableINT(EXTI_LINE0);
    MNVIC_vEnable_Peripheral_INT(NVIC_EXTI0_IRQ);
}

u8 HIR_u8GetCommand(u8 *command)
{
    if (!ir_command_ready) {
        return 0;
    }

    *command = ir_command;
    ir_command_ready = 0;
    return 1;
}
