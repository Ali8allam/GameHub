#ifndef RTOS_INT_H_
#define RTOS_INT_H_

void OS_vStart(void);

u8 RTOS_vCreateTask(void (*TaskFunction)(void), u32 Periodicity, u8  Priority);
void RTOS_vScheduler(void);
u8 RTOS_DeleteTask(u8  Priority);
u8 RTOS_ResumeTask(u8  Priority);
u8 RTOS_SuspendTask(u8  Priority);

/* ERROR states meaning */
//  1  -> RESERVED_PRIORITY_ERROR 
//  2  -> OUT_OF_SYSTASK_RANGE_ERROR 

#endif 
