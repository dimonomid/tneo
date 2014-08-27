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

#include "tn.h"
#include "tn_utils.h"

extern CDLL_QUEUE tn_blocked_tasks_list;

//-----------------------------------------------------------------------------
int tn_task_create(TN_TCB * task,                 //-- task TCB
                 void (*task_func)(void *param),  //-- task function
                 int priority,                    //-- task priority
                 unsigned int * task_stack_start, //-- task stack first addr in memory (bottom)
                 int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
                 void * param,                    //-- task function parameter
                 int option)                      //-- Creation option
{
   TN_INTSAVE_DATA
   int rc;

   unsigned int * ptr_stack;
   int i;

   //-- Light weight checking of system tasks recreation

   if((priority == 0 && ((option & TN_TASK_TIMER) == 0)) ||
       (priority == TN_NUM_PRIORITY-1 && (option & TN_TASK_IDLE) == 0))
      return TERR_WRONG_PARAM;

   if((priority < 0 || priority > TN_NUM_PRIORITY-1) ||
        task_stack_size < TN_MIN_STACK_SIZE ||
        task_func == NULL || task == NULL || task_stack_start == NULL ||
        task->id_task != 0)  //-- recreation
      return TERR_WRONG_PARAM;

   rc = TERR_NO_ERR;

   TN_CHECK_NON_INT_CONTEXT

   if(tn_system_state == TN_ST_STATE_RUNNING)
      tn_disable_interrupt();

   //--- Init task TCB

   task->task_func_addr  = (void*)task_func;
   task->task_func_param = param;
   task->stk_start       = (unsigned int*)task_stack_start;  //-- Base address of task stack space
   task->stk_size        = task_stack_size;                  //-- Task stack size (in bytes)
   task->base_priority   = priority;                         //-- Task base priority
   task->activate_count  = 0;                                //-- Activation request count
   task->id_task         = TN_ID_TASK;

   //-- Fill all task stack space by TN_FILL_STACK_VAL - only inside create_task

   for(ptr_stack = task->stk_start,i = 0;i < task->stk_size; i++)
      *ptr_stack-- = TN_FILL_STACK_VAL;

   task_set_dormant_state(task);

   //--- Init task stack

   ptr_stack = tn_stack_init(task->task_func_addr,
                             task->stk_start,
                             task->task_func_param);

   task->task_stk = ptr_stack;    //-- Pointer to task top of stack,
                                  //-- when not running


   //-- Add task to created task queue

   queue_add_tail(&tn_create_queue,&(task->create_queue));
   tn_created_tasks_qty++;

   if((option & TN_TASK_START_ON_CREATION) != 0)
      task_to_runnable(task);

   if(tn_system_state == TN_ST_STATE_RUNNING)
      tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//  If the task is runnable, it is moved to the SUSPENDED state. If the task
//  is in the WAITING state, it is moved to the WAITING­SUSPENDED state.
//----------------------------------------------------------------------------
int tn_task_suspend(TN_TCB * task)
{
   TN_INTSAVE_DATA
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(task->task_state & TSK_STATE_SUSPEND)
      rc = TERR_OVERFLOW;
   else
   {
      if(task->task_state == TSK_STATE_DORMANT)
         rc = TERR_WSTATE;
      else
      {
         if(task->task_state == TSK_STATE_RUNNABLE)
         {
            task->task_state = TSK_STATE_SUSPEND;
            task_to_non_runnable(task);
            tn_enable_interrupt();
            tn_switch_context();

            return TERR_NO_ERR;
         }
         else
         {
            task->task_state |= TSK_STATE_SUSPEND;
            rc = TERR_NO_ERR;
         }
      }
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_task_resume(TN_TCB * task)
{
   TN_INTSAVE_DATA
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(!(task->task_state & TSK_STATE_SUSPEND))
      rc = TERR_WSTATE;
   else
   {
      if(!(task->task_state & TSK_STATE_WAIT)) //- The task is not in the WAIT-SUSPEND state
      {
         task_to_runnable(task);
         tn_enable_interrupt();
         tn_switch_context();

         return TERR_NO_ERR;
      }
      else  //-- Just remove TSK_STATE_SUSPEND from the task state
      {
         task->task_state &= ~TSK_STATE_SUSPEND;
         rc = TERR_NO_ERR;
      }
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_task_sleep(unsigned long timeout)
{
   TN_INTSAVE_DATA
   int rc;

   if(timeout == 0)
      return  TERR_WRONG_PARAM;

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(tn_curr_run_task->wakeup_count > 0)
   {
      tn_curr_run_task->wakeup_count--;
      rc = TERR_NO_ERR;
   }
   else
   {
      task_curr_to_wait_action(NULL, TSK_WAIT_REASON_SLEEP, timeout);
      tn_enable_interrupt();
      tn_switch_context();
      return  TERR_NO_ERR;
   }

   tn_enable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
int tn_task_wakeup(TN_TCB * task)
{
   TN_INTSAVE_DATA
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(task->task_state == TSK_STATE_DORMANT)
   {
      rc = TERR_WCONTEXT;
   }
   else
   {
      if((task->task_state & TSK_STATE_WAIT) &&
                 task->task_wait_reason == TSK_WAIT_REASON_SLEEP)
      {
         if(task_wait_complete(task))
         {
            tn_enable_interrupt();
            tn_switch_context();
            return TERR_NO_ERR;
         }
         rc = TERR_NO_ERR;
      }
      else
      {      // v.2.7 - Thanks to Eugene Scopal
         //-- Check for 0 - case max wakeup_count value is 1  
         if(task->wakeup_count == 0) //-- if here - the task is
         {                           //-- not in the SLEEP mode
            task->wakeup_count++;
            rc = TERR_NO_ERR;
         }
         else
            rc = TERR_OVERFLOW;
      }
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_task_iwakeup(TN_TCB * task)
{
   TN_INTSAVE_DATA_INT
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

   if(task->task_state == TSK_STATE_DORMANT)
   {
      rc = TERR_WCONTEXT;
   }
   else
   {
      if((task->task_state & TSK_STATE_WAIT) &&
                 task->task_wait_reason == TSK_WAIT_REASON_SLEEP)
      {
         if(task_wait_complete(task))
         {
            tn_ienable_interrupt();
            return TERR_NO_ERR;
         }
         rc = TERR_NO_ERR;
      }
      else
      {     // v.2.7 - Thanks to Eugene Scopal
         if(task->wakeup_count == 0)  //-- if here - the task is
         {                            //-- not in the SLEEP mode
            task->wakeup_count++;
            rc = TERR_NO_ERR;
         }
         else
            rc = TERR_OVERFLOW;
      }
   }

   tn_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_task_activate(TN_TCB * task)
{
   TN_INTSAVE_DATA
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(task->task_state == TSK_STATE_DORMANT)
   {
      task_to_runnable(task);
      tn_enable_interrupt();
      tn_switch_context();

      return TERR_NO_ERR;
   }
   else
   {
      if(task->activate_count == 0)
      {
         task->activate_count++;
         rc = TERR_NO_ERR;
      }
      else
         rc = TERR_OVERFLOW;
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_task_iactivate(TN_TCB * task)
{
   TN_INTSAVE_DATA_INT
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

   if(task->task_state == TSK_STATE_DORMANT)
   {
      task_to_runnable(task);
      tn_ienable_interrupt();

      return TERR_NO_ERR;
   }
   else
   {
      if(task->activate_count == 0)
      {
         task->activate_count++;
         rc = TERR_NO_ERR;
      }
      else
         rc = TERR_OVERFLOW;
   }

   tn_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_task_release_wait(TN_TCB * task)
{
   TN_INTSAVE_DATA
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if((task->task_state & TSK_STATE_WAIT) == 0)
   {
      rc = TERR_WCONTEXT;
   }
   else
   {
      queue_remove_entry(&(task->task_queue));
      if(task_wait_complete(task))
      {
         tn_enable_interrupt();
         tn_switch_context();
         return TERR_NO_ERR;
      }
      rc = TERR_NO_ERR;
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_task_irelease_wait(TN_TCB * task)
{
   TN_INTSAVE_DATA_INT
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

   if((task->task_state & TSK_STATE_WAIT) == 0)
      rc = TERR_WCONTEXT;
   else
   {
      queue_remove_entry(&(task->task_queue));
      task_wait_complete(task);

      rc = TERR_NO_ERR;
   }

   tn_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void tn_task_exit(int attr)
{
	/*  
	  The structure is used to force GCC compiler properly locate and use
   	'stack_exp' - thanks to Angelo R. Di Filippo
	*/
   struct  // v.2.7
   {	
#ifdef USE_MUTEXES
      CDLL_QUEUE * que;
      TN_MUTEX * mutex;
#endif
      TN_TCB * task;
      volatile int stack_exp[TN_PORT_STACK_EXPAND_AT_EXIT];
   }data;
	 
   TN_CHECK_NON_INT_CONTEXT_NORETVAL

#ifdef TNKERNEL_PORT_MSP430X
   __disable_interrupt();
#else
   tn_cpu_save_sr();  //-- For ARM - disable interrupts without saving SPSR
#endif

   //--------------------------------------------------

   //-- Unlock all mutexes, locked by the task

#ifdef USE_MUTEXES
   while(!is_queue_empty(&(tn_curr_run_task->mutex_queue)))
   {
      data.que = queue_remove_head(&(tn_curr_run_task->mutex_queue));
      data.mutex = get_mutex_by_mutex_queque(data.que);
      do_unlock_mutex(data.mutex);
   }
#endif

   data.task = tn_curr_run_task;
   task_to_non_runnable(tn_curr_run_task);

   task_set_dormant_state(data.task);
	 //-- Pointer to task top of stack,when not running
   data.task->task_stk = tn_stack_init(data.task->task_func_addr,
                                  data.task->stk_start,
                                  data.task->task_func_param);

   if(data.task->activate_count > 0)  //-- Cannot exit
   {
      data.task->activate_count--;
      task_to_runnable(data.task);
   }
   else  // V 2.6 Thanks to Alex Borisov
   {
      if(attr == TN_EXIT_AND_DELETE_TASK)
      {
         queue_remove_entry(&(data.task->create_queue));
         tn_created_tasks_qty--;
         data.task->id_task = 0;
      }
   }

   tn_switch_context_exit();  // interrupts will be enabled inside tn_switch_context_exit()
}

//-----------------------------------------------------------------------------
int tn_task_terminate(TN_TCB * task)
{
   TN_INTSAVE_DATA

   int rc;
/* see the structure purpose in tn_task_exit() */
	 struct // v.2.7
	 {
#ifdef USE_MUTEXES
      CDLL_QUEUE * que;
      TN_MUTEX * mutex;
#endif
      volatile int stack_exp[TN_PORT_STACK_EXPAND_AT_EXIT];
   }data; 
	 
#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();


   //--------------------------------------------------

   rc = TERR_NO_ERR;

   if(task->task_state == TSK_STATE_DORMANT || tn_curr_run_task == task)
      rc = TERR_WCONTEXT; //-- Cannot terminate running task
   else
   {
      if(task->task_state == TSK_STATE_RUNNABLE)
         task_to_non_runnable(task);
      else if(task->task_state & TSK_STATE_WAIT)
      {
         //-- Free all queues, involved in the 'waiting'

         queue_remove_entry(&(task->task_queue));

         //-----------------------------------------

         if(task->tick_count != TN_WAIT_INFINITE)
            queue_remove_entry(&(task->timer_queue));
      }

      //-- Unlock all mutexes, locked by the task

#ifdef USE_MUTEXES
      while(!is_queue_empty(&(task->mutex_queue)))
      {
         data.que = queue_remove_head(&(task->mutex_queue));
         data.mutex = get_mutex_by_mutex_queque(data.que);
         do_unlock_mutex(data.mutex);
      }
#endif

      task_set_dormant_state(task);
			//-- Pointer to task top of the stack when not running

      task->task_stk = tn_stack_init(task->task_func_addr,
                                     task->stk_start,
                                     task->task_func_param);
       
      if(task->activate_count > 0) //-- Cannot terminate
      {
         task->activate_count--;

         task_to_runnable(task);
         tn_enable_interrupt();
         tn_switch_context();

         return TERR_NO_ERR;
      }
   }

   tn_enable_interrupt();

   return rc;
}

//-----------------------------------------------------------------------------
int tn_task_delete(TN_TCB * task)
{
   TN_INTSAVE_DATA
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   rc = TERR_NO_ERR;

   if(task->task_state != TSK_STATE_DORMANT)
      rc = TERR_WCONTEXT;  //-- Cannot delete not-terminated task
   else
   {
      queue_remove_entry(&(task->create_queue));
      tn_created_tasks_qty--;
      task->id_task = 0;
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_task_change_priority(TN_TCB * task, int new_priority)
{
   TN_INTSAVE_DATA
   int rc;

#if TN_CHECK_PARAM
   if(task == NULL)
      return  TERR_WRONG_PARAM;
   if(task->id_task != TN_ID_TASK)
      return TERR_NOEXS;
   if(new_priority < 0 || new_priority > TN_NUM_PRIORITY - 2) //-- try to set pri
      return TERR_WRONG_PARAM;                                // reserved by sys
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(new_priority == 0)
      new_priority = task->base_priority;

   rc = TERR_NO_ERR;

   if(task->task_state == TSK_STATE_DORMANT)
      rc = TERR_WCONTEXT;
   else if(task->task_state == TSK_STATE_RUNNABLE)
   {
      if(change_running_task_priority(task,new_priority))
      {
         tn_enable_interrupt();
         tn_switch_context();
         return TERR_NO_ERR;
      }
   }
   else
   {
      task->priority = new_priority;
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//  Utilities
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void find_next_task_to_run(void)
{
   int tmp;

#ifndef USE_ASM_FFS
   int i;
   unsigned int mask;
#endif

#ifdef USE_ASM_FFS
   tmp = ffs_asm(tn_ready_to_run_bmp);
   tmp--;
#else
   mask = 1;
   tmp = 0;
   for(i=0; i < TN_BITS_IN_INT; i++)  //-- for each bit in bmp
   {
      if(tn_ready_to_run_bmp & mask)
      {
         tmp = i;
         break;
      }
      mask = mask<<1;
   }
#endif

   tn_next_task_to_run = get_task_by_tsk_queue(tn_ready_list[tmp].next);
}

//-----------------------------------------------------------------------------
void task_to_non_runnable(TN_TCB * task)
{
   int priority;
   CDLL_QUEUE * que;

   priority = task->priority;
   que = &(tn_ready_list[priority]);

   //-- remove the curr task from any queue (now - from ready queue)

   queue_remove_entry(&(task->task_queue));

   if(is_queue_empty(que))  //-- No ready tasks for the curr priority
   {
      //-- remove 'ready to run' from the curr priority

      tn_ready_to_run_bmp &= ~(1<<priority);

      //-- Find highest priority ready to run -
      //-- at least, MSB bit must be set for the idle task

      find_next_task_to_run();   //-- v.2.6
   }
   else //-- There are 'ready to run' task(s) for the curr priority
   {
      if(tn_next_task_to_run == task)
         tn_next_task_to_run = get_task_by_tsk_queue(que->next);
   }
}

//----------------------------------------------------------------------------
void task_to_runnable(TN_TCB * task)
{
   int priority;

   priority          = task->priority;
   task->task_state  = TSK_STATE_RUNNABLE;
   task->pwait_queue = NULL;

   //-- Add the task to the end of 'ready queue' for the current priority

   queue_add_tail(&(tn_ready_list[priority]), &(task->task_queue));
   tn_ready_to_run_bmp |= 1 << priority;

   //-- less value - greater priority, so '<' operation is used here

   if(priority < tn_next_task_to_run->priority)
      tn_next_task_to_run = task;
}

//----------------------------------------------------------------------------
int task_wait_complete(TN_TCB * task) //-- v. 2.6
{
#ifdef USE_MUTEXES
   int fmutex;
   int curr_priority;
   TN_MUTEX * mutex;
   TN_TCB * mt_holder_task;
   CDLL_QUEUE * t_que;
#endif
   int rc = FALSE;

   if(task == NULL)
      return 0;

#ifdef USE_MUTEXES

   t_que = NULL;
   if(task->task_wait_reason == TSK_WAIT_REASON_MUTEX_I ||
         task->task_wait_reason == TSK_WAIT_REASON_MUTEX_C)
   {
      fmutex = TRUE;
      t_que = task->pwait_queue;
   }
   else
      fmutex = FALSE;

#endif

   task->pwait_queue  = NULL;
   task->task_wait_rc = TERR_NO_ERR;

   if(task->tick_count != TN_WAIT_INFINITE)
      queue_remove_entry(&(task->timer_queue));

   task->tick_count = TN_WAIT_INFINITE;

   if(!(task->task_state & TSK_STATE_SUSPEND))
   {
      task_to_runnable(task);
      rc = TRUE;
   }
   else  //-- remove WAIT state
      task->task_state = TSK_STATE_SUSPEND;


#ifdef USE_MUTEXES

   if(fmutex)
   {
      mutex = get_mutex_by_wait_queque(t_que);

      mt_holder_task = mutex->holder;
      if(mt_holder_task != NULL)
      {
         //-- if task was blocked by another task and its pri was changed
         //--  - recalc current priority

         if(mt_holder_task->priority != mt_holder_task->base_priority &&
             mt_holder_task->priority == task->priority)
         {
            curr_priority = find_max_blocked_priority(mutex,
                                             mt_holder_task->base_priority);

            set_current_priority(mt_holder_task, curr_priority);
            rc = TRUE;
         }
      }
   }
#endif

   task->task_wait_reason = 0; //-- Clear wait reason

   return rc;
}

//----------------------------------------------------------------------------
void task_curr_to_wait_action(CDLL_QUEUE * wait_que,
                              int wait_reason,
                              unsigned long timeout)
{
   task_to_non_runnable(tn_curr_run_task);

   tn_curr_run_task->task_state       = TSK_STATE_WAIT;
   tn_curr_run_task->task_wait_reason = wait_reason;
   tn_curr_run_task->tick_count       = timeout;

   //--- Add to the wait queue  - FIFO

   if(wait_que == NULL) //-- Thanks to Vavilov D.O.
   {
      queue_reset(&(tn_curr_run_task->task_queue));
   }
   else
   {
      queue_add_tail(wait_que, &(tn_curr_run_task->task_queue));
      tn_curr_run_task->pwait_queue = wait_que;
   }

   //--- Add to the timers queue

   if(timeout != TN_WAIT_INFINITE)
      queue_add_tail(&tn_wait_timeout_list, &(tn_curr_run_task->timer_queue));
}

//----------------------------------------------------------------------------
int change_running_task_priority(TN_TCB * task, int new_priority)
{
   int old_priority;

   old_priority = task->priority;

  //-- remove curr task from any (wait/ready) queue

   queue_remove_entry(&(task->task_queue));

  //-- If there are no ready tasks for the old priority
  //-- clear ready bit for old priority

   if(is_queue_empty(&(tn_ready_list[old_priority])))
      tn_ready_to_run_bmp &= ~(1<<old_priority);

   task->priority = new_priority;

  //-- Add task to the end of ready queue for current priority

   queue_add_tail(&(tn_ready_list[new_priority]), &(task->task_queue));
   tn_ready_to_run_bmp |= 1 << new_priority;
   find_next_task_to_run();

   return TRUE;
}

#ifdef USE_MUTEXES

//----------------------------------------------------------------------------
void set_current_priority(TN_TCB * task, int priority)
{
   TN_MUTEX * mutex;

   //-- transitive priority changing

   // if we have a task A that is blocked by the task B and we changed priority
   // of task A,priority of task B also have to be changed. I.e, if we have
   // the task A that is waiting for the mutex M1 and we changed priority
   // of this task, a task B that holds a mutex M1 now, also needs priority's
   // changing.  Then, if a task B now is waiting for the mutex M2, the same
   // procedure have to be applied to the task C that hold a mutex M2 now
   // and so on.


   //-- This function in ver 2.6 is more "lightweight".
   //-- The code is derived from Vyacheslav Ovsiyenko version

   for(;;)
   {
      if(task->priority <= priority)
         return;

      if(task->task_state == TSK_STATE_RUNNABLE)
      {
         change_running_task_priority(task, priority);
         return;
      }

      if(task->task_state & TSK_STATE_WAIT)
      {
         if(task->task_wait_reason == TSK_WAIT_REASON_MUTEX_I)
         {
            task->priority = priority;

            mutex = get_mutex_by_wait_queque(task->pwait_queue);
            task  = mutex->holder;

            continue;
         }
      }

      task->priority = priority;
      return;
   }
}

#endif

//----------------------------------------------------------------------------
void task_set_dormant_state(TN_TCB* task)
{
	     // v.2.7 - thanks to Alexander Gacov, Vyacheslav Ovsiyenko
   queue_reset(&(task->task_queue));
   queue_reset(&(task->timer_queue));

#ifdef USE_MUTEXES

   queue_reset(&(task->mutex_queue));

#endif

   task->pwait_queue = NULL;

   task->priority    = task->base_priority; //-- Task curr priority
   task->task_state  = TSK_STATE_DORMANT;   //-- Task state
   task->task_wait_reason = 0;              //-- Reason for waiting
   task->task_wait_rc = TERR_NO_ERR;

#ifdef USE_EVENTS

   task->ewait_pattern = 0;                 //-- Event wait pattern
   task->ewait_mode    = 0;                 //-- Event wait mode:  _AND or _OR

#endif

   task->data_elem     = NULL;              //-- Store data queue entry,if data queue is full

   task->tick_count    = TN_WAIT_INFINITE;  //-- Remaining time until timeout
   task->wakeup_count  = 0;                 //-- Wakeup request count
   task->suspend_count = 0;                 //-- Suspension count

   task->tslice_count  = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

