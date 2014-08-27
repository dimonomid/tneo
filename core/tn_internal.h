
/*******************************************************************************
 *    Internal TNKernel header
 *
 ******************************************************************************/


#ifndef _TN_INTERNAL_H
#define _TN_INTERNAL_H


/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

int _tn_task_create(TN_TCB * task,                //-- task TCB
                 void (*task_func)(void *param),  //-- task function
                 int priority,                    //-- task priority
                 unsigned int *task_stack_bottom, //-- task stack first addr in memory (bottom)
                 int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
                 void *param,                     //-- task function parameter
                 int option);                     //-- Creation option



#endif // _TN_INTERNAL_H


