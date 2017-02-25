/*******************************************************************************
 *
 * TNeo: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright 2013, 2014 Anders Montonen.
 *    TNeo:                      copyright 2014       Dmitry Frank.
 *
 *    TNeo was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeo.
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

#ifndef __TN_TASKS_H
#define __TN_TASKS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "_tn_sys.h"
#include "tn_tasks.h"





#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Get pointer to `struct #TN_Task` by pointer to the `task_queue` member
 * of the `struct #TN_Task`.
 */
#define _tn_get_task_by_tsk_queue(que)                                   \
   (que ? container_of(que, struct TN_Task, task_queue) : 0)




/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/

//-- functions for each task state: set, clear, check {{{

//-- runnable {{{

/**
 * Bring task to the $(TN_TASK_STATE_RUNNABLE) state.
 * Should be called when task_state is NONE.
 *
 * Set RUNNABLE bit in task_state,
 * put task on the 'ready queue' for its priority,
 *
 * if priority of given `task` is higher than priority of
 * `_tn_next_task_to_run`, then set `_tn_next_task_to_run` to given `task`.
 */
void _tn_task_set_runnable(struct TN_Task *task);

/**
 * Bring task out from the $(TN_TASK_STATE_RUNNABLE) state.
 * Should be called when task_state has just single RUNNABLE bit set.
 *
 * Clear RUNNABLE bit, remove task from 'ready queue', determine and set
 * new `#_tn_next_task_to_run`.
 */
void _tn_task_clear_runnable(struct TN_Task *task);

/**
 * Returns whether given task is in $(TN_TASK_STATE_RUNNABLE) state.
 */
_TN_STATIC_INLINE TN_BOOL _tn_task_is_runnable(struct TN_Task *task)
{
   return !!(task->task_state & TN_TASK_STATE_RUNNABLE);
}

//}}}

//-- wait {{{

/**
 * Bring task to the $(TN_TASK_STATE_WAIT) state.
 * Should be called when task_state is either NONE or $(TN_TASK_STATE_SUSPEND).
 *
 * @param task
 *    Task to bring to the $(TN_TASK_STATE_WAIT) state
 *
 * @param wait_que
 *    Wait queue to put task in, may be `#TN_NULL`. If not `#TN_NULL`, task is
 *    included in that list by `task_queue` member of `struct #TN_Task`.
 *
 * @param wait_reason
 *    Reason of waiting, see `enum #TN_WaitReason`.
 *
 * @param timeout
 *    If neither `0` nor `#TN_WAIT_INFINITE`, task will be woken up by timer
 *    after specified number of system ticks.
 */
void _tn_task_set_waiting(
      struct TN_Task      *task,
      struct TN_ListItem  *wait_que,
      enum TN_WaitReason   wait_reason,
      TN_TickCnt           timeout
      );

/**
 * Bring task out from the $(TN_TASK_STATE_WAIT) state.
 * Task must be already in the $(TN_TASK_STATE_WAIT) state. It may additionally
 * be in the $(TN_TASK_STATE_SUSPEND) state.
 *
 * @param task
 *    Task to bring out from the $(TN_TASK_STATE_WAIT) state
 *
 * @param wait_rc 
 *    return code that will be returned to waiting task from waited function.
 */
void _tn_task_clear_waiting(struct TN_Task *task, enum TN_RCode wait_rc);

/**
 * Returns whether given task is in $(TN_TASK_STATE_WAIT) state. 
 * Note that this state could be combined with $(TN_TASK_STATE_SUSPEND) state.
 */
_TN_STATIC_INLINE TN_BOOL _tn_task_is_waiting(struct TN_Task *task)
{
   return !!(task->task_state & TN_TASK_STATE_WAIT);
}

//}}}

//-- suspended {{{

/**
 * Bring task to the $(TN_TASK_STATE_SUSPEND ) state.
 * Should be called when `task_state` is either NONE or $(TN_TASK_STATE_WAIT).
 *
 * @param task
 *    Task to bring to the $(TN_TASK_STATE_SUSPEND) state
 */
void _tn_task_set_suspended(struct TN_Task *task);

/**
 * Bring task out from the $(TN_TASK_STATE_SUSPEND) state.
 * Task must be already in the $(TN_TASK_STATE_SUSPEND) state. It may
 * additionally be in the $(TN_TASK_STATE_WAIT) state.
 *
 * @param task
 *    Task to bring out from the $(TN_TASK_STATE_SUSPEND) state
 */
void _tn_task_clear_suspended(struct TN_Task *task);

/**
 * Returns whether given task is in $(TN_TASK_STATE_SUSPEND) state. 
 * Note that this state could be combined with $(TN_TASK_STATE_WAIT) state.
 */
_TN_STATIC_INLINE TN_BOOL _tn_task_is_suspended(struct TN_Task *task)
{
   return !!(task->task_state & TN_TASK_STATE_SUSPEND);
}

//}}}

//-- dormant {{{

/**
 * Bring task to the $(TN_TASK_STATE_DORMANT) state.
 * Should be called when task_state is NONE.
 *
 * Set DORMANT bit in task_state, reset task's priority to base value,
 * reset time slice count to 0.
 */
void _tn_task_set_dormant(struct TN_Task* task);

/**
 * Bring task out from the $(TN_TASK_STATE_DORMANT) state.
 * Should be called when task_state has just single DORMANT bit set.
 *
 * Note: task's stack will be initialized inside this function (that is,
 * `#_tn_arch_stack_init()` will be called)
 */
