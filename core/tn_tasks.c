/*

  TNKernel real-time kernel

  Copyright © 2004, 2013 Yuri Tiomkin
  All rights reserved.

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

  /* ver 2.7 */

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

extern struct TN_ListItem tn_blocked_tasks_list;



/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Depending on TN_API_TASK_CREATE, provinde proper method for getting
 * bottom stack address from user-provided stack pointer.
 */
#if (!defined TN_API_TASK_CREATE)
#  error TN_API_TASK_CREATE is not defined
#elif (!defined TN_API_TASK_CREATE__NATIVE)
#  error TN_API_TASK_CREATE__NATIVE is not defined
#elif (!defined TN_API_TASK_CREATE__CONVENIENT)
#  error TN_API_TASK_CREATE__CONVENIENT is not defined
#endif

#if (TN_API_TASK_CREATE == TN_API_TASK_CREATE__NATIVE)
static inline unsigned int *_stk_bottom_get(unsigned int *user_provided_addr, int task_stack_size)
{
   return user_provided_addr;
}
#elif (TN_API_TASK_CREATE == TN_API_TASK_CREATE__CONVENIENT)
static inline unsigned int *_stk_bottom_get(unsigned int *user_provided_addr, int task_stack_size)
{
   return user_provided_addr + task_stack_size - 1;
}
#else
#  error wrong TN_API_TASK_CREATE
#endif

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
static BOOL _find_next_task_to_run(void)
{
   BOOL ret = FALSE;

   int priority;

#ifdef USE_ASM_FFS
   priority = ffs_asm(tn_ready_to_run_bmp);
   priority--;
#else
   int i;
   unsigned int mask;

   mask = 1;
   priority = 0;

   for (i = 0; i < TN_BITS_IN_INT; i++){
      //-- for each bit in bmp
      if (tn_ready_to_run_bmp & mask){
         priority = i;
         break;
      }
      mask = (mask << 1);
   }
#endif

   {
      struct TN_Task *task_to_run;
      task_to_run = get_task_by_tsk_queue(tn_ready_list[priority].next);

      if (task_to_run != tn_next_task_to_run){
         tn_next_task_to_run = task_to_run;
         ret = TRUE;
      }
   }

   return ret;
}

static void _task_set_dormant_state(struct TN_Task* task)
{
   // v.2.7 - thanks to Alexander Gacov, Vyacheslav Ovsiyenko
   tn_list_reset(&(task->task_queue));
   tn_list_reset(&(task->timer_queue));

   _init_mutex_queue(task);
   _init_deadlock_list(task);

   task->pwait_queue = NULL;

   task->priority    = task->base_priority; //-- Task curr priority
   task->task_state  = TSK_STATE_DORMANT;   //-- Task state
   task->task_wait_reason = 0;              //-- Reason for waiting
   task->task_wait_rc = TERR_NO_ERR;

#if TN_USE_EVENTS

   task->ewait_pattern = 0;                 //-- Event wait pattern
   task->ewait_mode    = 0;                 //-- Event wait mode:  _AND or _OR

#endif

   task->data_elem     = NULL;              //-- Store data queue entry,if data queue is full

   task->tick_count    = TN_WAIT_INFINITE;  //-- Remaining time until timeout
   task->wakeup_count  = 0;                 //-- Wakeup request count
   task->suspend_count = 0;                 //-- Suspension count

   task->tslice_count  = 0;
}

// }}}

//-- Inline functions {{{


/**
 * See the comment for tn_task_wakeup, tn_task_iwakeup in the tn_tasks.h
 */
