
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

/**
 * Should be called when task finishes waiting for anything.
 * Remove task from wait queue, 
 * and make it runnable (if only it isn't suspended)
 *
 * If task was waiting for mutex, and mutex holder's priority was changed
 * because of that, then change holder's priority back.
 *
 * @return non-zero if task became runnable, zero otherwise.
 */
BOOL  _tn_task_wait_complete  (struct TN_Task *task);

/**
 * Change task's state to runnable,
 * put it on the 'ready queue' for its priority,
 * if priority is higher than tn_next_task_to_run's priority,
 * then set tn_next_task_to_run to this task.
 */
void  _tn_task_to_runnable    (struct TN_Task *task);


/**
 * Remove task from 'ready queue', determine and set
 * new tn_next_task_to_run.
 */
void  _tn_task_to_non_runnable(struct TN_Task *task);

/**
 * calls _tn_task_to_non_runnable() for current task, i.e. tn_curr_run_task
 * Set task state to TSK_STATE_WAIT, set given wait_reason and timeout.
 *
 * If non-NULL wait_que is provided, then add task to it; otherwise reset task's task_queue.
 * If timeout is not TN_WAIT_INFINITE, add task to tn_wait_timeout_list
 */
void _tn_task_curr_to_wait_action(
      struct TN_ListItem *wait_que,
      enum TN_WaitReason wait_reason,
      unsigned long timeout);
enum TN_Retval  _tn_change_running_task_priority(struct TN_Task *task, int new_priority);


//-- tn_mutex.h

/**
 *    * Unconditionally lock count to 0. This is needed because mutex
 *      might be deleted 'unexpectedly' when its holder task is deleted
 *    * Remove given mutex from task's locked mutexes list,
 *    * Set new priority of the task
 *      (depending on its base_priority and other locked mutexes),
 *    * If no other tasks want to lock this mutex, set holder to NULL,
 *      otherwise grab first task from the mutex's wait_queue
 *      and lock mutex by this task.
 *
 * Currently it always returns TRUE.
 */
BOOL _tn_mutex_do_unlock(struct TN_Mutex *mutex);

#endif // _TN_INTERNAL_H


