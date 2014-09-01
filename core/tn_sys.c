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

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_common.h"
#include "tn_sys.h"
#include "tn_internal.h"

#include "tn_tasks.h"


//  The System uses two levels of priorities for the own purpose:
//
//   - level 0                    (highest) for system timers task
//   - level (TN_NUM_PRIORITY-1)  (lowest)  for system idle task



/*******************************************************************************
 *    PUBLIC DATA
 ******************************************************************************/

struct tn_que_head tn_ready_list[TN_NUM_PRIORITY];        //-- all ready to run(RUNNABLE) tasks
struct tn_que_head tn_create_queue;                       //-- all created tasks
volatile int tn_created_tasks_qty;                //-- num of created tasks
struct tn_que_head tn_wait_timeout_list;                  //-- all tasks that wait timeout expiration

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

#if TN_USE_TIMER_TASK
//-- timer task - priority (0) - highest
TN_TCB  tn_timer_task;
#endif

//-- idle task - priority (TN_NUM_PRIORITY-1) - lowest

TN_TCB  tn_idle_task;
static void _idle_task_body(void * par);

void (*appl_init_callback)(void);               /* Pointer to user callback app init function */
void (*idle_user_func_callback)(void);          /* Pointer to user idle loop function         */





/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Disable interrupts, call user-provided callback function,
 * enable interrupts back.
 *
 * This function is called by idle task while system is being started.
 */
static inline void _call_appl_callback(void)
{
   //-- Make sure interrupts are disabled before calling application callback,
   //   so that this idle task is guaranteed to not be be preempted
   //   until appl_init_callback() finished its job.
   tn_cpu_int_disable();

   //-- User application init - user's objects initial (tasks etc.) creation
   appl_init_callback();

   //-- Enable interrupt here ( including tick int)
   tn_cpu_int_enable();
}

/**
 * Idle task body. In fact, this task is always in RUNNABLE state.
 */
static void _idle_task_body(void *par)
{

#ifdef TN_MEAS_PERFORMANCE
   TN_INTSAVE_DATA;
#endif

#if !TN_USE_TIMER_TASK
   _call_appl_callback();
#endif

   for(;;)
   {
#ifdef TN_MEAS_PERFORMANCE
      tn_disable_interrupt();
#endif

      idle_user_func_callback();
      tn_idle_count++;

#ifdef TN_MEAS_PERFORMANCE
      tn_enable_interrupt();
#endif
   }
}


/**
 * Manage tn_wait_timeout_list.
 * This job was previously done in tn_timer_task, but now it is preferred
 * to call it right from tn_tick_int_processing()
 */
static inline void _wait_timeout_list_manage(void)
{
   volatile struct tn_que_head *curr_que;
   volatile TN_TCB * task;

   curr_que = tn_wait_timeout_list.next;
   while(curr_que != &tn_wait_timeout_list)
   {
      task = get_task_by_timer_queque((struct tn_que_head*)curr_que);
      if(task->tick_count != TN_WAIT_INFINITE)
      {
         if(task->tick_count > 0)
         {
            task->tick_count--;
            if(task->tick_count == 0) //-- Timeout expired
            {
               queue_remove_entry(&(((TN_TCB *)task)->task_queue));
               _tn_task_wait_complete((TN_TCB *)task);
               task->task_wait_rc = TERR_TIMEOUT;
            }
         }
      }

      curr_que = curr_que->next;
   }
}

/**
 * Manage round-robin (if used)
 */
static inline void _round_robin_manage(void)
{
   volatile struct tn_que_head *curr_que;   //-- Need volatile here only to solve
   volatile struct tn_que_head *pri_queue;  //-- IAR(c) compiler's high optimization mode problem
   volatile int priority = tn_curr_run_task->priority;

   if (tn_tslice_ticks[priority] != NO_TIME_SLICE){
      tn_curr_run_task->tslice_count++;

      if (tn_curr_run_task->tslice_count > tn_tslice_ticks[priority]){
         tn_curr_run_task->tslice_count = 0;

         pri_queue = &(tn_ready_list[priority]);
         //-- If ready queue is not empty and qty  of queue's tasks > 1
         if (!(is_queue_empty((struct tn_que_head *)pri_queue)) && pri_queue->next->next != pri_queue){
            //-- Remove task from head and add it to the tail of
            //-- ready queue for current priority

            curr_que = queue_remove_head(&(tn_ready_list[priority]));
            queue_add_tail(&(tn_ready_list[priority]),(struct tn_que_head *)curr_que);
         }
      }
   }
}