static inline enum TN_Retval _task_wakeup(struct TN_Task *task, int *p_need_switch_context)
{
   enum TN_Retval rc = TERR_NO_ERR;
   *p_need_switch_context = 0;

   if (task->task_state & TSK_STATE_DORMANT){
      rc = TERR_WCONTEXT;
   } else {

      if (     (task->task_state & TSK_STATE_WAIT)
            && (task->task_wait_reason == TSK_WAIT_REASON_SLEEP))
      {
         //-- Task is sleeping, so, let's wake it up.

         *p_need_switch_context = _tn_task_wait_complete(task, (TN_WCOMPL__TO_RUNNABLE));
      } else {
         //-- Task isn't sleeping. Probably it is in WAIT state,
         //   but not because of call to tn_task_sleep().

         //-- Check for 0 - case max wakeup_count value is 1  
         if (task->wakeup_count == 0){                           
            //-- there isn't wakeup request yet, so, increment counter
            task->wakeup_count++;
         } else {
            //-- too many wakeup requests; return error.
            rc = TERR_OVERFLOW;
         }
      }
   }

   return rc;
}

static inline enum TN_Retval _task_release_wait(struct TN_Task *task, int *p_need_switch_context)
{
   enum TN_Retval rc = TERR_NO_ERR;
   *p_need_switch_context = 0;

   if ((task->task_state & TSK_STATE_WAIT)){
      //-- task is in WAIT state, so, let's release it from that state.
      *p_need_switch_context = _tn_task_wait_complete(task, (TN_WCOMPL__TO_RUNNABLE));
   } else {
      rc = TERR_WCONTEXT;
   }

   return rc;
}

/**
 * See the comment for tn_task_activate, tn_task_iactivate in the tn_tasks.h
 */
static inline enum TN_Retval _task_activate(struct TN_Task *task, int *p_need_switch_context)
{
   enum TN_Retval rc = TERR_NO_ERR;
   *p_need_switch_context = 0;

   if (task->task_state & TSK_STATE_DORMANT){
      _tn_task_to_runnable(task);
      *p_need_switch_context = 1;
   } else {
      if (task->activate_count == 0){
         task->activate_count++;
      } else {
         rc = TERR_OVERFLOW;
      }
   }

   return rc;
}

static inline enum TN_Retval _task_job_perform(
      struct TN_Task *task,
      int (p_worker)(struct TN_Task *task, int *p_need_switch_context)
      )
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc = TERR_NO_ERR;
   int need_switch_context = 0;

#if TN_CHECK_PARAM
   if(task == NULL)
      return TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = p_worker(task, &need_switch_context);

   tn_enable_interrupt();
   if (need_switch_context){
      tn_switch_context();
   }

   return rc;
}

static inline enum TN_Retval _task_job_iperform(
      struct TN_Task *task,
      int (p_worker)(struct TN_Task *task, int *p_need_switch_context)
      )
{
   TN_INTSAVE_DATA_INT;
   enum TN_Retval rc = TERR_NO_ERR;
   int need_switch_context = 0;

#if TN_CHECK_PARAM
   if(task == NULL)
      return TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT;

   tn_idisable_interrupt();

   rc = p_worker(task, &need_switch_context);

   tn_ienable_interrupt();
   //-- inside interrupt, we ignore need_switch_context

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
 *         is set to TSK_WAIT_REASON_MUTEX_I/.._C
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
   if (task->task_wait_reason == TSK_WAIT_REASON_MUTEX_I){
      _tn_mutex_i_on_task_wait_complete(task);
   }

   //-- for any mutex, call special handler
   if (     (task->task_wait_reason == TSK_WAIT_REASON_MUTEX_I)
         || (task->task_wait_reason == TSK_WAIT_REASON_MUTEX_C)
      )
   {
      _tn_mutex_on_task_wait_complete(task);
   }

}

/**
 * Teminate task:
 *    * unlock all mutexes that are held by task
 *    * set dormant state (reinitialize everything)
 *    * reitinialize stack
 *    * handle activate_count TODO do we need this?
 *
 * @return TRUE if task can be terminated (activate_count is 0),
 *         FALSE otherwise
 */
