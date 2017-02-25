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

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- common tnkernel headers
#include "tn_common.h"
#include "tn_sys.h"

//-- internal tnkernel headers
#include "_tn_mutex.h"
#include "_tn_tasks.h"
#include "_tn_list.h"

//-- header of current module
#include "tn_mutex.h"

//-- header of other needed modules
#include "tn_tasks.h"


#if TN_USE_MUTEXES



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#if TN_DEBUG
#  define _get_mutex_by_wait_queque(que)               \
      (que ? container_of(que, struct TN_Mutex, wait_queue) : 0)
#else
//-- when `TN_DEBUG` isn't set, don't check for `que` to be not `NULL`
#  define _get_mutex_by_wait_queque(que)               \
      container_of(que, struct TN_Mutex, wait_queue)
#endif




#if TN_MUTEX_REC
//-- recursive locking enabled
#  define __mutex_lock_cnt_change(mutex, value)  { (mutex)->cnt += (value); }
#  define __MUTEX_REC_LOCK_RETVAL   TN_RC_OK
#else
//-- recursive locking disabled
#  define __mutex_lock_cnt_change(mutex, value)
#  define __MUTEX_REC_LOCK_RETVAL   TN_RC_ILLEGAL_USE
#endif

// L. Sha, R. Rajkumar, J. Lehoczky, Priority Inheritance Protocols: An Approach
// to Real-Time Synchronization, IEEE Transactions on Computers, Vol.39, No.9, 1990




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
_TN_STATIC_INLINE enum TN_RCode _check_param_generic(
      const struct TN_Mutex *mutex
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (mutex == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (!_tn_mutex_is_valid(mutex)){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _check_param_create(
      const struct TN_Mutex        *mutex,
      enum TN_MutexProtocol   protocol,
      int                     ceil_priority
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (mutex == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (_tn_mutex_is_valid(mutex)){
      rc = TN_RC_WPARAM;
   } else if (    protocol != TN_MUTEX_PROT_CEILING 
               && protocol != TN_MUTEX_PROT_INHERIT)
   {
      rc = TN_RC_WPARAM;
   } else if (1
         && protocol == TN_MUTEX_PROT_CEILING 
         && (0
            || ceil_priority <  0
            || ceil_priority >= (TN_PRIORITIES_CNT - 1)
            )
         )
   {
      rc = TN_RC_WPARAM;
   }

   return rc;
}

#else
#  define _check_param_generic(mutex)                             (TN_RC_OK)
#  define _check_param_create(mutex, protocol, ceil_priority)     (TN_RC_OK)
#endif
// }}}


/**
 * Iterate through all the tasks that wait for locked mutex,
 * checking if task's priority is higher than ref_priority.
 *
 * Max priority (i.e. lowest value) is returned.
 */
_TN_STATIC_INLINE int _find_max_blocked_priority(struct TN_Mutex *mutex, int ref_priority)
{
   int               priority;
   struct TN_Task   *task;

   priority = ref_priority;

   //-- Iterate through all the tasks that wait for lock mutex.
   //   Highest priority (i.e. lowest number) will be returned eventually.
   _tn_list_for_each_entry(
         task, struct TN_Task, &(mutex->wait_queue), task_queue
         )
   {
      if (task->priority < priority){
         //--  task priority is higher, remember it
         priority = task->priority;
      }
   }

   return priority;
}

/**
 * Returns max priority that could be set to some task because
 * it locked given mutex, but not less than given `ref_priority`.
 */
_TN_STATIC_INLINE int _find_max_priority_by_mutex(
      struct TN_Mutex *mutex, int ref_priority
      )
{
   int priority = ref_priority;

   switch (mutex->protocol){
      case TN_MUTEX_PROT_CEILING:
         //-- Mutex protocol is 'priority ceiling':
         //   just check specified ceil_priority of the mutex
         if (mutex->ceil_priority < priority){
            priority = mutex->ceil_priority;
         }
         break;

      case TN_MUTEX_PROT_INHERIT:
         //-- Mutex protocol is 'priority inheritance':
         //   we need to iterate through all the tasks that wait for 
         //   the mutex, checking if task's priority is higher than
         //   `ref_priority`.
         priority = _find_max_blocked_priority(mutex, priority);
         break;

      default:
         //-- should never happen
         _TN_FATAL_ERROR("wrong mutex protocol=%d", mutex->protocol);
         break;
   }

   return priority;
}

/**
 * Iterate through all the mutexes that are held by task,
 * for each mutex:
 *
 *    - if protocol is TN_MUTEX_PROT_CEILING:
 *      check if ceil priority higher than task's base priority
 *    - if protocol is TN_MUTEX_PROT_INHERIT:
 *      iterate through all the tasks that wait for this mutex,
 *      and check if priority of each task is higher than
 *      our task's base priority
 *
 * Eventually, find out highest priority and set it.
 */
static void _update_task_priority(struct TN_Task *task)
{
   int priority;

   //-- Now, we need to determine new priority of current task.
   //   We start from its base priority, but if there are other
   //   mutexes that are locked by the task, we should check
   //   what priority we should set.
   priority = task->base_priority;

   {
      struct TN_Mutex *mutex;

      //-- Iterate through all the mutexes locked by given task
      _tn_list_for_each_entry(
            mutex, struct TN_Mutex, &(task->mutex_queue), mutex_queue
            )
      {
         priority = _find_max_priority_by_mutex(mutex, priority);
      }
   }

   //-- New priority determined, set it
   if (priority != task->priority){
      _tn_change_task_priority(task, priority);
   }
}


/**
 * Elevate task's priority to given value (if task's priority is now lower).
 * If task is waiting for some mutex too, go on to holder of that mutex
 * and elevate its priority too, recursively. And so on.
 */
_TN_STATIC_INLINE void _task_priority_elevate(struct TN_Task *task, int priority)
{
in:
   //-- transitive priority changing

   // if we have a task A that is blocked by the task B and we changed priority
   // of task A,priority of task B also have to be changed. I.e, if we have
   // the task A that is waiting for the mutex M1 and we changed priority
   // of this task, a task B that holds a mutex M1 now, also needs priority's
   // changing.  Then, if a task B now is waiting for the mutex M2, the same
   // procedure have to be applied to the task C that hold a mutex M2 now
   // and so on.


   if (task->priority <= priority){
      //-- Task's priority is alreasy higher than given one, so,
      //   don't do anything
   } else if (_tn_task_is_runnable(task)){
      //-- Task is runnable, so, set new priority to it
      //   (for runnable tasks, we should use special function for that)
      _tn_change_running_task_priority(task, priority);
   } else {
      //-- Task is not runnable, so, just set new priority to it
      task->priority = priority;

      //-- and check if the task is waiting for mutex
      if (     (_tn_task_is_waiting(task))
            && (task->task_wait_reason == TN_WAIT_REASON_MUTEX_I)
         )
      {
         //-- Task is waiting for another mutex. In this case, 
         //   call this function again, recursively,
         //   for mutex's holder
         //
         //   NOTE: as a workaround for crappy compilers that don't
         //   know what is tail-recursion, we have to use goto explicitly.
         //   (see SICP, section 1.2.1: Linear Recursion and Iteration)
         //
         //_task_priority_elevate(
         //      _get_mutex_by_wait_queque(task->pwait_queue)->holder,
         //      priority
         //      );

         task = _get_mutex_by_wait_queque(task->pwait_queue)->holder;
         goto in;
      }
   }

}

_TN_STATIC_INLINE void _mutex_do_lock(struct TN_Mutex *mutex, struct TN_Task *task)
{
   mutex->holder = task;
   __mutex_lock_cnt_change(mutex, 1);

   //-- Add mutex to task's locked mutexes queue
   _tn_list_add_tail(&(task->mutex_queue), &(mutex->mutex_queue));

   //-- Determine new priority for the task
   {
      int new_priority = _find_max_priority_by_mutex(mutex, task->priority);
      if (task->priority != new_priority){
         _tn_change_task_priority(task, new_priority);
      }
   }

}

#if TN_MUTEX_DEADLOCK_DETECT

/**
 * Link all mutexes and all tasks involved in deadlock together.
 * Should be called when deadlock is detected.
 */
static void _link_deadlock_lists(struct TN_Mutex *mutex, struct TN_Task *task)
{
   struct TN_Task *holder;

in:
   holder = mutex->holder;

   if (     (holder->task_wait_reason != TN_WAIT_REASON_MUTEX_I)
         && (holder->task_wait_reason != TN_WAIT_REASON_MUTEX_C)
      )
   {
      _TN_FATAL_ERROR();
   }

   struct TN_Mutex *mutex2 = _get_mutex_by_wait_queque(holder->pwait_queue);

   //-- link two tasks together
   _tn_list_add_tail(&task->deadlock_list, &holder->deadlock_list); 

   //-- link two mutexes together
   _tn_list_add_head(&mutex->deadlock_list, &mutex2->deadlock_list); 

   if (_tn_is_mutex_locked_by_task(task, mutex2)){
      //-- done; all mutexes and tasks involved in deadlock were linked together.
      //   will return now.
   } else {
      //-- call this function again, recursively
      //
      //   NOTE: as a workaround for crappy compilers that don't
      //   know what is tail-recursion, we have to use goto explicitly.
      //   (see SICP, section 1.2.1: Linear Recursion and Iteration)
      //
      //_link_deadlock_lists(mutex2, task);

      mutex = mutex2;
      goto in;
   }
}

/**
 * Unlink deadlock lists (for mutexes and tasks involved)
 */
static void _unlink_deadlock_lists(struct TN_Mutex *mutex, struct TN_Task *task)
{
   struct TN_ListItem *item;
   struct TN_ListItem *tmp_item;

   //-- unlink list of mutexes
   _tn_list_for_each_safe(item, tmp_item, &mutex->deadlock_list){
      _tn_list_remove_entry(item);
      _tn_list_reset(item);
   }

   //-- unlink list of tasks
   _tn_list_for_each_safe(item, tmp_item, &task->deadlock_list){
      _tn_list_remove_entry(item);
      _tn_list_reset(item);
   }

}

/**
 * Should be called whenever task wanted to lock mutex, but it is already 
 * locked, so, task was just added to mutex's wait_queue.
 *
 * Checks for deadlock; if it is detected, flag TN_STATE_FLAG__DEADLOCK is set.
 */
static void _check_deadlock_active(struct TN_Mutex *mutex, struct TN_Task *task)
{
   struct TN_Task *holder;

in:
   holder = mutex->holder;
   if (     (_tn_task_is_waiting(task))
         && (
               (holder->task_wait_reason == TN_WAIT_REASON_MUTEX_I)
            || (holder->task_wait_reason == TN_WAIT_REASON_MUTEX_C)
            )
      )
   {
      //-- holder is waiting for another mutex (mutex2). In this case, 
      //   check against task's locked mutexes list; if mutex2 is
      //   there - deadlock.
      //
      //   Otherwise, get holder of mutex2 and call this function
      //   again, recursively.

      struct TN_Mutex *mutex2 = _get_mutex_by_wait_queque(holder->pwait_queue);
      if (_tn_is_mutex_locked_by_task(task, mutex2)){
         //-- link all mutexes and all tasks involved in the deadlock
         _link_deadlock_lists(
               _get_mutex_by_wait_queque(task->pwait_queue),
               task
               );

         //-- cry deadlock active
         //   (user will be notified if he has set callback with
         //   tn_callback_deadlock_set()) NOTE: we should call this function
         //   _after_ calling _link_deadlock_lists(), so that user may examine
         //   mutexes and tasks involved in deadlock.
         _tn_cry_deadlock(TN_TRUE, mutex, task);
      } else {
         //-- call this function again, recursively
         //
         //   NOTE: as a workaround for crappy compilers that don't
         //   know what is tail-recursion, we have to use goto explicitly.
         //   (see SICP, section 1.2.1: Linear Recursion and Iteration)
         //
         //_check_deadlock_active(mutex2, task);

         mutex = mutex2;
         goto in;
      }

   } else {
      //-- no deadlock: holder of given mutex isn't waiting for another mutex
   }
}

/**
 * If deadlock was active with given task and mutex involved,
 * cry that deadlock becomes inactive,
 * and unlink deadlock lists (for mutexes and tasks involved)
 */
static void _cry_deadlock_inactive(struct TN_Mutex *mutex, struct TN_Task *task)
{
   if (!_tn_list_is_empty(&mutex->deadlock_list)){

      if (_tn_list_is_empty(&task->deadlock_list)){
         //-- should never be here: deadlock lists for tasks and mutexes
         //   should either be both non-empty or both empty
         _TN_FATAL_ERROR();
      }

      //-- cry that deadlock becomes inactive
      //   (user will be notified if he has set callback with
      //   tn_callback_deadlock_set()) NOTE: we should call this function
      //   _before_ calling _unlink_deadlock_lists(), so that user may examine
      //   mutexes and tasks involved in deadlock.
      _tn_cry_deadlock(TN_FALSE, mutex, task);

      //-- unlink deadlock lists (for mutexes and tasks involved)
      _unlink_deadlock_lists(mutex, task);
   }

}
#else
static void _check_deadlock_active(struct TN_Mutex *mutex, struct TN_Task *task)
{}
static void _cry_deadlock_inactive(struct TN_Mutex *mutex, struct TN_Task *task)
{}
#endif

_TN_STATIC_INLINE void _add_curr_task_to_mutex_wait_queue(
      struct TN_Mutex *mutex,
      TN_TickCnt timeout
      )
{
   enum TN_WaitReason wait_reason;

   if (mutex->protocol == TN_MUTEX_PROT_INHERIT){
      //-- Priority inheritance protocol

      //-- if run_task curr priority higher holder's curr priority
      if (_tn_curr_run_task->priority < mutex->holder->priority){
         _task_priority_elevate(mutex->holder, _tn_curr_run_task->priority);
      }

      wait_reason = TN_WAIT_REASON_MUTEX_I;
   } else {
      //-- Priority ceiling protocol
      wait_reason = TN_WAIT_REASON_MUTEX_C;
   }

   _tn_task_curr_to_wait_action(&(mutex->wait_queue), wait_reason, timeout);

   //-- check if there is deadlock
   _check_deadlock_active(mutex, _tn_curr_run_task);
}

/**
 *    * Unconditionally set lock count to 0. This is needed because mutex
 *      might be deleted 'unexpectedly' when its holder task is deleted
 *    * Remove given mutex from task's locked mutexes list,
 *    * Set new priority of the task
 *      (depending on its base_priority and other locked mutexes),
 *    * If no other tasks want to lock this mutex, set holder to TN_NULL,
 *      otherwise grab first task from the mutex's wait_queue
 *      and lock mutex by this task.
 *
 * @returns TN_TRUE if context switch is needed
 *          (that is, if there is some other task that waited for mutex,
 *          and this task has highest priority now)
 */
static void _mutex_do_unlock(struct TN_Mutex * mutex)
{
   //-- explicitly reset lock count to 0, because it might be not zero
   //   if mutex is unlocked because task is being deleted.
   mutex->cnt = 0;

   //-- Delete curr mutex from task's locked mutexes queue
   _tn_list_remove_entry(&(mutex->mutex_queue));

   //-- update priority for current holder
   _update_task_priority(mutex->holder);

   //-- Check for the task(s) that want to lock the mutex
   if (_tn_list_is_empty(&(mutex->wait_queue))){
      //-- no more tasks want to lock the mutex,
      //   so, set holder to TN_NULL and return.
      mutex->holder = TN_NULL;
   } else {
      //-- there are tasks that want to lock the mutex,
      //   so, lock it by the first task in the queue

      struct TN_Task *task;

      //-- get first task from mutex's wait_queue
      task = _tn_list_first_entry(
            &(mutex->wait_queue), struct TN_Task, task_queue
            );

      //-- wake it up.
      //   Note: _update_task_priority() for current holder
      //   of mutex will be eventually called from:
      //    _tn_task_wait_complete ->
      //       _tn_task_clear_waiting ->
      //          _on_task_wait_complete ->
      //             _tn_mutex_i_on_task_wait_complete().
      //
      //   But, we have already called it above. It would be
      //   not so efficient to perform the same job twice,
      //   so, special flag invented: priority_already_updated.
      //   It's probably not so elegant, but I believe it is
      //   acceptable tradeoff in the name of efficiency.
      //
      //   NOTE that we can't remove the above call _update_task_priority(),
      //   because it then won't be called if nobody waited for mutex.
      mutex->holder->priority_already_updated = TN_TRUE;
      _tn_task_wait_complete(task, TN_RC_OK);
      mutex->holder->priority_already_updated = TN_FALSE;

      //-- lock mutex by it
      _mutex_do_lock(mutex, task);
   }
}

/**
 * Update priority of the holder of mutex which `task` was waiting for. If the
 * holder itself waits for some mutex2, update priority of mutex2's holder, and
 * so on, recursively.
 *
 * Say, we have the following arrangement:
 *
 * - `task_a` locked mutex M1:               `task_a` is runnable;
 * - `task_b` locked mutex M2, waits for M1: `task_b` is waiting;
 * - `task_c`                  waits for M2: `task_c` is waiting.
 * 
 * Now, `task_c` finishes waiting for mutex by timeout, and
 * `_update_holders_priority_recursive(task_c)` is eventually called.
 *
 * First, we get `task_b` since it is a holder of M2 which `task_c` was waiting
 * for. So, priority of `task_b` is updated.
 *
 * Then, we get `task_a` since it is a holder of M1 which `task_b` is waiting
 * for. So, priority of `task_a` is updated.
 *
 * Then we see that `task_a` doesn't wait for any mutex, and the function 
 * returns.
 *
 * Preconditions:
 *
 * - `task->pwait_queue` should point to the mutex wait queue;
 * - `mutex->holder` should point to the holder.
 *
 */
static void _update_holders_priority_recursive(struct TN_Task *task)
{
   struct TN_Task *holder;

in:
   //-- get the holder of mutex for which `task` is/was waiting for.
   //   NOTE that this holder might still hold the mutex
   //   (if `task` stopped waiting by timeout), or it might
   //   already release it. In either case, `mutex->holder` still
   //   points to the task which is/was holding the mutex.
   holder = _get_mutex_by_wait_queque(task->pwait_queue)->holder;

   //-- now, `holder` points to the (ex-)holder, i.e. to the task which is/was
   //   holding the mutex. Now, we iterate through all the mutexes that are
   //   still held by (ex-)holder, determining new priority for (ex-)holder.
   _update_task_priority(holder);

   //-- and check if the (ex-)holder is also waiting for some other mutex
   if (     (_tn_task_is_waiting(holder))
         && (holder->task_wait_reason == TN_WAIT_REASON_MUTEX_I)
      )
   {
      //-- holder is waiting for another mutex. In this case, call this
      //   function again, recursively, for the holder.
      //
      //   NOTE: as a workaround for crappy compilers that don't
      //   know what is tail-recursion, we have to use goto explicitly.
      //   (see SICP, section 1.2.1: Linear Recursion and Iteration)
      //
      //_update_holders_priority_recursive(holder);

      task = holder;
      goto in;
   }
}



/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (tn_mutex.h)
 */
enum TN_RCode tn_mutex_create(
      struct TN_Mutex        *mutex,
      enum TN_MutexProtocol   protocol,
      int                     ceil_priority
      )
{
   enum TN_RCode rc = _check_param_create(mutex, protocol, ceil_priority);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {

      _tn_list_reset(&(mutex->wait_queue));
      _tn_list_reset(&(mutex->mutex_queue));
#if TN_MUTEX_DEADLOCK_DETECT
      _tn_list_reset(&(mutex->deadlock_list));
#endif

      mutex->protocol      = protocol;
      mutex->holder        = TN_NULL;
      mutex->ceil_priority = ceil_priority;
      mutex->cnt           = 0;
      mutex->id_mutex      = TN_ID_MUTEX;
   }

   return rc;
}

/*
 * See comments in the header file (tn_mutex.h)
 */
enum TN_RCode tn_mutex_delete(struct TN_Mutex *mutex)
{
   enum TN_RCode rc = _check_param_generic(mutex);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      //-- mutex can be deleted if only it isn't held 
      if (mutex->holder != TN_NULL && mutex->holder != _tn_curr_run_task){
         rc = TN_RC_ILLEGAL_USE;
      } else {

         //-- Remove all tasks (if any) from mutex's wait queue
         _tn_wait_queue_notify_deleted(&(mutex->wait_queue));

         if (mutex->holder != TN_NULL){
            //-- If the mutex is locked
            _mutex_do_unlock(mutex);

            //-- NOTE: redundant reset, because it will anyway
            //         be reset in tn_mutex_create()
            //
            //         Probably we need to remove it.
            _tn_list_reset(&(mutex->mutex_queue));
         }

         mutex->id_mutex = TN_ID_NONE; //-- mutex does not exist now

      }

      TN_INT_RESTORE();

      //-- we might need to switch context if _tn_wait_queue_notify_deleted()
      //   has woken up some high-priority task
      _tn_context_switch_pend_if_needed();
   }

   return rc;
}

/*
 * See comments in the header file (tn_mutex.h)
 */
enum TN_RCode tn_mutex_lock(struct TN_Mutex *mutex, TN_TickCnt timeout)
{
   enum TN_RCode rc = _check_param_generic(mutex);
   TN_BOOL waited_for_mutex = TN_FALSE;

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      if (_tn_curr_run_task == mutex->holder){
         //-- mutex is already locked by current task
         //   if recursive locking enabled (TN_MUTEX_REC), increment lock count,
         //   otherwise error is returned
         __mutex_lock_cnt_change(mutex, 1);
         rc = __MUTEX_REC_LOCK_RETVAL;

      } else if (
            mutex->protocol == TN_MUTEX_PROT_CEILING
            && _tn_curr_run_task->base_priority < mutex->ceil_priority
            )
      {
         //-- base priority of current task higher
         rc = TN_RC_ILLEGAL_USE;

      } else if (mutex->holder == TN_NULL){
         //-- mutex is not locked, let's lock it

         //-- TODO: probably, we should add special flag to _mutex_do_lock,
         //   something like "other_tasks_can_wait", and set it to false here.
         //   When _mutex_do_lock() is called from _mutex_do_unlock(), this flag
         //   should be set to true there.
         //   _mutex_do_lock() should forward this flag to _find_max_priority_by_mutex(),
         //   and if that flag is false, _find_max_priority_by_mutex() should not
         //   call _find_max_blocked_priority().
         //   We could save about 30 cycles then. =)
         _mutex_do_lock(mutex, _tn_curr_run_task);

      } else {
         //-- mutex is already locked

         if (timeout == 0){
            //-- in polling mode, just return TN_RC_TIMEOUT
            rc = TN_RC_TIMEOUT;
         } else {
            //-- timeout specified, so, wait until mutex is free or timeout expired
            _add_curr_task_to_mutex_wait_queue(mutex, timeout);

            waited_for_mutex = TN_TRUE;

            //-- rc will be set later to _tn_curr_run_task->task_wait_rc;
         }
      }

#if TN_DEBUG
      if (!_tn_need_context_switch() && waited_for_mutex){
         _TN_FATAL_ERROR("");
      }
#endif

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();
      if (waited_for_mutex){
         //-- get wait result
         rc = _tn_curr_run_task->task_wait_rc;
      }
   }

   return rc;
}

/*
 * See comments in the header file (tn_mutex.h)
 */
enum TN_RCode tn_mutex_lock_polling(struct TN_Mutex *mutex)
{
   return tn_mutex_lock(mutex, 0);
}


/*
 * See comments in the header file (tn_mutex.h)
 */
enum TN_RCode tn_mutex_unlock(struct TN_Mutex *mutex)
{
   enum TN_RCode rc = _check_param_generic(mutex);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      //-- unlocking is enabled only for the owner and already locked mutex
      if (_tn_curr_run_task != mutex->holder){
         rc = TN_RC_ILLEGAL_USE;
      } else {

         //-- decrement lock count (if recursive locking is enabled)
         __mutex_lock_cnt_change(mutex, -1);

         if (mutex->cnt > 0){
            //-- there was recursive lock, so here we just decremented counter, 
            //   but don't unlock the mutex. 
            //   We're done, TN_RC_OK will be returned.
         } else if (mutex->cnt < 0){
            //-- should never be here: lock count is negative.
            //   Bug in the kernel, or memory got corrupted.
            _TN_FATAL_ERROR();
         } else {
            //-- lock counter is 0, so, unlock mutex
            _mutex_do_unlock(mutex);
         }

      }

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();
   }

   return rc;

}





/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

/**
 * See comment in _tn_mutex.h file
 */
void _tn_mutex_unlock_all_by_task(struct TN_Task *task)
{
   struct TN_Mutex *mutex;       //-- "cursor" for the loop iteration
   struct TN_Mutex *tmp_mutex;   //-- we need for temporary item because
                                 //   item is removed from the list
                                 //   in _mutex_do_unlock().

   _tn_list_for_each_entry_safe(
         mutex, struct TN_Mutex, tmp_mutex, &(task->mutex_queue), mutex_queue
         )
   {
      //-- NOTE: we don't remove item from the list, because it is removed
      //   inside _mutex_do_unlock().
      _mutex_do_unlock(mutex);
   }
}


/**
 * See comments in _tn_mutex.h file
 */
void _tn_mutex_i_on_task_wait_complete(struct TN_Task *task)
{
   //-- NOTE: task->task_wait_reason should be TN_WAIT_REASON_MUTEX_I here
#if TN_DEBUG
   if (task->task_wait_reason != TN_WAIT_REASON_MUTEX_I){
      _TN_FATAL_ERROR();
   }
#endif

   if (task->priority_already_updated){
      //-- priority is already updated (in _mutex_do_unlock)
      //   so, just do nothing here
      //   (flag will be cleared in _mutex_do_unlock 
      //   when we exit from `_tn_task_wait_complete()`)
   } else {
      //-- Update priority of the holder of mutex which `task` was waiting
      //   for. If the holder itself waits for some mutex2, update priority
      //   of mutex2's holder, and so on, recursively.
      //
      //   See comments for `_update_holders_priority_recursive()` for details.
      _update_holders_priority_recursive(task);
   }

}

/**
 * See comments in _tn_mutex.h file
 */
void _tn_mutex_on_task_wait_complete(struct TN_Task *task)
{
   //-- if deadlock was active with given task involved,
   //   it means that deadlock becomes inactive. So, notify user about it
   //   and unlink deadlock lists (for mutexes and tasks involved)
   _cry_deadlock_inactive(
         _get_mutex_by_wait_queque(task->pwait_queue),
         task
         );
}


#endif //-- TN_USE_MUTEXES

