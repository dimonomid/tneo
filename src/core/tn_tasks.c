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
#include "_tn_tasks.h"
#include "_tn_mutex.h"
#include "_tn_timer.h"
#include "_tn_list.h"


//-- header of current module
#include "_tn_tasks.h"

//-- header of other needed modules
#include "tn_mutex.h"

//-- std header for memset() and memcpy()
#include <string.h>



/*******************************************************************************
 *    EXTERNAL DATA
 ******************************************************************************/



/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
_TN_STATIC_INLINE enum TN_RCode _check_param_generic(
      const struct TN_Task *task
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (task == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (!_tn_task_is_valid(task)){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

#else
#  define _check_param_generic(task)            (TN_RC_OK)
#endif
// }}}

//-- Private utilities {{{

#if TN_USE_MUTEXES

_TN_STATIC_INLINE void _init_mutex_queue(struct TN_Task *task)
{
   _tn_list_reset(&(task->mutex_queue));
}

#if TN_MUTEX_DEADLOCK_DETECT
_TN_STATIC_INLINE void _init_deadlock_list(struct TN_Task *task)
{
   _tn_list_reset(&(task->deadlock_list));
}
#else
#  define   _init_deadlock_list(task)
#endif

#else
#  define   _init_mutex_queue(task)
#  define   _init_deadlock_list(task)
#endif


/**
 * Looks for first runnable task with highest priority,
 * set _tn_next_task_to_run to it.
 *
 * @return `TN_TRUE` if _tn_next_task_to_run was changed, `TN_FALSE` otherwise.
 */
static void _find_next_task_to_run(void)
{
   int priority;

#ifdef _TN_FFS
   //-- architecture-dependent way to find-first-set-bit is available,
   //   so use it.
   priority = _TN_FFS(_tn_ready_to_run_bmp);
   priority--;
#else
   //-- there is no architecture-dependent way to find-first-set-bit available,
   //   so, use generic (somewhat naive) algorithm.
   int i;
   unsigned int mask;

   mask = 1;
   priority = 0;

   for (i = 0; i < TN_PRIORITIES_CNT; i++){
      //-- for each bit in bmp
      if (_tn_ready_to_run_bmp & mask){
         priority = i;
         break;
      }
      mask = (mask << 1);
   }
#endif

   //-- set task to run: fetch next task from ready list of appropriate
   //   priority.
   _tn_next_task_to_run = _tn_get_task_by_tsk_queue(
         _tn_tasks_ready_list[priority].next
         );
}

// }}}

//-- Inline functions {{{


/**
 * See the comment for tn_task_wakeup, tn_task_iwakeup in the tn_tasks.h
 */
_TN_STATIC_INLINE enum TN_RCode _task_wakeup(struct TN_Task *task)
{
   enum TN_RCode rc = TN_RC_OK;

   if (     (_tn_task_is_waiting(task))
         && (task->task_wait_reason == TN_WAIT_REASON_SLEEP))
   {
      //-- Task is sleeping, so, let's wake it up.
      _tn_task_wait_complete(task, TN_RC_OK);

   } else {
      //-- Task isn't sleeping. Probably it is in WAIT state,
      //   but not because of call to tn_task_sleep().

      rc = TN_RC_WSTATE;
   }

   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _task_release_wait(struct TN_Task *task)
{
   enum TN_RCode rc = TN_RC_OK;

   if ((_tn_task_is_waiting(task))){
      //-- task is in WAIT state, so, let's release it from that state,
      //   returning TN_RC_FORCED.
      _tn_task_wait_complete(task, TN_RC_FORCED);
   } else {
      rc = TN_RC_WSTATE;
   }

   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _task_delete(struct TN_Task *task)
{
   enum TN_RCode rc = TN_RC_OK;

   if (!_tn_task_is_dormant(task)){
      //-- Cannot delete not-terminated task
      rc = TN_RC_WSTATE;
   } else {
      _tn_list_remove_entry(&(task->create_queue));
      _tn_tasks_created_cnt--;
      task->id_task = TN_ID_NONE;
   }

   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _task_job_perform(
      struct TN_Task *task,
      enum TN_RCode (p_worker)(struct TN_Task *task)
      )
{
   enum TN_RCode rc = _check_param_generic(task);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      //-- proceed to real job
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      rc = p_worker(task);

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();
   }
   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _task_job_iperform(
      struct TN_Task *task,
      enum TN_RCode (p_worker)(struct TN_Task *task)
      )
{
   enum TN_RCode rc = _check_param_generic(task);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA_INT;

      TN_INT_IDIS_SAVE();

      //-- call actual worker function (_task_wakeup, _tn_task_activate, etc)
      rc = p_worker(task);

      TN_INT_IRESTORE();
      _TN_CONTEXT_SWITCH_IPEND_IF_NEEDED();

   }
   return rc;
}

/**
 * Returns TN_TRUE if there are no more items in the runqueue for given
 * priority, TN_FALSE otherwise.
 */
_TN_STATIC_INLINE TN_BOOL _remove_entry_from_ready_queue(
      struct TN_ListItem *list_node,
      int priority
      )
{
   TN_BOOL ret;

   //-- remove given list_node from the queue
   _tn_list_remove_entry(list_node);

   //-- check if the queue for given priority is empty now
   ret = _tn_list_is_empty(&(_tn_tasks_ready_list[priority]));

   if (ret){
      //-- list is empty, so, modify bitmask _tn_ready_to_run_bmp
      _tn_ready_to_run_bmp &= ~(1 << priority);
   }

   return ret;
}

_TN_STATIC_INLINE void _add_entry_to_ready_queue(
      struct TN_ListItem *list_node, int priority
      )
{
   _tn_list_add_tail(&(_tn_tasks_ready_list[priority]), list_node);
   _tn_ready_to_run_bmp |= (1 << priority);
}

// }}}

/**
 * handle current wait_reason: say, for MUTEX_I, we should
 * handle priorities of other involved tasks.
 *
 * This function is called _after_ removing task from wait_queue,
 * because _find_max_blocked_priority()
 * in tn_mutex.c checks for all tasks in mutex's wait_queue to
 * get max blocked priority.

 * This function is called _before_ task is actually woken up,
 * so callback functions may check whatever waiting parameters
 * task had, such as task_wait_reason and pwait_queue.
 *      
 *   TODO: probably create callback list, so, when task_wait_reason 
 *         is set to TN_WAIT_REASON_MUTEX_I/.._C
 *         (this is done in tn_mutex.c, _add_curr_task_to_mutex_wait_queue())
 *         it should add its callback that will be called 
 *         when task stops waiting.
 *
 *         But it seems too much overhead: we need to allocate a memory for that
 *         every time callback is registered.
 */
static void _on_task_wait_complete(struct TN_Task *task)
{
   //-- for mutex with priority inheritance, call special handler
   if (task->task_wait_reason == TN_WAIT_REASON_MUTEX_I){
      _tn_mutex_i_on_task_wait_complete(task);
   }

   //-- for any mutex, call special handler
   if (     (task->task_wait_reason == TN_WAIT_REASON_MUTEX_I)
         || (task->task_wait_reason == TN_WAIT_REASON_MUTEX_C)
      )
   {
      _tn_mutex_on_task_wait_complete(task);
   }

}

/**
 * NOTE: task_state should be set to TN_TASK_STATE_NONE before calling.
 *
 * Teminate task:
 *    * unlock all mutexes that are held by task
 *    * set dormant state (reinitialize everything)
 *    * reitinialize stack
 */
static void _task_terminate(struct TN_Task *task)
{
#if TN_DEBUG
   if (task->task_state != TN_TASK_STATE_NONE){
      _TN_FATAL_ERROR("");
   }
#endif

   //-- Unlock all mutexes locked by the task
   _tn_mutex_unlock_all_by_task(task);

   //-- task is already in the state NONE, so, we just need 
   //   to set dormant state.
   _tn_task_set_dormant(task);
}

/**
 * This function is called by timer
 */
static void _task_wait_timeout(struct TN_Timer *timer, void *p_user_data)
{
   struct TN_Task *task = (struct TN_Task *)p_user_data;

   //-- interrupts should be enabled
   //   NOTE: we can't perform this check on PIC24/dsPIC, because on this arch
   //   TNeo has a distinction between "user" and "system" interrupts, and 
   //   TN_IS_INT_DISABLED() returns true if (IPL >= system_IPL_level).
   //   Since timer callback is called from system interrupt,
   //   TN_IS_INT_DISABLED() returns true.
   //
   //   TODO: probably make a distinction between disabled interrupts and
   //   system IPL level.
#if !defined(__TN_ARCH_PIC24_DSPIC__)
   _TN_BUG_ON( TN_IS_INT_DISABLED() );
#endif

   //-- since timer callback is called with interrupts enabled,
   //   we need to disable them before calling `_tn_task_wait_complete()`.
   TN_INTSAVE_DATA_INT;
   TN_INT_IDIS_SAVE();

   _tn_task_wait_complete(task, TN_RC_TIMEOUT);

   TN_INT_IRESTORE();

   _TN_UNUSED(timer);
}

/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_create(
      struct TN_Task         *task,
      TN_TaskBody            *task_func,
      int                     priority,
      TN_UWord               *task_stack_low_addr,
      int                     task_stack_size,
      void                   *param,
      enum TN_TaskCreateOpt   opts
      )
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc;
   enum TN_Context context;

   int i;

   //-- Lightweight checking of system tasks recreation
   if (     priority == (TN_PRIORITIES_CNT - 1)
         && !(opts & _TN_TASK_CREATE_OPT_IDLE)
      )
   {
      return TN_RC_WPARAM;
   }

   if (0
         || (priority < 0 || priority > (TN_PRIORITIES_CNT - 1))
         || task_stack_size < TN_MIN_STACK_SIZE
         || task_func == TN_NULL
         || task == TN_NULL
         || task_stack_low_addr == TN_NULL
         || _tn_task_is_valid(task)
      )
   {
      return TN_RC_WPARAM;
   }

   rc = TN_RC_OK;

   context = tn_sys_context_get();

   //-- Note: since `tn_task_create()` is called from `tn_sys_start()`, it
   //   is allowed to have `#TN_CONTEXT_NONE` here. In this case,
   //   interrupts aren't disabled/enabled.
   if (context != TN_CONTEXT_TASK && context != TN_CONTEXT_NONE){
      return TN_RC_WCONTEXT;
   }

   if (context == TN_CONTEXT_TASK){
      TN_INT_DIS_SAVE();
   }

   //--- Init task structure
   task->task_func_addr  = task_func;
   task->task_func_param = param;

   task->stack_low_addr = task_stack_low_addr;
   task->stack_high_addr = task_stack_low_addr + task_stack_size - 1;

   task->base_priority   = priority;
   task->task_state      = TN_TASK_STATE_NONE;
   task->id_task         = TN_ID_TASK;

   task->task_wait_reason = TN_WAIT_REASON_NONE;
   task->task_wait_rc = TN_RC_OK;

   task->pwait_queue  = TN_NULL;

#if TN_PROFILER
   memset(&task->profiler, 0x00, sizeof(task->profiler));
#endif

   //-- fill all task stack space by #TN_FILL_STACK_VAL
   {
      TN_UWord *ptr_stack;
      for (
            i = 0, ptr_stack = task_stack_low_addr;
            i < task_stack_size;
            i++ 
          )
      {
         *ptr_stack++ = TN_FILL_STACK_VAL;
      }
   }

   //-- reset task_queue (the queue used to include task to runqueue or 
   //   waitqueue)
   _tn_list_reset(&(task->task_queue));

   //-- init timer that is needed to implement task wait timeout
   _tn_timer_create(&task->timer, _task_wait_timeout, task);

   //-- init auxiliary lists needed for tasks
   _init_mutex_queue(task);
   _init_deadlock_list(task);

   //-- Set initial task state: `TN_TASK_STATE_DORMANT`
   _tn_task_set_dormant(task);

   //-- Add task to created task queue
   _tn_list_add_tail(&_tn_tasks_created_list, &(task->create_queue));
   _tn_tasks_created_cnt++;

   if ((opts & TN_TASK_CREATE_OPT_START)){
      _tn_task_activate(task);
   }

   if (context == TN_CONTEXT_TASK){
      TN_INT_RESTORE();
   }

   return rc;
}


enum TN_RCode tn_task_create_wname(
      struct TN_Task         *task,
      TN_TaskBody            *task_func,
      int                     priority,
      TN_UWord               *task_stack_low_addr,
      int                     task_stack_size,
      void                   *param,
      enum TN_TaskCreateOpt   opts,
      const char             *name
      )
{
   enum TN_RCode ret = tn_task_create(
         task, task_func, priority, task_stack_low_addr, task_stack_size,
         param, opts
         );

   //-- if task was successfully created, set the name
   if (ret == TN_RC_OK){
      task->name = name;
   }

   return ret;
}



/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_suspend(struct TN_Task *task)
{
   enum TN_RCode rc = _check_param_generic(task);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      if (_tn_task_is_suspended(task) || _tn_task_is_dormant(task)){
         //-- task is already suspended, or it is dormant;
         //   in either case, the state is wrong for suspending.
         rc = TN_RC_WSTATE;
      } else {

         //-- if task is runnable, clear runnable state.
         //   Note: it might be waiting instead of runnable: then,
         //   don't do anything with 'waiting' state: it is legal
         //   for task to be in waiting + suspended state.
         //   (TN_TASK_STATE_WAITSUSP)
         if (_tn_task_is_runnable(task)){
            _tn_task_clear_runnable(task);
         }

         //-- set suspended state
         _tn_task_set_suspended(task);

      }

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();

   }
   return rc;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_resume(struct TN_Task *task)
{
   enum TN_RCode rc = _check_param_generic(task);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      if (!_tn_task_is_suspended(task)){
         //-- task isn't suspended; this is wrong state.
         rc = TN_RC_WSTATE;
      } else {

         //-- clear suspended state
         _tn_task_clear_suspended(task);

         if (!_tn_task_is_waiting(task)){
            //-- The task is not in the WAIT-SUSPEND state,
            //   so we need to make it runnable and probably switch context
            _tn_task_set_runnable(task);
         }

      }

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();

   }
   return rc;

}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_sleep(TN_TickCnt timeout)
{
   enum TN_RCode rc;

   if (timeout == 0){
      rc = TN_RC_TIMEOUT;
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      //-- put task to wait with reason SLEEP and without wait queue.
      _tn_task_curr_to_wait_action(TN_NULL, TN_WAIT_REASON_SLEEP, timeout);

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();
      rc = _tn_curr_run_task->task_wait_rc;

   }

   return rc;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_wakeup(struct TN_Task *task)
{
   return _task_job_perform(task, _task_wakeup);
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_iwakeup(struct TN_Task *task)
{
   return _task_job_iperform(task, _task_wakeup);
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_activate(struct TN_Task *task)
{
   return _task_job_perform(task, _tn_task_activate);
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_iactivate(struct TN_Task *task)
{
   return _task_job_iperform(task, _tn_task_activate);
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_release_wait(struct TN_Task *task)
{
   return _task_job_perform(task, _task_release_wait);
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_irelease_wait(struct TN_Task *task)
{
   return _task_job_iperform(task, _task_release_wait);
}

/*
 * See comments in the header file (tn_tasks.h)
 */
void tn_task_exit(enum TN_TaskExitOpt opts)
{
   struct TN_Task *task;
	 
   if (!tn_is_task_context()){
      //-- do nothing, just return
   } else {

      //-- here, we unconditionally disable interrupts:
      //   this function never returns, and interrupt status is restored
      //   from different task's stack inside 
      //   `_tn_arch_context_switch_now_nosave()` call.
      tn_arch_int_dis();

      task = _tn_curr_run_task;

      //-- clear runnable state of currently running task,
      //   and terminate it

      _tn_task_clear_runnable(task);
      _task_terminate(task);

      if ((opts & TN_TASK_EXIT_OPT_DELETE)){
         //-- after exiting from task, we should delete it as well
         //   (because appropriate flag was set)
         _task_delete(task);
      }

      //-- interrupts will be enabled inside _tn_arch_context_switch_now_nosave()
      _tn_arch_context_switch_now_nosave();  
   }

   return;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_terminate(struct TN_Task *task)
{
   enum TN_RCode rc = _check_param_generic(task);
	 
   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      if (_tn_task_is_dormant(task)){
         //-- The task is already terminated
         rc = TN_RC_WSTATE;
      } else if (_tn_curr_run_task == task){
         //-- Cannot terminate currently running task
         //   (use tn_task_exit() instead)
         rc = TN_RC_WCONTEXT;
      } else {

         if (_tn_task_is_runnable(task)){
            //-- if task is runnable, we must clear runnable state
            //   before terminating the task
            _tn_task_clear_runnable(task);
         } else if (_tn_task_is_waiting(task)){
            //-- if task is waiting, we must clear waiting state
            //   before terminating the task.
            //   No matter what waiting result code we specify:
            //   nobody is going to read it.
            _tn_task_clear_waiting(
                  task,
                  TN_RC_OK    //-- doesn't matter: nobody will read it
                  );
         }

         if (_tn_task_is_suspended(task)){
            //-- if task is suspended, we must clear suspended state
            //   before terminating the task
            _tn_task_clear_suspended(task);
         }

         //-- eventually, terminate the task
         _task_terminate(task);
      }

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();

   }
   return rc;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_delete(struct TN_Task *task)
{
   enum TN_RCode rc = _check_param_generic(task);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();
      rc = _task_delete(task);
      TN_INT_RESTORE();

   }
   return rc;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_state_get(
      struct TN_Task *task,
      enum TN_TaskState *p_state
      )
{
   enum TN_RCode rc = _check_param_generic(task);
   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();
      *p_state = task->task_state;
      TN_INT_RESTORE();
   }
   return rc;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_change_priority(struct TN_Task *task, int new_priority)
{
   enum TN_RCode rc = _check_param_generic(task);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (new_priority < 0 || new_priority >= (TN_PRIORITIES_CNT - 1)){
      rc = TN_RC_WPARAM;
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      if (new_priority == 0){
         new_priority = task->base_priority;
      }

      rc = TN_RC_OK;

      if (_tn_task_is_dormant(task)){
         rc = TN_RC_WSTATE;
      } else {
         _tn_change_task_priority(task, new_priority);
      }

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();
   }
   return rc;
}

#if TN_PROFILER
enum TN_RCode tn_task_profiler_timing_get(
      const struct TN_Task *task,
      struct TN_TaskTiming *tgt
      )
{
   enum TN_RCode rc = _check_param_generic(task);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      int sr_saved;
      sr_saved = tn_arch_sr_save_int_dis();

      //-- just copy timing data from task structure
      //   to the user-provided location
      memcpy(tgt, &task->profiler.timing, sizeof(*tgt));

      tn_arch_sr_restore(sr_saved);
   }
   return rc;
}
#endif




/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

/**
 * See comment in the _tn_tasks.h file
 */
void _tn_task_set_runnable(struct TN_Task * task)
{
#if TN_DEBUG
   //-- task_state should be NONE here
   if (task->task_state != TN_TASK_STATE_NONE){
      _TN_FATAL_ERROR("");
   }
#endif

   int priority;

   priority          = task->priority;
   task->task_state  |= TN_TASK_STATE_RUNNABLE;

   //-- Add the task to the end of 'ready queue' for the current priority
   _add_entry_to_ready_queue(&(task->task_queue), priority);

   //-- less value - greater priority, so '<' operation is used here
   if (priority < _tn_next_task_to_run->priority){
      _tn_next_task_to_run = task;
   }
}

/**
 * See comment in the _tn_tasks.h file
 */
void _tn_task_clear_runnable(struct TN_Task *task)
{
#if TN_DEBUG
   //-- task_state should be exactly TN_TASK_STATE_RUNNABLE here
   if (task->task_state != TN_TASK_STATE_RUNNABLE){
      _TN_FATAL_ERROR("");
   }

   if (task == &_tn_idle_task){
      //-- idle task should always be runnable
      _TN_FATAL_ERROR("idle task should always be runnable");
   }
#endif

   //TN_BOOL ret = TN_FALSE;

   int priority;
   priority = task->priority;

   //-- remove runnable state
   task->task_state &= ~TN_TASK_STATE_RUNNABLE;

   //-- remove the curr task from any queue (now - from ready queue)
   if (_remove_entry_from_ready_queue(&(task->task_queue), priority)){
      //-- No ready tasks for the curr priority

      //-- Find highest priority ready to run -
      //   at least, lowest-priority bit must be set for the idle task
      //   (the exact bit position actually depends on TN_PRIORITIES_CNT value)
      _find_next_task_to_run();

   } else {
      //-- There are 'ready to run' task(s) for the curr priority

      if (_tn_next_task_to_run == task){
         //-- the task that just became non-runnable was the "next task to run",
         //   so we should select new next task to run
         _tn_next_task_to_run = _tn_get_task_by_tsk_queue(
               _tn_tasks_ready_list[priority].next
               );

         //-- _tn_next_task_to_run was just altered, so, we should return TN_TRUE
      }
   }

   //-- and reset task's queue
   _tn_list_reset(&(task->task_queue));

}

void _tn_task_set_waiting(
      struct TN_Task *task,
      struct TN_ListItem *wait_que,
      enum TN_WaitReason wait_reason,
      TN_TickCnt timeout
      )
{
#if TN_DEBUG
   //-- only SUSPEND bit is allowed here
   if (task->task_state & ~(TN_TASK_STATE_SUSPEND)){
      _TN_FATAL_ERROR("");
   } else if (timeout == 0){
      _TN_FATAL_ERROR("");
   } else if (_tn_timer_is_active(&task->timer)){
      _TN_FATAL_ERROR("");
   }

#endif

   task->task_state       |= TN_TASK_STATE_WAIT;
   task->task_wait_reason = wait_reason;

   task->waited           = TN_TRUE;

   //--- Add to the wait queue  - FIFO

   if (wait_que != TN_NULL){
      _tn_list_add_tail(wait_que, &(task->task_queue));
      task->pwait_queue = wait_que;
   } else {
      //-- NOTE: we don't need to reset task_queue because
      //   it is already reset in _tn_task_clear_runnable().
   }

   //-- Add to the timers queue, if timeout is neither 0 nor `TN_WAIT_INFINITE`.
   _tn_timer_start(&task->timer, timeout);
}

/**
 * See comment in the _tn_tasks.h file
 */
void _tn_task_clear_waiting(struct TN_Task *task, enum TN_RCode wait_rc)
{
#if TN_DEBUG
   //-- only WAIT and SUSPEND bits are allowed here,
   //   and WAIT bit must be set
   if (
              (task->task_state & ~(TN_TASK_STATE_WAIT | TN_TASK_STATE_SUSPEND))
         || (!(task->task_state &  (TN_TASK_STATE_WAIT)                    ))
      )
   {
      _TN_FATAL_ERROR("");
   }

   if (_tn_list_is_empty(&task->task_queue) != (task->pwait_queue == TN_NULL)){
      _TN_FATAL_ERROR("task_queue and pwait_queue are out of sync");
   }

#endif

   //-- NOTE: we should remove task from wait_queue before calling
   //   _on_task_wait_complete(), because _find_max_blocked_priority()
   //   in tn_mutex.c checks for all tasks in mutex's wait_queue to
   //   get max blocked priority

   //-- NOTE: we don't care here whether task is contained in any wait_queue,
   //   because even if it isn't, _tn_list_remove_entry() on empty list
   //   does just nothing.
   _tn_list_remove_entry(&task->task_queue);
   //-- and reset task's queue
   _tn_list_reset(&(task->task_queue));

   //-- handle current wait_reason: say, for MUTEX_I, we should
   //   handle priorities of other involved tasks.
   _on_task_wait_complete(task);

   task->pwait_queue  = TN_NULL;
   task->task_wait_rc = wait_rc;

   //-- if timer is active (i.e. task waits for timeout),
   //   cancel that timer
   _tn_timer_cancel(&task->timer);

   //-- remove WAIT state
   task->task_state &= ~TN_TASK_STATE_WAIT;

   //-- Clear wait reason
   task->task_wait_reason = TN_WAIT_REASON_NONE;
}

void _tn_task_set_suspended(struct TN_Task *task)
{
#if TN_DEBUG
   //-- only WAIT bit is allowed here
   if (task->task_state & ~(TN_TASK_STATE_WAIT)){
      _TN_FATAL_ERROR("");
   }
#endif

   task->task_state |= TN_TASK_STATE_SUSPEND;
}

void _tn_task_clear_suspended(struct TN_Task *task)
{
#if TN_DEBUG
   //-- only WAIT and SUSPEND bits are allowed here,
   //   and SUSPEND bit must be set
   if (
         (task->task_state & ~(TN_TASK_STATE_WAIT | TN_TASK_STATE_SUSPEND))
         || (!(task->task_state &                   (TN_TASK_STATE_SUSPEND)))
      )
   {
      _TN_FATAL_ERROR("");
   }
#endif

   task->task_state &= ~TN_TASK_STATE_SUSPEND;
}

void _tn_task_set_dormant(struct TN_Task* task)
{

#if TN_DEBUG
   if (task->task_state != TN_TASK_STATE_NONE){
      _TN_FATAL_ERROR("");
   } else if (!_tn_list_is_empty(&task->mutex_queue)){
      _TN_FATAL_ERROR("");
   } else if (!_tn_list_is_empty(&task->deadlock_list)){
      _TN_FATAL_ERROR("");
   }
#endif

   task->priority    = task->base_priority;      //-- Task curr priority
   task->task_state  |= TN_TASK_STATE_DORMANT;   //-- Task state

   task->tslice_count  = 0;
}

void _tn_task_clear_dormant(struct TN_Task *task)
{
#if TN_DEBUG
   //-- task_state should be exactly TN_TASK_STATE_DORMANT here
   if (task->task_state != TN_TASK_STATE_DORMANT){
      _TN_FATAL_ERROR("");
   }
#endif

   //--- Init task stack, save pointer to task top of stack,
   //    when not running
   task->stack_cur_pt = _tn_arch_stack_init(
         task->task_func_addr,
         task->stack_low_addr,
         task->stack_high_addr,
         task->task_func_param
         );

   task->task_state &= ~TN_TASK_STATE_DORMANT;


#if TN_PROFILER
   //-- If profiler is present, set last tick count
   //   to current tick count value
   task->profiler.last_tick_cnt = _tn_timer_sys_time_get();
#endif
}

/**
 * See comment in the _tn_tasks.h file
 */
enum TN_RCode _tn_task_activate(struct TN_Task *task)
{
   enum TN_RCode rc = TN_RC_OK;

   if (_tn_task_is_dormant(task)){
      _tn_task_clear_dormant(task);
      _tn_task_set_runnable(task);
   } else {
      rc = TN_RC_WSTATE;
   }

   return rc;
}

/**
 * See comment in the _tn_tasks.h file
 */
TN_BOOL _tn_task_first_wait_complete(
      struct TN_ListItem           *wait_queue,
      enum TN_RCode                 wait_rc,
      _TN_CBBeforeTaskWaitComplete *callback,
      void                         *user_data_1,
      void                         *user_data_2
      )
{
   TN_BOOL ret = TN_FALSE;

   if (!(_tn_list_is_empty(wait_queue))){
      struct TN_Task *task;
      //-- there are tasks in the wait queue, so, wake up the first one

      //-- get first task from the wait_queue
      task = _tn_list_first_entry(wait_queue, struct TN_Task, task_queue);

      //-- call provided callback (if any)
      if (callback != TN_NULL){
         callback(task, user_data_1, user_data_2);
      }

      //-- wake task up
      _tn_task_wait_complete(task, wait_rc);

      //-- indicate that some task has been woken up
      ret = TN_TRUE;
   }

   return ret;
}


/**
 * See comment in the _tn_tasks.h file
 */
void _tn_change_task_priority(struct TN_Task *task, int new_priority)
{
   if (_tn_task_is_runnable(task)){
      _tn_change_running_task_priority(task, new_priority);
   } else {
      task->priority = new_priority;
   }
}

/**
 * See comment in the _tn_tasks.h file
 */
void _tn_change_running_task_priority(struct TN_Task *task, int new_priority)
{
   if (!_tn_task_is_runnable(task)){
      _TN_FATAL_ERROR("_tn_change_running_task_priority called for non-runnable task");
   }

   //-- remove curr task from any (wait/ready) queue
   _remove_entry_from_ready_queue(&(task->task_queue), task->priority);

   task->priority = new_priority;

   //-- Add task to the end of ready queue for current priority
   _add_entry_to_ready_queue(&(task->task_queue), new_priority);

   _find_next_task_to_run();
}

#if 0
/**
 * See comment in the _tn_tasks.h file
 */
void _tn_task_set_last_rc_if_error(enum TN_RCode rc)
{
   if (rc != TN_RC_OK){
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();
      _tn_curr_run_task->last_rc = rc;
      TN_INT_RESTORE();
   }
}
#endif

#if TN_USE_MUTEXES
/**
 * See comment in the _tn_tasks.h file
 */
TN_BOOL _tn_is_mutex_locked_by_task(struct TN_Task *task, struct TN_Mutex *mutex)
{
   TN_BOOL ret = TN_FALSE;

   struct TN_Mutex *tmp_mutex;
   _tn_list_for_each_entry(
         tmp_mutex, struct TN_Mutex, &(task->mutex_queue), mutex_queue
         )
   {
      if (tmp_mutex == mutex){
         ret = TN_TRUE;
         break;
      }
   }

   return ret;
}

#endif

/*
 * See comment in the _tn_tasks.h file
 */
void _tn_task_exit_nodelete(void)
{
   tn_task_exit((enum TN_TaskExitOpt)(0));
}



#if !defined(_TN_ARCH_STACK_DIR)
#  error _TN_ARCH_STACK_DIR is not defined
#endif

#if (_TN_ARCH_STACK_DIR == _TN_ARCH_STACK_DIR__ASC)
//-- full ascending stack {{{

/*
 * See comments in the file _tn_sys.h
 */
TN_UWord *_tn_task_stack_end_get(
      struct TN_Task *task
      )
{
   return task->stack_high_addr;
}

// }}}
#elif (_TN_ARCH_STACK_DIR == _TN_ARCH_STACK_DIR__DESC)
//-- full descending stack {{{

/*
 * See comments in the file _tn_tasks.h
 */
TN_UWord *_tn_task_stack_end_get(
      struct TN_Task *task
      )
{
   return task->stack_low_addr;
}

// }}}
#else
#  error wrong _TN_ARCH_STACK_DIR
#endif