static BOOL _task_terminate(struct TN_Task *task)
{
   BOOL ret = TRUE;

   //-- Unlock all mutexes locked by the task
   _tn_mutex_unlock_all_by_task(task);

   _task_set_dormant_state(task);

   //-- Pointer to task top of stack, when not running
   task->task_stk = tn_stack_init(task->task_func_addr,
         task->stk_start,
         task->task_func_param);

   if (task->activate_count > 0){
      //-- Cannot terminate
      ret = FALSE;
      task->activate_count--;
   }

   return ret;
}


/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * Create task. See comments in tn_tasks.h file.
 */
enum TN_Retval tn_task_create(struct TN_Task *task,                  //-- task TCB
                 void (*task_func)(void *param),  //-- task function
                 int priority,                    //-- task priority
                 unsigned int *task_stack_start,  //-- task stack first addr in memory (see option TN_API_TASK_CREATE)
                 int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
                 void *param,                     //-- task function parameter
                 int option                       //-- creation option
                 )                      
{
   return _tn_task_create(task, task_func, priority,
                          _stk_bottom_get(task_stack_start, task_stack_size),
                          task_stack_size, param, option
                         );
}



/**
 * If the task is runnable, it is moved to the SUSPENDED state. If the task
 * is in the WAITING state, it is moved to the WAITING­SUSPENDED state.
 */
enum TN_Retval tn_task_suspend(struct TN_Task *task)
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc = TERR_NO_ERR;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   if (task->task_state & TSK_STATE_SUSPEND){
      rc = TERR_OVERFLOW;
      goto out_ei;
   }

   if (task->task_state & TSK_STATE_DORMANT){
      rc = TERR_WSTATE;
      goto out_ei;
   }

   task->task_state |= TSK_STATE_SUSPEND;

   if (task->task_state & TSK_STATE_RUNNABLE){
      if (_tn_task_to_non_runnable(task)){
         //-- context switch is needed
         goto out_ei_switch_context;
      } else {
         //-- context switch is not needed
         goto out_ei;
      }
   } else {
      goto out_ei;
   }

out_ei:
   tn_enable_interrupt();
   return rc;

out_ei_switch_context:
   tn_enable_interrupt();
   tn_switch_context();

   return rc;
}

/**
 * Release task from SUSPENDED state. If the given task is in the SUSPENDED state,
 * it is moved to READY state; afterwards it has the lowest precedence amoung
 * runnable tasks with the same priority. If the task is in WAITING_SUSPENDED state,
 * it is moved to WAITING state.
 */
enum TN_Retval tn_task_resume(struct TN_Task *task)
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc = TERR_NO_ERR;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   if (!(task->task_state & TSK_STATE_SUSPEND)){
      rc = TERR_WSTATE;
      goto out_ei;
   }

   if (!(task->task_state & TSK_STATE_WAIT)){
      //-- The task is not in the WAIT-SUSPEND state,
      //   so we need to make it runnable and probably switch context
      if (_tn_task_to_runnable(task)){
         //-- context switch is needed
         goto out_ei_switch_context;
      } else {
         //-- context switch is not needed
         goto out_ei;
      }
   } else {
      //-- Just remove TSK_STATE_SUSPEND from the task state
      task->task_state &= ~TSK_STATE_SUSPEND;
      goto out_ei;
   }

out_ei:
   tn_enable_interrupt();
   return rc;

out_ei_switch_context:
   tn_enable_interrupt();
   tn_switch_context();

   return rc;
}

/**
 * Put current task to sleep for at most timeout ticks. When the timeout
 * expires and the task was not suspended during the sleep, it is switched
 * to runnable state. If the timeout value is TN_WAIT_INFINITE and the task
 * was not suspended during the sleep, the task will sleep until another
 * function call (like tn_task_wakeup() or similar) will make it runnable.
 *
 * Each task has a wakeup request counter. If its value for currently
 * running task is greater then 0, the counter is decremented by 1 and the
 * currently running task is not switched to the sleeping mode and
 * continues execution.
 */