/**
 * Create idle task, the task is NOT started after creation.
 * _idle_task_to_runnable() is used to make it runnable.
 */
static inline void _idle_task_create(unsigned int  *idle_task_stack,
                                     unsigned int   idle_task_stack_size)
{
   _tn_task_create(
         (TN_TCB*)&tn_idle_task,          //-- task TCB
         _idle_task_body,                 //-- task function
         TN_NUM_PRIORITY - 1,             //-- task priority
         &(idle_task_stack                //-- task stack first addr in memory
            [idle_task_stack_size-1]),
         idle_task_stack_size,            //-- task stack size (in int,not bytes)
         NULL,                            //-- task function parameter
         (TN_TASK_IDLE)                   //-- Creation option
         );
}

/**
 * Make idle task runnable.
 */
static inline void _idle_task_to_runnable(void)
{
   _tn_task_to_runnable(&tn_idle_task);
}

//-- obsolete timer task stuff {{{
#if TN_USE_TIMER_TASK
/**
 * Timer task body. Now it is obsolete
 * (it's better not to use timer task at all)
 */
static void _timer_task_body(void * par)
{
   TN_INTSAVE_DATA;

   _call_appl_callback();

   //-------------------------------------------------------------------------

   for(;;){

      //------------ OS timer tick -------------------------------------
      tn_disable_interrupt();

      _wait_timeout_list_manage();

      task_curr_to_wait_action(NULL,
            TSK_WAIT_REASON_SLEEP,
            TN_WAIT_INFINITE);
      tn_enable_interrupt();

      tn_switch_context();
   }
}

/**
 * Create timer task, the task is NOT started after creation.
 * Now it is obsolete (it's better not to use timer task at all)
 */
static inline void _timer_task_create(unsigned int  *timer_task_stack,
      unsigned int   timer_task_stack_size)
{
   _tn_task_create(
         (TN_TCB*)&tn_timer_task,                     //-- task TCB
         _timer_task_body,                            //-- task function
         0,                                           //-- task priority
         &(timer_task_stack                           //-- task stack first addr in memory
            [timer_task_stack_size-1]),
         timer_task_stack_size,                       //-- task stack size (in int,not bytes)
         NULL,                                        //-- task function parameter
         (TN_TASK_TIMER)                              //-- Creation option
         );
}

/**
 * Put timer task on run. Now it is obsolete
 * (it's better not to use timer task at all)
 */
static inline void _timer_task_to_runnable(void)
{
   _tn_task_to_runnable(&tn_timer_task);
}

/**
 * Wake up timer task. This function was called each system tick,
 * but now it is obsolete (it's better not to use timer task at all)
 */
static inline void _timer_task_wakeup(void)
{
   //-- Enable a task with priority 0 - tn_timer_task

   queue_remove_entry(&(tn_timer_task.task_queue));
   tn_timer_task.task_wait_reason = 0;
   tn_timer_task.task_state       = TSK_STATE_RUNNABLE;
   tn_timer_task.pwait_queue      = NULL;
   tn_timer_task.task_wait_rc     = TERR_NO_ERR;

   queue_add_tail(&(tn_ready_list[0]), &(tn_timer_task.task_queue));
   tn_ready_to_run_bmp |= (1 << 0/*priority 0*/);

   tn_next_task_to_run = &tn_timer_task;
}
#else
//-- don't use timer task: just define a couple of stubs
#  define _timer_task_create(timer_task_stack, timer_task_stack_size)
#  define _timer_task_to_runnable()
#  define _timer_task_wakeup()
#endif
// }}}




/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * Main TNKernel function, never returns.
 * Typically called from main().
 */
