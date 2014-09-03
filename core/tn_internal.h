
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
 * Remove all tasks from wait queue, returning the TERR_DLT code.
 * Note: this function might sleep.
 */
void _tn_wait_queue_notify_deleted(struct TN_ListItem *wait_queue, TN_INTSAVE_DATA_ARG_DEC);


/**
 * Set flags by bitmask.
 * Given flags value will be OR-ed with existing flags.
 *
 * @return previous tn_sys_state value.
 */
enum TN_StateFlag _tn_sys_state_flags_set(enum TN_StateFlag flags);

/**
 * Clear flags by bitmask
 * Given flags value will be inverted and AND-ed with existing flags.
 *
 * @return previous tn_sys_state value.
 */
enum TN_StateFlag _tn_sys_state_flags_clear(enum TN_StateFlag flags);

/**
 * If user-callback function for events is set, call it;
 * otherwise do nothing
 */
void _tn_event_callback_call(enum TN_StateFlag flags, BOOL set);




//-- tn_tasks.c

enum TN_Retval  _tn_task_create(struct TN_Task *task,                 //-- task TCB
      void (*task_func)(void *param),  //-- task function
      int priority,                    //-- task priority
      unsigned int *task_stack_bottom, //-- task stack first addr in memory (bottom)
      int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
      void *param,                     //-- task function parameter
      int option);                     //-- Creation option

/**
 * Remove task from wait queue and call _tn_task_wait_complete() for it
 */
BOOL _tn_task_remove_from_wait_queue_and_wake_up(struct TN_Task *task);

/**
 * Should be called when task finishes waiting for anything.
 * Remove task from wait queue, 
 * and make it runnable (if only it isn't suspended)
 *
 * @return TRUE if tn_next_task_to_run is set to given task
 *              (that is, context switch is needed)
 */
BOOL  _tn_task_wait_complete  (struct TN_Task *task);

/**
 * Change task's state to runnable,
 * put it on the 'ready queue' for its priority,
 *
 * if priority is higher than tn_next_task_to_run's priority,
 * then set tn_next_task_to_run to this task and return TRUE,
 * otherwise return FALSE.
 *
 * @return TRUE if given task should run next
 */
BOOL _tn_task_to_runnable(struct TN_Task *task);


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

/**
 * Change priority of any task (runnable or non-runnable)
 */
BOOL _tn_change_task_priority(struct TN_Task *task, int new_priority);

/**
 * When changing priority of the runnable task, this function 
 * should be called instead of plain assignment.
 *
 * For non-runnable tasks, this function should never be called.
 *
 * Remove current task from ready queue for its current priority,
 * change its priority, add to the end of ready queue of new priority,
 * find next task to run.
 */
BOOL  _tn_change_running_task_priority(struct TN_Task *task, int new_priority);

/**
 * Check if mutex is locked by task.
 *
 * @return TRUE if mutex is locked, FALSE otherwise.
 */
BOOL _tn_is_mutex_locked_by_task(struct TN_Task *task, struct TN_Mutex *mutex);

//-- tn_mutex.h

/**
 *    * Unconditionally set lock count to 0. This is needed because mutex
 *      might be deleted 'unexpectedly' when its holder task is deleted
 *    * Remove given mutex from task's locked mutexes list,
 *    * Set new priority of the task
 *      (depending on its base_priority and other locked mutexes),
 *    * If no other tasks want to lock this mutex, set holder to NULL,
 *      otherwise grab first task from the mutex's wait_queue
 *      and lock mutex by this task.
 *
 * @returns TRUE if context switch is needed
 *          (that is, if there is some other task that waited for mutex,
 *          and this task has highest priority now)
 */
BOOL _tn_mutex_do_unlock(struct TN_Mutex *mutex);

#endif // _TN_INTERNAL_H