enum TN_Retval tn_task_sleep(unsigned long timeout)
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc;

   if (timeout == 0){
      return TERR_WRONG_PARAM;
   }

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   if (tn_curr_run_task->wakeup_count > 0){
      tn_curr_run_task->wakeup_count--;
      rc = TERR_NO_ERR;
      goto out_ei;
   } else {
      _tn_task_curr_to_wait_action(NULL, TSK_WAIT_REASON_SLEEP, timeout);
      rc = TERR_NO_ERR;
      //-- task was just put to sleep, so we need to switch context
      goto out_ei_switch_context;
   }

out_ei:
   tn_enable_interrupt();
   return rc;

out_ei_switch_context:
   tn_enable_interrupt();
   tn_switch_context();

   return rc;
}

/**
 * See comments in the file tn_tasks.h .
 *
 * This function merely performs little checks, disables interrupts
 * and calls _task_wakeup() function, in which real job is done.
 * It then re-enables interrupts, switches context if needed, and returns.
 */
enum TN_Retval tn_task_wakeup(struct TN_Task *task)
{
   return _task_job_perform(task, _task_wakeup);
}

/**
 * See comments in the file tn_tasks.h .
 *
 * This function merely performs little checks, disables interrupts
 * and calls _task_wakeup() function, in which real job is done.
 * It then re-enables interrupts and returns.
 */
enum TN_Retval tn_task_iwakeup(struct TN_Task *task)
{
   return _task_job_iperform(task, _task_wakeup);
}

/**
 * See comments in the file tn_tasks.h .
 *
 * This function merely performs little checks, disables interrupts
 * and calls _task_activate() function, in which real job is done.
 * It then re-enables interrupts, switches context if needed, and returns.
 */
enum TN_Retval tn_task_activate(struct TN_Task *task)
{
   return _task_job_perform(task, _task_activate);
}

/**
 * See comments in the file tn_tasks.h .
 *
 * This function merely performs little checks, disables interrupts
 * and calls _task_activate() function, in which real job is done.
 * It then re-enables interrupts and returns.
 */
enum TN_Retval tn_task_iactivate(struct TN_Task *task)
{
   return _task_job_iperform(task, _task_activate);
}

/**
 * See comments in the file tn_tasks.h .
 *
 * This function merely performs little checks, disables interrupts
 * and calls _task_release_wait() function, in which real job is done.
 * It then re-enables interrupts, switches context if needed, and returns.
 */
enum TN_Retval tn_task_release_wait(struct TN_Task *task)
{
   return _task_job_perform(task, _task_release_wait);
}

/**
 * See comments in the file tn_tasks.h .
 *
 * This function merely performs little checks, disables interrupts
 * and calls _task_release_wait() function, in which real job is done.
 * It then re-enables interrupts and returns.
 */
enum TN_Retval tn_task_irelease_wait(struct TN_Task *task)
{
   return _task_job_iperform(task, _task_release_wait);
}

/**
 * See comments in the file tn_tasks.h .
 */
void tn_task_exit(int attr)
{
#if 0
	/*  
	 * The structure is used to force GCC compiler properly locate and use
    * 'stack_exp' - thanks to Angelo R. Di Filippo
	 */
   struct  // v.2.7
   {	
      volatile int stack_exp[TN_PORT_STACK_EXPAND_AT_EXIT];
   } data;
#endif

   struct TN_Task *task;
	 
   TN_CHECK_NON_INT_CONTEXT_NORETVAL;

#ifdef TNKERNEL_PORT_MSP430X
   __disable_interrupt();
#else
   tn_cpu_save_sr();  //-- For ARM - disable interrupts without saving SPSR
#endif

   //--------------------------------------------------

   task = tn_curr_run_task;
   _tn_task_to_non_runnable(task);

   if (!_task_terminate(task)){
      //-- Cannot exit
      _tn_task_to_runnable(task);
   } else {
      // V 2.6 Thanks to Alex Borisov
      if (attr == TN_EXIT_AND_DELETE_TASK){
         tn_list_remove_entry(&(task->create_queue));
         tn_created_tasks_qty--;
         task->id_task = 0;
      }
   }

   tn_switch_context_exit();  // interrupts will be enabled inside tn_switch_context_exit()
}

