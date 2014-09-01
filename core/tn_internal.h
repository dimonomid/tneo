
/*******************************************************************************
 *    Internal TNKernel header
 *
 ******************************************************************************/


#ifndef _TN_INTERNAL_H
#define _TN_INTERNAL_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

struct tn_task;
struct tn_mutex;
struct tn_que_head;


/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

//-- system
/**
 * Note: this function sleeps if there is at least one element in wait_queue.
 */
void _tn_wait_queue_notify_deleted(struct tn_que_head *wait_queue, TN_INTSAVE_DATA_ARG_DEC);


//-- tn_tasks.c
int  _tn_task_create(struct tn_task *task,                 //-- task TCB
      void (*task_func)(void *param),  //-- task function
      int priority,                    //-- task priority
      unsigned int *task_stack_bottom, //-- task stack first addr in memory (bottom)
      int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
      void *param,                     //-- task function parameter
      int option);                     //-- Creation option

int   _tn_task_wait_complete  (struct tn_task *task);
void  _tn_task_to_runnable    (struct tn_task *task);
void  _tn_task_to_non_runnable(struct tn_task *task);

void _tn_task_curr_to_wait_action(
      struct tn_que_head *wait_que,
      int wait_reason,
      unsigned long timeout);
int  _tn_change_running_task_priority(struct tn_task *task, int new_priority);

#ifdef TN_USE_MUTEXES
void _tn_set_current_priority(struct tn_task *task, int priority);
#endif


//-- tn_mutex.h
int find_max_blocked_priority(struct tn_mutex *mutex, int ref_priority);
int try_lock_mutex(struct tn_task *task);
int do_unlock_mutex(struct tn_mutex *mutex);

#endif // _TN_INTERNAL_H


