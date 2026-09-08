#include "../LIB/STD_TYPES.h"
#include "../LIB/BIT_MATH.h"

#include "../MCAL/SYSTICK/SYSTICK_int.h"

#include "RTOS_int.h"
#include "RTOS_prv.h"
#include "RTOS_cfg.h"

Task_t SystemTasks[MAX_SYSTASK_SIZE] = {{NULL}};
u32 TimingArray[MAX_SYSTASK_SIZE]    = {0};

void OS_vStart(void)
{
    MSYSTICK_Config_t STK_cfg = {
        .InterruptEnable = INT_ENABLE,
        .CLK_SRC          = CLK_SRC_AHB_8
    };

    MSYSTICK_vInit(&STK_cfg);


    MSYSTICK_vSetIntervalMulti(TICKTIME, RTOS_vScheduler);
}

u8 RTOS_vCreateTask(void (*TaskFunction)(void), u32 Periodicity, u8 Priority)
{
    u8 L_u8ErrorState = 0;

    if (Priority < MAX_SYSTASK_SIZE)
    {
        if (SystemTasks[Priority].TaskFunction == NULL)
        {
            SystemTasks[Priority].TaskFunction = TaskFunction;
            SystemTasks[Priority].Periodicity  = Periodicity;
            SystemTasks[Priority].state        = READY;


            TimingArray[Priority] = 0;
        }
        else
        {
            L_u8ErrorState = RESERVED_PRIORITY_ERROR;
        }
    }
    else
    {
        L_u8ErrorState = OUT_OF_SYSTASK_RANGE_ERROR;
    }

    return L_u8ErrorState; 
}

void RTOS_vScheduler(void)
{
    for (u8 counter = 0; counter < MAX_SYSTASK_SIZE; counter++)
    {

        if ((SystemTasks[counter].TaskFunction != NULL) &&
            (SystemTasks[counter].state == READY))
        {
            if (TimingArray[counter] == 0)
            {

                SystemTasks[counter].TaskFunction();


                TimingArray[counter] = SystemTasks[counter].Periodicity - 1;
            }
            else
            {
                TimingArray[counter]--;
            }
        }
    }
}

u8 RTOS_DeleteTask(u8 Priority)
{
    u8 L_u8ErrorState = 0;

    if (Priority < MAX_SYSTASK_SIZE)
    {
        if (SystemTasks[Priority].TaskFunction != NULL)
        {
            SystemTasks[Priority].TaskFunction = NULL;
            SystemTasks[Priority].Periodicity  = 0;
            SystemTasks[Priority].state        = SUSPENDED;
            TimingArray[Priority]              = 0;
        }

    }
    else
    {
        L_u8ErrorState = OUT_OF_SYSTASK_RANGE_ERROR;
    }

    return L_u8ErrorState;
}

u8 RTOS_ResumeTask(u8 Priority)
{
    u8 L_u8ErrorState = 0;

    if (Priority < MAX_SYSTASK_SIZE)
    {
        if (SystemTasks[Priority].TaskFunction != NULL)
        {
            if (SystemTasks[Priority].state == SUSPENDED)
            {
                SystemTasks[Priority].state = READY;
            }
        }

    }
    else
    {
        L_u8ErrorState = OUT_OF_SYSTASK_RANGE_ERROR;
    }

    return L_u8ErrorState;
}


u8 RTOS_SuspendTask(u8 Priority)
{
    u8 L_u8ErrorState = 0;

    if (Priority < MAX_SYSTASK_SIZE)
    {
        if (SystemTasks[Priority].TaskFunction != NULL)
        {
            if (SystemTasks[Priority].state == READY)
            {
                SystemTasks[Priority].state = SUSPENDED;
            }
        }

    }
    else
    {
        L_u8ErrorState = OUT_OF_SYSTASK_RANGE_ERROR;
    }

    return L_u8ErrorState;
}