/**
 * See comments in the file tn_tasks.h .
 */
enum TN_Retval tn_task_terminate(struct TN_Task *task)
{
   TN_INTSAVE_DATA;

   enum TN_Retval rc;
	 
#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();


   //--------------------------------------------------

   rc = TERR_NO_ERR;

   if (task->task_state & TSK_STATE_DORMANT){
      //-- The task is already terminated
      rc = TERR_WCONTEXT;
   } else if (tn_curr_run_task == task){
      //-- Cannot terminate currently running task
      //   (use tn_task_exit() instead)
      rc = TERR_WCONTEXT;
   } else {
      if (task->task_state & TSK_STATE_RUNNABLE){
         _tn_task_to_non_runnable(task);

      } else if (task->task_state & TSK_STATE_WAIT){
         //-- call _tn_task_wait_complete() for task
         //   without making it runnable
         //   (flag TN_WCOMPL__TO_RUNNABLE is not set)
         _tn_task_wait_complete(
               task, (TN_WCOMPL__REMOVE_WQUEUE)
               );
      }

      if (!_task_terminate(task)){
         //-- Cannot terminate

         _tn_task_to_runnable(task);
         tn_enable_interrupt();
         tn_switch_context();

         return TERR_NO_ERR;
      }
   }

   tn_enable_interrupt();

   return rc;
}

/**
 * See comments in the file tn_tasks.h .
 */
enum TN_Retval tn_task_delete(struct TN_Task *task)
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = TERR_NO_ERR;

   if (!(task->task_state & TSK_STATE_DORMANT)){
      //-- Cannot delete not-terminated task
      rc = TERR_WCONTEXT;
   } else {
      tn_list_remove_entry(&(task->create_queue));
      tn_created_tasks_qty--;
      task->id_task = 0;
   }

   tn_enable_interrupt();

   return rc;
}

/**
 * Set new priority for task.
 * If priority is 0, then task's base_priority is set.
 */
enum TN_Retval tn_task_change_priority(struct TN_Task *task, int new_priority)
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc;

#if TN_CHECK_PARAM
   if (task == NULL)
      return  TERR_WRONG_PARAM;
   if (task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
   if (new_priority < 0 || new_priority >= (TN_NUM_PRIORITY - 1))
      return TERR_WRONG_PARAM; //-- tried to set priority reverved by system
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   if(new_priority == 0){
      new_priority = task->base_priority;
   }

   rc = TERR_NO_ERR;

   if (task->task_state & TSK_STATE_DORMANT){
      rc = TERR_WCONTEXT;
   } else if (task->task_state & TSK_STATE_RUNNABLE){
      if (_tn_change_running_task_priority(task,new_priority)){
         tn_enable_interrupt();
         tn_switch_context();
         return TERR_NO_ERR;
      }
   } else {
      task->priority = new_priority;
   }

   tn_enable_interrupt();

   return rc;
}





/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

