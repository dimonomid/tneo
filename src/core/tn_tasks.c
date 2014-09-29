/*******************************************************************************
 *
 * TNeoKernel: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeoKernel:                copyright © 2014       Dmitry Frank.
 *
 *    TNeoKernel was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
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

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- common tnkernel headers
#include "tn_common.h"
#include "tn_sys.h"
#include "tn_internal.h"

//-- header of current module
#include "tn_tasks.h"

//-- header of other needed modules
#include "tn_mutex.h"



/*******************************************************************************
 *    EXTERNAL DATA
 ******************************************************************************/



/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
static inline enum TN_RCode _check_param_generic(
      struct TN_Task *task
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (task == NULL){
      rc = TN_RC_WPARAM;
   } else if (task->id_task != TN_ID_TASK){
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

static inline void _init_mutex_queue(struct TN_Task *task)
{
   tn_list_reset(&(task->mutex_queue));
}

#if TN_MUTEX_DEADLOCK_DETECT
static inline void _init_deadlock_list(struct TN_Task *task)
{
   tn_list_reset(&(task->deadlock_list));
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
 * set tn_next_task_to_run to it.
 *
 * @return TRUE if tn_next_task_to_run was changed, FALSE otherwise.
 */
static void _find_next_task_to_run(void)
{
   int priority;

#ifdef _TN_FFS
   priority = _TN_FFS(tn_ready_to_run_bmp);
   priority--;
#else
   int i;
   unsigned int mask;

   mask = 1;
   priority = 0;

   for (i = 0; i < TN_PRIORITIES_CNT; i++){
      //-- for each bit in bmp
      if (tn_ready_to_run_bmp & mask){
         priority = i;
         break;
      }
      mask = (mask << 1);
   }
#endif

   tn_next_task_to_run = get_task_by_tsk_queue(tn_ready_list[priority].next);
}

// }}}

//-- Inline functions {{{


/**
 * See the comment for tn_task_wakeup, tn_task_iwakeup in the tn_tasks.h
 */
static inline enum TN_RCode _task_wakeup(struct TN_Task *task)
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

static inline enum TN_RCode _task_release_wait(struct TN_Task *task)
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

static inline enum TN_RCode _task_delete(struct TN_Task *task)
{
   enum TN_RCode rc = TN_RC_OK;

   if (!_tn_task_is_dormant(task)){
      //-- Cannot delete not-terminated task
      rc = TN_RC_WSTATE;
   } else {
      tn_list_remove_entry(&(task->create_queue));
      tn_created_tasks_cnt--;
      task->id_task = 0;
   }

   return rc;
}

static inline enum TN_RCode _task_job_perform(
      struct TN_Task *task,
      int (p_worker)(struct TN_Task *task)
      )
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(task);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   rc = p_worker(task);

   TN_INT_RESTORE();
   _tn_switch_context_if_needed();

out:
   return rc;
}

static inline enum TN_RCode _task_job_iperform(
      struct TN_Task *task,
      int (p_worker)(struct TN_Task *task)
      )
{
   TN_INTSAVE_DATA_INT;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(task);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_IDIS_SAVE();

   rc = p_worker(task);

   TN_INT_IRESTORE();

out:
   return rc;
}

/**
 * Returns TRUE if there are no more items in the runqueue for given priority,
 *         FALSE otherwise.
 */
static inline BOOL _remove_entry_from_ready_queue(struct TN_ListItem *list_node, int priority)
{
   BOOL ret;

   //-- remove given list_node from the queue
   tn_list_remove_entry(list_node);

   //-- check if the queue for given priority is empty now
   ret = tn_is_list_empty(&(tn_ready_list[priority]));

   if (ret){
      //-- list is empty, so, modify bitmask tn_ready_to_run_bmp
      tn_ready_to_run_bmp &= ~(1 << priority);
   }

   return ret;
}

static inline void _add_entry_to_ready_queue(struct TN_ListItem *list_node, int priority)
{
   tn_list_add_tail(&(tn_ready_list[priority]), list_node);
   tn_ready_to_run_bmp |= (1 << priority);
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

   _tn_task_set_dormant(task);
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
      unsigned int           *task_stack_low_addr,
      int                     task_stack_size,
      void                   *param,
      enum TN_TaskCreateOpt   opts
      )
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc;
   enum TN_Context context;

   unsigned int *ptr_stack;
   int i;

   //-- Lightweight checking of system tasks recreation
   if (0
         || (priority == (TN_PRIORITIES_CNT - 1) && !(opts & TN_TASK_CREATE_OPT_IDLE))
         || (priority == 0)   //-- there's no more timer task in the kernel,
                              //   but for a kind of compatibility
                              //   it's better to disallow tasks with priority 0
      )
   {
      return TN_RC_WPARAM;
   }

   if (0
         || (priority < 0 || priority > (TN_PRIORITIES_CNT - 1))
         || task_stack_size < TN_MIN_STACK_SIZE
         || task_func == NULL
         || task == NULL
         || task_stack_low_addr == NULL
         || task->id_task == TN_ID_TASK
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

   //--- Init task TCB
   task->task_func_addr  = task_func;
   task->task_func_param = param;
   task->stk_start       = _tn_arch_stack_start_get(
         task_stack_low_addr,
         task_stack_size
         );
   task->stk_size        = task_stack_size;
   task->base_priority   = priority;
   task->task_state      = TN_TASK_STATE_NONE;
   task->id_task         = TN_ID_TASK;

   //-- fill all task stack space by #TN_FILL_STACK_VAL
   for (ptr_stack = task->stk_start, i = 0; i < task->stk_size; i++){
      *ptr_stack-- = TN_FILL_STACK_VAL;
   }

   _tn_task_set_dormant(task);

   //-- Add task to created task queue
   tn_list_add_tail(&tn_create_queue, &(task->create_queue));
   tn_created_tasks_cnt++;

   if ((opts & TN_TASK_CREATE_OPT_START)){
      _tn_task_activate(task);
   }

   if (context == TN_CONTEXT_TASK){
      TN_INT_RESTORE();
   }

   return rc;
}



/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_suspend(struct TN_Task *task)
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(task);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   if (_tn_task_is_suspended(task) || _tn_task_is_dormant(task)){
      rc = TN_RC_WSTATE;
      goto out_ei;
   }

   if (_tn_task_is_runnable(task)){
      _tn_task_clear_runnable(task);
   }

   _tn_task_set_suspended(task);

out_ei:
   TN_INT_RESTORE();
   _tn_switch_context_if_needed();

out:
   return rc;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_resume(struct TN_Task *task)
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(task);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   if (!_tn_task_is_suspended(task)){
      rc = TN_RC_WSTATE;
      goto out_ei;
   }

   _tn_task_clear_suspended(task);

   if (!_tn_task_is_waiting(task)){
      //-- The task is not in the WAIT-SUSPEND state,
      //   so we need to make it runnable and probably switch context
      _tn_task_set_runnable(task);
   }

out_ei:
   TN_INT_RESTORE();
   _tn_switch_context_if_needed();

out:
   return rc;

}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_sleep(TN_Timeout timeout)
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc;

   if (timeout == 0){
      rc = TN_RC_TIMEOUT;
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   _tn_task_curr_to_wait_action(NULL, TN_WAIT_REASON_SLEEP, timeout);

   TN_INT_RESTORE();
   _tn_switch_context_if_needed();
   rc = tn_curr_run_task->task_wait_rc;

out:
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

   //-- it is here only for TN_INT_DIS_SAVE() normal operation,
   //   but actually we don't need to save current interrupt status:
   //   this function never returns, and interrupt status is restored
   //   from different task's stack inside `_tn_arch_context_switch_exit()`
   //   call.
   TN_INTSAVE_DATA;
	 
   if (!tn_is_task_context()){
      goto out;
   }

   TN_INT_DIS_SAVE();

   task = tn_curr_run_task;
   _tn_task_clear_runnable(task);

   _task_terminate(task);

   if ((opts & TN_TASK_EXIT_OPT_DELETE)){
      _task_delete(task);
   }

   //-- interrupts will be enabled inside _tn_arch_context_switch_exit()
   _tn_arch_context_switch_exit();  

out:
   return;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_terminate(struct TN_Task *task)
{
   TN_INTSAVE_DATA;

   enum TN_RCode rc;
	 
   rc = _check_param_generic(task);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   rc = TN_RC_OK;

   if (_tn_task_is_dormant(task)){
      //-- The task is already terminated
      rc = TN_RC_WSTATE;
   } else if (tn_curr_run_task == task){
      //-- Cannot terminate currently running task
      //   (use tn_task_exit() instead)
      rc = TN_RC_WCONTEXT;
   } else {

      if (_tn_task_is_runnable(task)){
         _tn_task_clear_runnable(task);
      } else if (_tn_task_is_waiting(task)){
         _tn_task_clear_waiting(
               task,
               TN_RC_OK    //-- doesn't matter: nobody will read it
               );
      }

      if (_tn_task_is_suspended(task)){
         _tn_task_clear_suspended(task);
      }

      _task_terminate(task);
   }

   TN_INT_RESTORE();
   _tn_switch_context_if_needed();

out:
   return rc;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_delete(struct TN_Task *task)
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc;

   rc = _check_param_generic(task);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   rc = _task_delete(task);

   TN_INT_RESTORE();

out:
   return rc;
}

/*
 * See comments in the header file (tn_tasks.h)
 */
enum TN_RCode tn_task_change_priority(struct TN_Task *task, int new_priority)
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(task);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (new_priority < 0 || new_priority >= (TN_PRIORITIES_CNT - 1)){
      rc = TN_RC_WPARAM;
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   if(new_priority == 0){
      new_priority = task->base_priority;
   }

   rc = TN_RC_OK;

   if (_tn_task_is_dormant(task)){
      rc = TN_RC_WSTATE;
   } else {
      _tn_change_task_priority(task, new_priority);
   }

   TN_INT_RESTORE();
   _tn_switch_context_if_needed();

out:
   return rc;
}





/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

/**
 * See comment in the tn_internal.h file
 */
void _tn_task_set_runnable(struct TN_Task * task)
{
#if TN_DEBUG
   //-- task_state should be NONE here
   if (task->task_state != TN_TASK_STATE_NONE){
      _TN_FATAL_ERROR("");
   }
#endif

   //BOOL ret = FALSE;
   int priority;

   priority          = task->priority;
   task->task_state  |= TN_TASK_STATE_RUNNABLE;
   task->pwait_queue = NULL;

   //-- Add the task to the end of 'ready queue' for the current priority
   _add_entry_to_ready_queue(&(task->task_queue), priority);

   //-- less value - greater priority, so '<' operation is used here
   if (priority < tn_next_task_to_run->priority){
      tn_next_task_to_run = task;
      //ret = TRUE;
   }

   //return ret;
}

/**
 * See comment in the tn_internal.h file
 */
void _tn_task_clear_runnable(struct TN_Task *task)
{
#if TN_DEBUG
   //-- task_state should be exactly TN_TASK_STATE_RUNNABLE here
   if (task->task_state != TN_TASK_STATE_RUNNABLE){
      _TN_FATAL_ERROR("");
   }

   if (task == &tn_idle_task){
      //-- idle task should always be runnable
      _TN_FATAL_ERROR("idle task should always be runnable");
   }
#endif

   //BOOL ret = FALSE;

   int priority;
   priority = task->priority;

   //-- remove runnable state
   task->task_state &= ~TN_TASK_STATE_RUNNABLE;

   //-- remove the curr task from any queue (now - from ready queue)
   if (_remove_entry_from_ready_queue(&(task->task_queue), priority)){
      //-- No ready tasks for the curr priority

      //-- Find highest priority ready to run -
      //-- at least, MSB bit must be set for the idle task

      _find_next_task_to_run();
   } else {
      //-- There are 'ready to run' task(s) for the curr priority

      if (tn_next_task_to_run == task){
         //-- the task that just became non-runnable was the "next task to run",
         //   so we should select new next task to run
         tn_next_task_to_run = get_task_by_tsk_queue(tn_ready_list[priority].next);

         //-- tn_next_task_to_run was just altered, so, we should return TRUE
      }
   }

   //-- and reset task's queue
   tn_list_reset(&(task->task_queue));

}

void _tn_task_set_waiting(
      struct TN_Task *task,
      struct TN_ListItem *wait_que,
      enum TN_WaitReason wait_reason,
      unsigned long timeout
      )
{
#if TN_DEBUG
   //-- only SUSPEND bit is allowed here
   if (task->task_state & ~(TN_TASK_STATE_SUSPEND)){
      _TN_FATAL_ERROR("");
   }
#endif

   task->task_state       |= TN_TASK_STATE_WAIT;
   task->task_wait_reason = wait_reason;
   task->tick_count       = timeout;

   //--- Add to the wait queue  - FIFO

   if (wait_que != NULL){
      tn_list_add_tail(wait_que, &(task->task_queue));
      task->pwait_queue = wait_que;
   } else {
      //-- NOTE: we don't need to reset task_queue because
      //   it is already reset in _tn_task_clear_runnable().
   }

   //--- Add to the timers queue

   if (timeout != TN_WAIT_INFINITE){
      tn_list_add_tail(&tn_wait_timeout_list, &(task->timer_queue));
   }
}

/**
 * See comment in the tn_internal.h file
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

   if (tn_is_list_empty(&task->task_queue) != (task->pwait_queue == NULL)){
      _TN_FATAL_ERROR("task_queue and pwait_queue are out of sync");
   }

#endif

   //-- NOTE: we should remove task from wait_queue before calling
   //   _on_task_wait_complete(), because _find_max_blocked_priority()
   //   in tn_mutex.c checks for all tasks in mutex's wait_queue to
   //   get max blocked priority

   //-- NOTE: we don't care here whether task is contained in any wait_queue,
   //   because even if it isn't, tn_list_remove_entry() on empty list
   //   does just nothing.
   tn_list_remove_entry(&task->task_queue);
   //-- and reset task's queue
   tn_list_reset(&(task->task_queue));

   //-- handle current wait_reason: say, for MUTEX_I, we should
   //   handle priorities of other involved tasks.
   _on_task_wait_complete(task);

   task->pwait_queue  = NULL;
   task->task_wait_rc = wait_rc;

   //-- remove task from timer queue (if it is there)
   if (task->tick_count != TN_WAIT_INFINITE){
      tn_list_remove_entry(&(task->timer_queue));
   }

   task->tick_count = TN_WAIT_INFINITE;

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
   }
#endif

   tn_list_reset(&(task->task_queue));
   tn_list_reset(&(task->timer_queue));

   _init_mutex_queue(task);
   _init_deadlock_list(task);

   task->pwait_queue = NULL;

   task->priority    = task->base_priority; //-- Task curr priority
   task->task_state  |= TN_TASK_STATE_DORMANT;   //-- Task state
   task->task_wait_reason = 0;              //-- Reason for waiting
   task->task_wait_rc = TN_RC_OK;

   task->tick_count    = TN_WAIT_INFINITE;  //-- Remaining time until timeout

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
   task->task_stk = _tn_arch_stack_init(
         task->task_func_addr,
         task->stk_start,
         task->task_func_param
         );

   task->task_state &= ~TN_TASK_STATE_DORMANT;
}

/**
 * See the comment for tn_task_activate, tn_task_iactivate in the tn_tasks.h
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
 * See comment in the tn_internal.h file
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
 * See comment in the tn_internal.h file
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

#if TN_USE_MUTEXES
/**
 * See comment in the tn_internal.h file
 */
BOOL _tn_is_mutex_locked_by_task(struct TN_Task *task, struct TN_Mutex *mutex)
{
   BOOL ret = FALSE;

   struct TN_Mutex *tmp_mutex;
   tn_list_for_each_entry(tmp_mutex, &(task->mutex_queue), mutex_queue){
      if (tmp_mutex == mutex){
         ret = TRUE;
         break;
      }
   }

   return ret;
}

#endif

