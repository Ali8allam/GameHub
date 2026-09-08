#ifndef RTOS_PRV_H_
#define RTOS_PRV_H_

typedef enum 
{
    READY,
    RUNNING,
    SUSPENDED
}TASK_States_t;

typedef struct{
    void (*TaskFunction)(void);
    u32 Periodicity;
    TASK_States_t state;
}Task_t;


#define RESERVED_PRIORITY_ERROR                1 
#define OUT_OF_SYSTASK_RANGE_ERROR             2


#endif 