enum TN_Retval _tn_task_create(struct TN_Task *task,                 //-- task TCB
                 void (*task_func)(void *param),  //-- task function
                 int priority,                    //-- task priority
                 unsigned int *task_stack_bottom, //-- task stack first addr in memory (bottom)
                 int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
                 void *param,                     //-- task function parameter
                 int option                       //-- Creation option
      )
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc;

   unsigned int * ptr_stack;
   int i;

   //-- Lightweight checking of system tasks recreation
   if (0
         || (priority == 0                     && !(option & TN_TASK_TIMER))
         || (priority == (TN_NUM_PRIORITY - 1) && !(option & TN_TASK_IDLE))
      )
   {
      return TERR_WRONG_PARAM;
   }

   if (0
         || (priority < 0 || priority > TN_NUM_PRIORITY-1)
         || task_stack_size < TN_MIN_STACK_SIZE
         || task_func == NULL
         || task == NULL
         || task_stack_bottom == NULL
         || task->id_task != 0      //-- task recreation
      )
   {
      
      return TERR_WRONG_PARAM;
   }

   rc = TERR_NO_ERR;

   TN_CHECK_NON_INT_CONTEXT;

   if (tn_sys_state & TN_STATE_FLAG__SYS_RUNNING){
      tn_disable_interrupt();
   }

   //--- Init task TCB

   task->task_func_addr  = (void*)task_func;
   task->task_func_param = param;
   task->stk_start       = task_stack_bottom;                //-- Base address of task stack space
   task->stk_size        = task_stack_size;                  //-- Task stack size (in bytes)
   task->base_priority   = priority;                         //-- Task base priority
   task->activate_count  = 0;                                //-- Activation request count
   task->id_task         = TN_ID_TASK;

   //-- Fill all task stack space by TN_FILL_STACK_VAL - only inside create_task

   for (ptr_stack = task->stk_start, i = 0; i < task->stk_size; i++){
      *ptr_stack-- = TN_FILL_STACK_VAL;
   }

   _task_set_dormant_state(task);

   //--- Init task stack
   ptr_stack = tn_stack_init(task->task_func_addr,
                             task->stk_start,
                             task->task_func_param);

   task->task_stk = ptr_stack;    //-- Pointer to task top of stack,
                                  //-- when not running


   //-- Add task to created task queue

   tn_list_add_tail(&tn_create_queue,&(task->create_queue));
   tn_created_tasks_qty++;

   if ((option & TN_TASK_START_ON_CREATION) != 0){
      _tn_task_to_runnable(task);
   }

   if (tn_sys_state & TN_STATE_FLAG__SYS_RUNNING){
      tn_enable_interrupt();
   }

   return rc;
}

/**
 * See comment in the tn_internal.h file
 */
BOOL _tn_task_wait_complete(struct TN_Task *task, enum TN_WComplFlags flags)
{
   BOOL rc = FALSE;

   if (task == NULL){
      return FALSE;
   }

   //-- NOTE: we should remove task from wait_queue before calling
   //   _on_task_wait_complete(), because _find_max_blocked_priority()
   //   in tn_mutex.c checks for all tasks in mutex's wait_queue to
   //   get max blocked priority
   if (flags & TN_WCOMPL__REMOVE_WQUEUE){
      tn_list_remove_entry(&task->task_queue);
   }

   //-- handle current wait_reason: say, for MUTEX_I, we should
   //   handle priorities of other involved tasks.
   _on_task_wait_complete(task);

   task->pwait_queue  = NULL;
   task->task_wait_rc = TERR_NO_ERR;

   //-- remove task from timer queue (if it is there)
   if (task->tick_count != TN_WAIT_INFINITE){
      tn_list_remove_entry(&(task->timer_queue));
   }

   task->tick_count = TN_WAIT_INFINITE;

   //-- remove WAIT state
   task->task_state &= ~TSK_STATE_WAIT;

   if (flags & TN_WCOMPL__TO_RUNNABLE){
      //-- if task state became NONE when we've removed WAIT state,
      //   let's make it runnable.
      //
      //   NOTE that if task was in WAITING_SUSPENDED state,
      //   there will be TSK_STATE_SUSPEND now.
      //   In this case, _tn_task_to_runnable() won't be called.
      if (task->task_state == TSK_STATE_NONE){
         rc = _tn_task_to_runnable(task);
      }
   }

   //-- Clear wait reason
   task->task_wait_reason = TSK_WAIT_REASON_NONE;

   return rc;
}

