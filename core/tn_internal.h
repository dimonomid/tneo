
/*******************************************************************************
 *    Internal TNKernel header
 *
 ******************************************************************************/


#ifndef _TN_INTERNAL_H
#define _TN_INTERNAL_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_port.h"

struct TN_Task;
struct TN_Mutex;
struct TN_ListItem;

/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

//-- system
/**
 * Note: this function sleeps if there is at least one element in wait_queue.
 */
void _tn_wait_queue_notify_deleted(struct TN_ListItem *wait_queue, TN_INTSAVE_DATA_ARG_DEC);


//-- tn_tasks.c
enum TN_Retval  _tn_task_create(struct TN_Task *task,                 //-- task TCB
      void (*task_func)(void *param),  //-- task function
      int priority,                    //-- task priority
      unsigned int *task_stack_bottom, //-- task stack first addr in memory (bottom)
      int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
      void *param,                     //-- task function parameter
      int option);                     //-- Creation option

BOOL  _tn_task_wait_complete  (struct TN_Task *task);
void  _tn_task_to_runnable    (struct TN_Task *task);
void  _tn_task_to_non_runnable(struct TN_Task *task);

void _tn_task_curr_to_wait_action(
      struct TN_ListItem *wait_que,
      enum TN_WaitReason wait_reason,
      unsigned long timeout);
enum TN_Retval  _tn_change_running_task_priority(struct TN_Task *task, int new_priority);


//-- tn_mutex.h

/**
 *    * Remove given mutex from task's locked mutexes list,
 *    * Set new priority of the task
 *      (depending on its base_priority and other locked mutexes),
 *    * If no other tasks want to lock this mutex, set holder to NULL,
 *      otherwise grab first task from the mutex's wait_queue
 *      and lock mutex by this task.
 *
 * Currently it always returns TRUE.
 */
BOOL _tn_do_unlock_mutex(struct TN_Mutex *mutex);

#endif // _TN_INTERNAL_H


