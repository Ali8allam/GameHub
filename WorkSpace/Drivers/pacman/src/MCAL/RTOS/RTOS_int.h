/*
 * RTOS_int.h
 *
 *  Created on: Aug 26, 2026
 *      Author: Mohamed
 */

#ifndef MCAL_RTOS_RTOS_INT_H_
#define MCAL_RTOS_RTOS_INT_H_
void RTOS_vStart(void);
u8 RTOS_vCreateTask(void (*TaskFunction)(void), u32 Periodicity, u8  Priority);
void RTOS_vScheduler(void);
void RTOS_vDeleteTask(u8 Priority);
u8 RTOS_ResumeTask(u8  Priority);
u8 RTOS_SuspendTask(u8  Priority);
u8 RTOS_DeleteTask(u8  Priority);
#endif /* MCAL_RTOS_RTOS_INT_H_ */
