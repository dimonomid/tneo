
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

#include "tn_tasks.h"   //-- for inline functions
#include "tn_sys.h"     //-- for inline functions
//struct TN_Task;

struct TN_Mutex;
struct TN_ListItem;



/*******************************************************************************
 *    INTERNAL TNKERNEL TYPES
 ******************************************************************************/

//-- tn_tasks.c

/**
 * Flags for _tn_task_wait_complete()
 * TODO: when events, dqueue, sem will be refactored, get rid of
 * these flags completely
 * (like, flag is always set)
 */
enum TN_WComplFlags {
   TN_WCOMPL__REMOVE_WQUEUE   = (1 << 0), //-- if set, task will be removed from wait_queue
};




/*******************************************************************************
 *    tn_sys.c
 ******************************************************************************/

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

#if TN_MUTEX_DEADLOCK_DETECT
void _tn_cry_deadlock(BOOL active, struct TN_Mutex *mutex, struct TN_Task *task);
#endif

static inline BOOL _tn_need_context_switch(void)
{
   return (tn_curr_run_task != tn_next_task_to_run);
}

static inline void _tn_switch_context_if_needed(void)
{
   if (_tn_need_context_switch()){
      tn_switch_context();
   }
}





/*******************************************************************************
 *    tn_tasks.c
 ******************************************************************************/

enum TN_Retval  _tn_task_create(
      struct TN_Task *task,            //-- task TCB
      void (*task_func)(void *param),  //-- task function
      int priority,                    //-- task priority
      unsigned int *task_stack_bottom, //-- task stack first addr in memory (bottom)
      int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
      void *param,                     //-- task function parameter
      int option                       //-- Creation option
);

static inline BOOL _tn_task_is_runnable(struct TN_Task *task)
{
   return !!(task->task_state & TSK_STATE_RUNNABLE);
}

static inline BOOL _tn_task_is_waiting(struct TN_Task *task)
{
   return !!(task->task_state & TSK_STATE_WAIT);
}

static inline BOOL _tn_task_is_suspended(struct TN_Task *task)
{
   return !!(task->task_state & TSK_STATE_SUSPEND);
}

static inline BOOL _tn_task_is_dormant(struct TN_Task *task)
{
   return !!(task->task_state & TSK_STATE_DORMANT);
}

/**
 * Should be called when task_state is NONE.
 *
 * Set RUNNABLE bit in task_state,
 * put task on the 'ready queue' for its priority,
 *
 * if priority is higher than tn_next_task_to_run's priority,
 * then set tn_next_task_to_run to this task and return TRUE,
 * otherwise return FALSE.
 */
void _tn_task_set_runnable(struct TN_Task *task);


/**
 * Should be called when task_state has just single RUNNABLE bit set.
 *
 * Clear RUNNABLE bit, remove task from 'ready queue', determine and set
 * new tn_next_task_to_run.
 */
void _tn_task_clear_runnable(struct TN_Task *task);

void _tn_task_set_waiting(
      struct TN_Task *task,
      struct TN_ListItem *wait_que,
      enum TN_WaitReason wait_reason,
      unsigned long timeout
      );

/**
 * @param wait_rc return code that will be returned to waiting task
 * @param flags   see enum TN_WComplFlags
 */
void _tn_task_clear_waiting(struct TN_Task *task, enum TN_Retval wait_rc, enum TN_WComplFlags flags);

void _tn_task_set_suspended(struct TN_Task *task);
void _tn_task_clear_suspended(struct TN_Task *task);

void _tn_task_set_dormant(struct TN_Task* task);

void _tn_task_clear_dormant(struct TN_Task *task);

/**
 * Should be called when task finishes waiting for anything.
 *
 * @param wait_rc return code that will be returned to waiting task
 * @param flags   see enum TN_WComplFlags
 */
static inline void _tn_task_wait_complete(struct TN_Task *task, enum TN_Retval wait_rc, enum TN_WComplFlags flags)
{
   _tn_task_clear_waiting(task, wait_rc, flags);

   //-- if task isn't suspended, make it runnable
   if (!_tn_task_is_suspended(task)){
      _tn_task_set_runnable(task);
   }

}


/**
 * calls _tn_task_clear_runnable() for current task, i.e. tn_curr_run_task
 * Set task state to TSK_STATE_WAIT, set given wait_reason and timeout.
 *
 * If non-NULL wait_que is provided, then add task to it; otherwise reset task's task_queue.
 * If timeout is not TN_WAIT_INFINITE, add task to tn_wait_timeout_list
 */
static inline void _tn_task_curr_to_wait_action(
      struct TN_ListItem *wait_que,
      enum TN_WaitReason wait_reason,
      unsigned long timeout
      )
{
   _tn_task_clear_runnable(tn_curr_run_task);
   _tn_task_set_waiting(tn_curr_run_task, wait_que, wait_reason, timeout);
}


/**
 * Change priority of any task (runnable or non-runnable)
 */
void _tn_change_task_priority(struct TN_Task *task, int new_priority);

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
void  _tn_change_running_task_priority(struct TN_Task *task, int new_priority);

#if TN_USE_MUTEXES
/**
 * Check if mutex is locked by task.
 *
 * @return TRUE if mutex is locked, FALSE otherwise.
 */
BOOL _tn_is_mutex_locked_by_task(struct TN_Task *task, struct TN_Mutex *mutex);
#endif



/*******************************************************************************
 *    tn_mutex.c
 ******************************************************************************/

#if TN_USE_MUTEXES
/**
 * Unlock all mutexes locked by the task
 */
void _tn_mutex_unlock_all_by_task(struct TN_Task *task);

/**
 * Should be called when task finishes waiting
 * for mutex with priority inheritance
 */
void _tn_mutex_i_on_task_wait_complete(struct TN_Task *task);

/**
 * Should be called when task winishes waiting
 * for any mutex (no matter which algorithm it uses)
 */
void _tn_mutex_on_task_wait_complete(struct TN_Task *task);

#else
static inline void _tn_mutex_unlock_all_by_task(struct TN_Task *task) {}
static inline void _tn_mutex_i_on_task_wait_complete(struct TN_Task *task) {}
static inline void _tn_mutex_on_task_wait_complete(struct TN_Task *task) {}
#endif

#endif // _TN_INTERNAL_H


