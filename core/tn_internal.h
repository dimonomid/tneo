/*******************************************************************************
 *
 * TNeoKernel: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeoKernel:                copyright © 2014       Dmitry Frank.
 *
 *    TNeoKernel was born as a thorough review and re-implementation 
 *    of TNKernel. New kernel has well-formed code, bugs of ancestor are fixed
 *    as well as new features added, and it is tested carefully with unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeoKernel.
 *
 *    Permission to use, copy, modify, and distribute this software in source
 *    and binary forms and its documentation for any purpose and without fee
 *    is hereby granted, provided that the above copyright notice appear
 *    in all copies and that both that copyright notice and this permission
 *    notice appear in supporting documentation.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE DMITRY FRANK AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DMITRY FRANK OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *    THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/**
 * \file
 * 
 *    Internal TNeoKernel header
 *
 */


#ifndef _TN_INTERNAL_H
#define _TN_INTERNAL_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_arch.h"

#include "tn_tasks.h"   //-- for inline functions
#include "tn_sys.h"     //-- for inline functions
//struct TN_Task;

struct TN_Mutex;
struct TN_ListItem;



/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/// list of all ready to run (TN_TASK_STATE_RUNNABLE) tasks
extern struct TN_ListItem tn_ready_list[TN_NUM_PRIORITY];

/// list all created tasks (now it is used for statictic only)
extern struct TN_ListItem tn_create_queue;

/// count of created tasks
extern volatile int tn_created_tasks_cnt;           

/// list of all tasks that wait timeout expiration
extern struct TN_ListItem tn_wait_timeout_list;             

/// system state flags
extern volatile enum TN_StateFlag tn_sys_state;

/// task that runs now
extern struct TN_Task * tn_curr_run_task;

/// task that should run as soon as possible (after context switch)
extern struct TN_Task * tn_next_task_to_run;

/// bitmask of priorities with runnable tasks.
/// lowest priority bit (1 << (TN_NUM_PRIORITY - 1)) should always be set,
/// since this priority is used by idle task and it is always runnable.
extern volatile unsigned int tn_ready_to_run_bmp;

/// system time that is get/set by `tn_sys_time_get()`/`tn_sys_time_set()`,
/// respectively.
///
/// NOTE that these and only these TNeoKernel functions use this counter,
/// it is not used for internal timeout calculation, or anything.
/// Its usage completely depends on user.
extern volatile unsigned int tn_sys_time_count;

/// current interrupt nesting count. Used by macros
/// `tn_soft_isr()` and `tn_srs_isr()`.
extern volatile int tn_int_nest_count;

/// saved task stack pointer. Needed when switching stack pointer from 
/// task stack to interrupt stack.
extern void *tn_user_sp;

/// saved ISR stack pointer. Needed when switching stack pointer from
/// interrupt stack to task stack.
extern void *tn_int_sp;



/*******************************************************************************
 *    INTERNAL KERNEL TYPES
 ******************************************************************************/

/* none yet */




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#if !defined(container_of)
/* given a pointer @ptr to the field @member embedded into type (usually
 * struct) @type, return pointer to the embedding instance of @type. */
#define container_of(ptr, type, member) \
   ((type *)((char *)(ptr)-(char *)(&((type *)0)->member)))
#endif




/*******************************************************************************
 *    tn_sys.c
 ******************************************************************************/

/**
 * Remove all tasks from wait queue, returning the TN_RC_DELETED code.
 * Note: this function might sleep.
 */
void _tn_wait_queue_notify_deleted(struct TN_ListItem *wait_queue);


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
      _tn_arch_context_switch();
   }
}





/*******************************************************************************
 *    tn_tasks.c
 ******************************************************************************/

static inline BOOL _tn_task_is_runnable(struct TN_Task *task)
{
   return !!(task->task_state & TN_TASK_STATE_RUNNABLE);
}

static inline BOOL _tn_task_is_waiting(struct TN_Task *task)
{
   return !!(task->task_state & TN_TASK_STATE_WAIT);
}

static inline BOOL _tn_task_is_suspended(struct TN_Task *task)
{
   return !!(task->task_state & TN_TASK_STATE_SUSPEND);
}

static inline BOOL _tn_task_is_dormant(struct TN_Task *task)
{
   return !!(task->task_state & TN_TASK_STATE_DORMANT);
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
 */
void _tn_task_clear_waiting(struct TN_Task *task, enum TN_RCode wait_rc);

void _tn_task_set_suspended(struct TN_Task *task);
void _tn_task_clear_suspended(struct TN_Task *task);

void _tn_task_set_dormant(struct TN_Task* task);

void _tn_task_clear_dormant(struct TN_Task *task);

enum TN_RCode _tn_task_activate(struct TN_Task *task);

/**
 * Should be called when task finishes waiting for anything.
 *
 * @param wait_rc return code that will be returned to waiting task
 */
static inline void _tn_task_wait_complete(struct TN_Task *task, enum TN_RCode wait_rc)
{
   _tn_task_clear_waiting(task, wait_rc);

   //-- if task isn't suspended, make it runnable
   if (!_tn_task_is_suspended(task)){
      _tn_task_set_runnable(task);
   }

}


/**
 * calls _tn_task_clear_runnable() for current task, i.e. tn_curr_run_task
 * Set task state to TN_TASK_STATE_WAIT, set given wait_reason and timeout.
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