void tn_start_system(
#if TN_USE_TIMER_TASK
      unsigned int  *timer_task_stack,       //-- pointer to array for timer task stack
      unsigned int   timer_task_stack_size,  //-- size of timer task stack
#endif
      unsigned int  *idle_task_stack,        //-- pointer to array for idle task stack
      unsigned int   idle_task_stack_size,   //-- size of idle task stack
      unsigned int  *int_stack,              //-- pointer to array for interrupt stack
      unsigned int   int_stack_size,         //-- size of interrupt stack
      void          (*app_in_cb)(void),      //-- callback function used for setup user tasks etc.
      void          (*idle_user_cb)(void)    //-- callback function repeatedly called from idle task
      )
{
   int i;

   //-- Clear/set all globals (vars, lists, etc)

   for (i = 0; i < TN_NUM_PRIORITY; i++){
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

   //-- reset wait queue
   queue_reset(&tn_wait_timeout_list);

   /*
    * NOTE: we need to separate creation of tasks and making them runnable,
    *       because otherwise tn_next_task_to_run would point on the task
    *       that isn't yet created, and it produces issues
    *       with order of task creation.
    *
    *       We should keep as little surprizes in the code as possible,
    *       so, it's better to just separate these steps and avoid any tricks.
    */

   //-- create system tasks
   _timer_task_create(timer_task_stack, timer_task_stack_size);
   _idle_task_create(idle_task_stack, idle_task_stack_size);

   //-- Just for the _tn_task_to_runnable() proper operation
   tn_next_task_to_run = &tn_idle_task; 

   //-- call _tn_task_to_runnable() for system tasks
   _timer_task_to_runnable();
   _idle_task_to_runnable();

   tn_curr_run_task = &tn_idle_task;  //-- otherwise it is NULL

   //-- remember user-provided callbacks
   appl_init_callback          = app_in_cb;
   idle_user_func_callback     = idle_user_cb;

   //-- Run OS - first context switch
   tn_start_exe();
}



/**
 * Process system tick. Should be called periodically, typically
 * from some kind of timer ISR.
 */
void tn_tick_int_processing(void)
{
   TN_INTSAVE_DATA_INT;

   TN_CHECK_INT_CONTEXT_NORETVAL;

   tn_idisable_interrupt();

   //-- manage round-robin (if used)
   _round_robin_manage();

   //-- wake up timer task (NOTE: it actually depends on TN_USE_TIMER_TASK option)
   _timer_task_wakeup();

#if !TN_USE_TIMER_TASK
   //-- manage tn_wait_timeout_list
   _wait_timeout_list_manage();
#endif

   //-- increment system timer
   tn_sys_time_count++;

   tn_ienable_interrupt();  //--  !!! thanks to Audrius Urmanavicius !!!
}

/**
 * Set time slice ticks value for specified priority (round-robin scheduling).
 * 
 * @param priority   priority of tasks for which time slice value should be set
 * @param value      time slice value. Set to NO_TIME_SLICE for no round-robin
 *                   scheduling for given priority (it's default value).
 */
int tn_sys_tslice_ticks(int priority, int value)
{
   int ret = TERR_NO_ERR;

   TN_INTSAVE_DATA;
   TN_CHECK_NON_INT_CONTEXT;

   if (     priority <= 0 || priority >= (TN_NUM_PRIORITY - 1)
         || value    <  0 || value    >   MAX_TIME_SLICE)
   {
      ret = TERR_WRONG_PARAM;
      goto out;
   }

   tn_disable_interrupt();

   tn_tslice_ticks[priority] = value;

   tn_enable_interrupt();

out:
   return ret;
}

/**
 * Get system ticks count.
 */
unsigned int tn_sys_time_get(void)
{
   //-- NOTE: it works if only read access to unsigned int is atomic!
   return tn_sys_time_count;
}

/**
 * Set system ticks count. Note that this value does NOT affect
 * any of the internal TNKernel routines, it is just incremented
 * each system tick (i.e. in tn_tick_int_processing())
 * and is returned by tn_sys_time_get().
 *
 * It is not used by TNKernel itself at all.
 */
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


/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

/**
 * Remove all tasks from wait queue, returning the TERR_DLT code.
 * Note: this function sleeps if there is at least one element in wait_queue.
 */
void _tn_wait_queue_notify_deleted(struct tn_que_head *wait_queue, TN_INTSAVE_DATA_ARG_DEC)
{
   struct tn_que_head *que;
   TN_TCB *task;

   while (!is_queue_empty(wait_queue)){
      //--- delete from wait queue

      que = queue_remove_head(wait_queue);
      task = get_task_by_tsk_queue(que);

      if (_tn_task_wait_complete(task)){
         task->task_wait_rc = TERR_DLT;
         tn_enable_interrupt();
         tn_switch_context();
         tn_disable_interrupt();
      }
   }
}


