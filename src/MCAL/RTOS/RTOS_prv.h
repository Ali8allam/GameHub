/*
 * RTOS_prv.h
 *
 *  Created on: Aug 26, 2026
 *      Author: Mohamed
 */

#ifndef MCAL_RTOS_RTOS_PRV_H_
#define MCAL_RTOS_RTOS_PRV_H_

typedef enum{
	READY = 1,
	RUNNING,
	SUSPENDED
}TASK_States_t;

typedef struct{
	void (*TaskFunction)(void);
	u32 Periodicity;
	u32 state
}Task_t;

#define RESERVED_PRIORITY_ERROR   				1
#define OUT_OF_SYSTASK_RANGE_ERROR  			2


#endif /* MCAL_RTOS_RTOS_PRV_H_ */