void _tn_task_clear_dormant(struct TN_Task *task);

/**
 * Returns whether given task is in $(TN_TASK_STATE_DORMANT) state.
 */
_TN_STATIC_INLINE TN_BOOL _tn_task_is_dormant(struct TN_Task *task)
{
   return !!(task->task_state & TN_TASK_STATE_DORMANT);
}

//}}}

//}}}



/**
 * Callback that is given to `_tn_task_first_wait_complete()`, may perform
 * any needed actions before waking task up, e.g. set some data in the `struct
 * #TN_Task` that task is waiting for.
 *
 * @param task
 *    Task that is going to be waken up
 *
 * @param user_data_1
 *    Arbitrary user data given to `_tn_task_first_wait_complete()`
 *
 * @param user_data_2
 *    Arbitrary user data given to `_tn_task_first_wait_complete()`
 */
typedef void (_TN_CBBeforeTaskWaitComplete)(
      struct TN_Task   *task,
      void             *user_data_1,
      void             *user_data_2
      );

/**
 * See the comment for tn_task_activate, tn_task_iactivate in the tn_tasks.h.
 *
 * It merely brings task out from the $(TN_TASK_STATE_DORMANT) state and 
 * brings it to the $(TN_TASK_STATE_RUNNABLE) state.
 *
 * If task is not in the `DORMANT` state, `#TN_RC_WSTATE` is returned.
 */
enum TN_RCode _tn_task_activate(struct TN_Task *task);


/**
 * Should be called when task finishes waiting for anything.
 *
 * @param wait_rc return code that will be returned to waiting task
 */
_TN_STATIC_INLINE void _tn_task_wait_complete(struct TN_Task *task, enum TN_RCode wait_rc)
{
   _tn_task_clear_waiting(task, wait_rc);

   //-- if task isn't suspended, make it runnable
   if (!_tn_task_is_suspended(task)){
      _tn_task_set_runnable(task);
   }

}


/**
 * Should be called when task starts waiting for anything.
 *
 * It merely calls `#_tn_task_clear_runnable()` and then
 * `#_tn_task_set_waiting()` for current task (`#_tn_curr_run_task`).
 */
_TN_STATIC_INLINE void _tn_task_curr_to_wait_action(
      struct TN_ListItem *wait_que,
      enum TN_WaitReason wait_reason,
      TN_TickCnt timeout
      )
{
   _tn_task_clear_runnable(_tn_curr_run_task);
   _tn_task_set_waiting(_tn_curr_run_task, wait_que, wait_reason, timeout);
}


/**
 * Change priority of any task (either runnable or non-runnable)
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

#if 0
#define _tn_task_set_last_rc(rc)  { _tn_curr_run_task = (rc); }

/**
 * If given return code is not `#TN_RC_OK`, save it in the task's structure
 */
void _tn_task_set_last_rc_if_error(enum TN_RCode rc);
#endif

#if TN_USE_MUTEXES
/**
 * Check if mutex is locked by task.
 *
 * @return TN_TRUE if mutex is locked, TN_FALSE otherwise.
 */
TN_BOOL _tn_is_mutex_locked_by_task(struct TN_Task *task, struct TN_Mutex *mutex);
#endif

/**
 * Wakes up first (if any) task from the queue, calling provided
 * callback before.
 *
 * @param wait_queue
 *    Wait queue to get first task from
 *
 * @param wait_rc
 *    Code that will be returned to woken-up task as a result of waiting
 *    (this code is just given to `_tn_task_wait_complete()` actually)
 *
 * @param callback
 *    Callback function to call before wake task up, see 
 *    `#_TN_CBBeforeTaskWaitComplete`. Can be `TN_NULL`.
 *
 * @param user_data_1
 *    Arbitrary data that is passed to the callback
 *
 * @param user_data_2
 *    Arbitrary data that is passed to the callback
 *
 *
 * @return
 *    - `TN_TRUE` if queue is not empty and task has woken up
 *    - `TN_FALSE` if queue is empty, so, no task to wake up
 */
TN_BOOL _tn_task_first_wait_complete(
      struct TN_ListItem           *wait_queue,
      enum TN_RCode                 wait_rc,
      _TN_CBBeforeTaskWaitComplete *callback,
      void                         *user_data_1,
      void                         *user_data_2
      );


/**
 * The same as `tn_task_exit(0)`, we need this function that takes no arguments
 * for exiting from task body function: we just set up initial task's stack so
 * that return address is `#_tn_task_exit_nodelete`, and it works.
 *
 * If the function takes arguments, it becomes much harder.
 */
void _tn_task_exit_nodelete(void);

/**
 * Returns end address of the stack. It depends on architecture stack
 * implementation, so there are two possible variants:
 *
 * - descending stack: `task->stack_low_addr`
 * - ascending stack:  `task->stack_high_addr`
 *
 * @param task
 *    Task in the subject
 *
 * @return
 *    End address of the stack.
 */
TN_UWord *_tn_task_stack_end_get(
      struct TN_Task *task
      );

/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Checks whether given task object is valid 
 * (actually, just checks against `id_task` field, see `enum #TN_ObjId`)
 */
_TN_STATIC_INLINE TN_BOOL _tn_task_is_valid(
      const struct TN_Task   *task
      )
{
   return (task->id_task == TN_ID_TASK);
}



#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // __TN_TASKS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