/**
 * See comment in the tn_internal.h file
 */
BOOL _tn_task_to_runnable(struct TN_Task * task)
{
   BOOL ret = FALSE;
   int priority;

   priority          = task->priority;
   task->task_state  = TSK_STATE_RUNNABLE;
   task->pwait_queue = NULL;

   //-- Add the task to the end of 'ready queue' for the current priority
   _add_entry_to_ready_queue(&(task->task_queue), priority);

   //-- less value - greater priority, so '<' operation is used here
   if (priority < tn_next_task_to_run->priority){
      tn_next_task_to_run = task;
      ret = TRUE;
   }

   return ret;
}

/**
 * See comment in the tn_internal.h file
 */
BOOL _tn_task_to_non_runnable(struct TN_Task *task)
{
   BOOL ret = FALSE;

   int priority;
   priority = task->priority;

   //-- remove runnable state
   task->task_state &= ~TSK_STATE_RUNNABLE;

   //-- remove the curr task from any queue (now - from ready queue)
   if (_remove_entry_from_ready_queue(&(task->task_queue), priority)){
      //-- No ready tasks for the curr priority

      //-- Find highest priority ready to run -
      //-- at least, MSB bit must be set for the idle task

      ret = _find_next_task_to_run();
   } else {
      //-- There are 'ready to run' task(s) for the curr priority

      if (tn_next_task_to_run == task){
         //-- the task that just became non-runnable was the "next task to run",
         //   so we should select new next task to run
         tn_next_task_to_run = get_task_by_tsk_queue(tn_ready_list[priority].next);

         //-- tn_next_task_to_run was just altered, so, we should return TRUE
         ret = TRUE;
      }
   }

   //-- and reset task's queue
   tn_list_reset(&(task->task_queue));

   return ret;
}



/**
 * See comment in the tn_internal.h file
 */
void _tn_task_curr_to_wait_action(struct TN_ListItem *wait_que,
      enum TN_WaitReason wait_reason,
      unsigned long timeout)
{
   _tn_task_to_non_runnable(tn_curr_run_task);

   tn_curr_run_task->task_state       = TSK_STATE_WAIT;
   tn_curr_run_task->task_wait_reason = wait_reason;
   tn_curr_run_task->tick_count       = timeout;

   //--- Add to the wait queue  - FIFO

   if (wait_que != NULL){
      tn_list_add_tail(wait_que, &(tn_curr_run_task->task_queue));
      tn_curr_run_task->pwait_queue = wait_que;
   } else {
      //-- NOTE: we don't need to reset task_queue because
      //   it is already reset in _tn_task_to_non_runnable().
   }

   //--- Add to the timers queue

   if (timeout != TN_WAIT_INFINITE){
      tn_list_add_tail(&tn_wait_timeout_list, &(tn_curr_run_task->timer_queue));
   }
}

/**
 * See comment in the tn_internal.h file
 */
BOOL _tn_change_task_priority(struct TN_Task *task, int new_priority)
{
   BOOL ret;

   if (task->task_state & TSK_STATE_RUNNABLE){
      ret = _tn_change_running_task_priority(task, new_priority);
   } else {
      task->priority = new_priority;
      ret = TRUE;
   }

   return ret;
}

/**
 * See comment in the tn_internal.h file
 */
BOOL _tn_change_running_task_priority(struct TN_Task *task, int new_priority)
{
   if (!(task->task_state & TSK_STATE_RUNNABLE)){
      TN_FATAL_ERROR("_tn_change_running_task_priority called for non-runnable task");
   }

   //-- remove curr task from any (wait/ready) queue
   _remove_entry_from_ready_queue(&(task->task_queue), task->priority);

   task->priority = new_priority;

   //-- Add task to the end of ready queue for current priority
   _add_entry_to_ready_queue(&(task->task_queue), new_priority);

   return _find_next_task_to_run();
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

