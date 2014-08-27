/*

  TNKernel real-time kernel

  Copyright © 2004, 2013 Yuri Tiomkin
  PIC32 version modifications copyright © 2013 Anders Montonen
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

//  The System uses two levels of priorities for the own purpose:
//
//   - level 0                    (highest) for system timers task
//   - level (TN_NUM_PRIORITY-1)  (lowest)  for system idle task



    //---- System's  global variables ----


CDLL_QUEUE tn_ready_list[TN_NUM_PRIORITY];        //-- all ready to run(RUNNABLE) tasks
CDLL_QUEUE tn_create_queue;                       //-- all created tasks
volatile int tn_created_tasks_qty;                //-- num of created tasks
CDLL_QUEUE tn_wait_timeout_list;                  //-- all tasks that wait timeout expiration

unsigned short tn_tslice_ticks[TN_NUM_PRIORITY];  //-- for round-robin only

volatile int tn_system_state;                     //-- System state -(running/not running/etc.)

TN_TCB * tn_next_task_to_run;                     //-- Task to be run after switch context
TN_TCB * tn_curr_run_task;                        //-- Task that is running now

volatile unsigned int tn_ready_to_run_bmp;
volatile unsigned long tn_idle_count;
volatile unsigned long tn_curr_performance;

volatile unsigned int  tn_sys_time_count;

volatile int tn_int_nest_count;

void * tn_user_sp;               //-- Saved task stack pointer
void * tn_int_sp;                //-- Saved ISR stack pointer

//-- System tasks

//-- timer task - priority 0  - highest

TN_TCB  tn_timer_task;
static void tn_timer_task_func(void * par);

//-- idle task - priority (TN_NUM_PRIORITY-1) - lowest

TN_TCB  tn_idle_task;
static void tn_idle_task_func(void * par);


//----------------------------------------------------------------------------
// TN main function (never return)
//----------------------------------------------------------------------------
void tn_start_system(
      unsigned int  *timer_task_stack,
      unsigned int   timer_task_stack_size,
      unsigned int  *idle_task_stack,
      unsigned int   idle_task_stack_size,
      unsigned int  *int_stack,
      unsigned int   int_stack_size
      )
{
   int i;

   //-- Clear/set all globals (vars, lists, etc)

   for(i=0;i < TN_NUM_PRIORITY;i++)
   {
      queue_reset(&(tn_ready_list[i]));
      tn_tslice_ticks[i] = NO_TIME_SLICE;
   }

   queue_reset(&tn_create_queue);
   tn_created_tasks_qty = 0;

   tn_system_state = TN_ST_STATE_NOT_RUN;

   tn_ready_to_run_bmp = 0;

   tn_idle_count       = 0;
   tn_curr_performance = 0;

   tn_next_task_to_run = NULL;
   tn_curr_run_task    = NULL;

   //-- Fill interrupt stack space with TN_FILL_STACK_VAL

   for (i = 0; i < int_stack_size; i++){
      int_stack[i] = TN_FILL_STACK_VAL;
   }

   //-- Pre-decrement stack

   tn_int_sp = &(int_stack[int_stack_size]);

  //-- System tasks

   queue_reset(&tn_wait_timeout_list);

   //--- Timer task

   _tn_task_create((TN_TCB*)&tn_timer_task,       //-- task TCB
                  tn_timer_task_func,             //-- task function
                  0,                              //-- task priority
                  &(timer_task_stack              //-- task stack first addr in memory
                      [timer_task_stack_size-1]),
                  timer_task_stack_size,          //-- task stack size (in int,not bytes)
                  NULL,                           //-- task function parameter
                  TN_TASK_TIMER);                 //-- Creation option

   //--- Idle task

   _tn_task_create((TN_TCB*)&tn_idle_task,        //-- task TCB
                  tn_idle_task_func,              //-- task function
                  TN_NUM_PRIORITY-1,              //-- task priority
                  &(idle_task_stack               //-- task stack first addr in memory
                      [idle_task_stack_size-1]),
                  idle_task_stack_size,           //-- task stack size (in int,not bytes)
                  NULL,                           //-- task function parameter
                  TN_TASK_IDLE);                  //-- Creation option

    //-- Activate timer & idle tasks

   tn_next_task_to_run = &tn_idle_task; //-- Just for the task_to_runnable() proper op

   task_to_runnable(&tn_idle_task);
   task_to_runnable(&tn_timer_task);

   tn_curr_run_task = &tn_idle_task;  //-- otherwise it is NULL

   //-- Run OS - first context switch

   tn_start_exe();
}

//----------------------------------------------------------------------------
static void tn_timer_task_func(void * par)
{
   TN_INTSAVE_DATA
   volatile TN_TCB * task;
   volatile CDLL_QUEUE * curr_que;

   //-- User application init - user's objects initial (tasks etc.) creation

   tn_app_init();

   //-- Enable interrupt here ( include tick int)

   tn_cpu_int_enable();

   //-------------------------------------------------------------------------

   for(;;)
   {

     //------------ OS timer tick -------------------------------------

      tn_disable_interrupt();

      curr_que = tn_wait_timeout_list.next;
      while(curr_que != &tn_wait_timeout_list)
      {
         task = get_task_by_timer_queque((CDLL_QUEUE*)curr_que);
         if(task->tick_count != TN_WAIT_INFINITE)
         {
            if(task->tick_count > 0)
            {
               task->tick_count--;
               if(task->tick_count == 0) //-- Time out expiried
               {
                  queue_remove_entry(&(((TN_TCB*)task)->task_queue));
                  task_wait_complete((TN_TCB*)task);
                  task->task_wait_rc = TERR_TIMEOUT;
               }
            }
         }

         curr_que = curr_que->next;
      }

      task_curr_to_wait_action(NULL,
                               TSK_WAIT_REASON_SLEEP,
                               TN_WAIT_INFINITE);
      tn_enable_interrupt();

      tn_switch_context();
   }
}

//----------------------------------------------------------------------------
//  In fact, this task is always in RUNNABLE state
//----------------------------------------------------------------------------
static void tn_idle_task_func(void * par)
{

#ifdef TN_MEAS_PERFORMANCE
   TN_INTSAVE_DATA
#endif

   for(;;)
   {
#ifdef TN_MEAS_PERFORMANCE
      tn_disable_interrupt();
#endif

      tn_idle_count++;

#ifdef TN_MEAS_PERFORMANCE
      tn_enable_interrupt();
#endif
   }
}

//--- Set time slice ticks value for priority for round-robin scheduling
//--- If value is NO_TIME_SLICE there are no round-robin scheduling
//--- for tasks with priority. NO_TIME_SLICE is default value.
//----------------------------------------------------------------------------
int tn_sys_tslice_ticks(int priority,int value)
{
   TN_INTSAVE_DATA

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(priority <= 0 || priority >= TN_NUM_PRIORITY-1 ||
                                value < 0 || value > MAX_TIME_SLICE)
      return TERR_WRONG_PARAM;

   tn_tslice_ticks[priority] = value;

   tn_enable_interrupt();
   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
void  tn_tick_int_processing()
{
   TN_INTSAVE_DATA_INT

   volatile CDLL_QUEUE * curr_que;   //-- Need volatile here only to solve
   volatile CDLL_QUEUE * pri_queue;  //-- IAR(c) compiler's high optimization mode problem
   volatile int priority;

   TN_CHECK_INT_CONTEXT_NORETVAL

   tn_idisable_interrupt();

//-------  Round -robin (if is used)

   priority  = tn_curr_run_task->priority;

   if(tn_tslice_ticks[priority] != NO_TIME_SLICE)
   {
      tn_curr_run_task->tslice_count++;
      if(tn_curr_run_task->tslice_count > tn_tslice_ticks[priority])
      {
         tn_curr_run_task->tslice_count = 0;

         pri_queue = &(tn_ready_list[priority]);
        //-- If ready queue is not empty and qty  of queue's tasks > 1
         if(!(is_queue_empty((CDLL_QUEUE *)pri_queue)) && pri_queue->next->next != pri_queue)
         {
            // v.2.7  - Thanks to Vyacheslav Ovsiyenko

           //-- Remove task from head and add it to the tail of
           //-- ready queue for current priority

            curr_que = queue_remove_head(&(tn_ready_list[priority]));
            queue_add_tail(&(tn_ready_list[priority]),(CDLL_QUEUE *)curr_que);
         }
      }
   }

   //-- increment system timer
   tn_sys_time_count++;

   //-- Enable a task with priority 0 - tn_timer_task

   queue_remove_entry(&(tn_timer_task.task_queue));
   tn_timer_task.task_wait_reason = 0;
   tn_timer_task.task_state       = TSK_STATE_RUNNABLE;
   tn_timer_task.pwait_queue      = NULL;
   tn_timer_task.task_wait_rc     = TERR_NO_ERR;

   queue_add_tail(&(tn_ready_list[0]), &(tn_timer_task.task_queue));
   tn_ready_to_run_bmp |= 1;  // priority 0;

   tn_next_task_to_run = &tn_timer_task;

   tn_ienable_interrupt();  //--  !!! thanks to Audrius Urmanavicius !!!
}


//----------------------------------------------------------------------------
unsigned int tn_sys_time_get(void)
{
   //-- NOTE: it works if only read access to unsigned int is atomic!
   return tn_sys_time_count;
}

void tn_sys_time_set(unsigned int value)
{
   if (tn_inside_int()){
      TN_INTSAVE_DATA_INT;
      tn_idisable_interrupt();
      tn_sys_time_count = value;
      tn_ienable_interrupt();
   } else {
      TN_INTSAVE_DATA;
      tn_disable_interrupt();
      tn_sys_time_count = value;
      tn_enable_interrupt();
   }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


