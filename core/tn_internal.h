
/*******************************************************************************
 *    Internal TNKernel header
 *
 ******************************************************************************/


#ifndef _TN_INTERNAL_H
#define _TN_INTERNAL_H


/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

//-- tn_tasks.c {{{
int  _tn_task_create(TN_TCB *task,                 //-- task TCB
      void (*task_func)(void *param),  //-- task function
      int priority,                    //-- task priority
      unsigned int *task_stack_bottom, //-- task stack first addr in memory (bottom)
      int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
      void *param,                     //-- task function parameter
      int option);                     //-- Creation option

int   _tn_task_wait_complete  (TN_TCB *task);
void  _tn_task_to_runnable    (TN_TCB *task);
void  _tn_task_to_non_runnable(TN_TCB *task);

void _tn_task_curr_to_wait_action(
      CDLL_QUEUE *wait_que,
      int wait_reason,
      unsigned long timeout);
int  _tn_change_running_task_priority(TN_TCB * task, int new_priority);

#ifdef TN_USE_MUTEXES
void _tn_set_current_priority(TN_TCB * task, int priority);
#endif
// }}}

#endif // _TN_INTERNAL_H


