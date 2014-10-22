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

/**
 * \file
 *
 * Kernel system routines.
 *   
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_common.h"
#include "tn_sys.h"

//-- internal tnkernel headers
#include "_tn_sys.h"
#include "_tn_timer.h"
#include "_tn_tasks.h"


#include "tn_tasks.h"
#include "tn_timer.h"




/*******************************************************************************
 *    PUBLIC DATA
 ******************************************************************************/

/*
 * For comments on these variables, please see _tn_sys.h file.
 */
struct TN_ListItem tn_ready_list[TN_PRIORITIES_CNT];
struct TN_ListItem tn_create_queue;
volatile int tn_created_tasks_cnt;

unsigned short tn_tslice_ticks[TN_PRIORITIES_CNT];

volatile enum TN_StateFlag tn_sys_state;

struct TN_Task *tn_next_task_to_run;
struct TN_Task *tn_curr_run_task;

volatile unsigned int tn_ready_to_run_bmp;

volatile unsigned int tn_sys_time_count;

volatile int tn_int_nest_count;

#if TN_MUTEX_DEADLOCK_DETECT
volatile int tn_deadlocks_cnt = 0;
#endif

void *tn_user_sp;
void *tn_int_sp;

//-- System tasks

//-- idle task - priority (TN_PRIORITIES_CNT - 1) - lowest

struct TN_Task  tn_idle_task;
static void _idle_task_body(void * par);

/**
 * Pointer to user idle loop function
 */
TN_CBIdle        *tn_callback_idle_hook = NULL;

/**
 * User-provided callback function that is called whenever 
 * event occurs (say, deadlock becomes active or inactive)
 */
TN_CBDeadlock    *tn_callback_deadlock = NULL;



/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Idle task body. In fact, this task is always in RUNNABLE state.
 */
static void _idle_task_body(void *par)
{
   //-- enter endless loop with calling user-provided hook function
   for(;;)
   {
      tn_callback_idle_hook();
   }
}

/**
 * Manage round-robin (if used)
 */
static inline void _round_robin_manage(void)
{
   //-- volatile is used here only to solve
   //   IAR(c) compiler's high optimization mode problem
   volatile struct TN_ListItem *curr_que;
   volatile struct TN_ListItem *pri_queue;
   volatile int priority = tn_curr_run_task->priority;

   if (tn_tslice_ticks[priority] != TN_NO_TIME_SLICE){
      tn_curr_run_task->tslice_count++;

      if (tn_curr_run_task->tslice_count > tn_tslice_ticks[priority]){
         tn_curr_run_task->tslice_count = 0;

         pri_queue = &(tn_ready_list[priority]);
         //-- If ready queue is not empty and there are more than 1 
         //   task in the queue
         if (     !(tn_is_list_empty((struct TN_ListItem *)pri_queue))
               && pri_queue->next->next != pri_queue
            )
         {
            //-- Remove task from head and add it to the tail of
            //-- ready queue for current priority

            curr_que = tn_list_remove_head(&(tn_ready_list[priority]));
            tn_list_add_tail(
                  &(tn_ready_list[priority]),
                  (struct TN_ListItem *)curr_que
                  );
         }
      }
   }
}

/**
 * Create idle task, the task is NOT started after creation.
 */
static inline enum TN_RCode _idle_task_create(
      TN_UWord      *idle_task_stack,
      unsigned int   idle_task_stack_size
      )
{
   return tn_task_create(
         (struct TN_Task*)&tn_idle_task,  //-- task TCB
         _idle_task_body,                 //-- task function
         TN_PRIORITIES_CNT - 1,           //-- task priority
         idle_task_stack,                 //-- task stack
         idle_task_stack_size,            //-- task stack size
                                          //   (in int, not bytes)
         NULL,                            //-- task function parameter
         (TN_TASK_CREATE_OPT_IDLE)        //-- Creation option
         );
}




/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (tn_sys.h)
 */
void tn_sys_start(
      TN_UWord            *idle_task_stack,
      unsigned int         idle_task_stack_size,
      TN_UWord            *int_stack,
      unsigned int         int_stack_size,
      TN_CBUserTaskCreate *cb_user_task_create,
      TN_CBIdle           *cb_idle
      )
{
   int i;
   enum TN_RCode rc;

   //-- for each priority: 
   //   - reset list of runnable tasks with this priority
   //   - reset time slice to `#TN_NO_TIME_SLICE`
   for (i = 0; i < TN_PRIORITIES_CNT; i++){
      tn_list_reset(&(tn_ready_list[i]));
      tn_tslice_ticks[i] = TN_NO_TIME_SLICE;
   }

   //-- reset generic task queue and task count to 0
   tn_list_reset(&tn_create_queue);
   tn_created_tasks_cnt = 0;

   //-- initial system flags: no flags set (see enum TN_StateFlag)
   tn_sys_state = (0);  

   //-- reset bitmask of priorities with runnable tasks
   tn_ready_to_run_bmp = 0;

   //-- reset system time
   tn_sys_time_count = 0;

   //-- reset interrupt nesting count
   tn_int_nest_count = 0;

   //-- reset pointers to currently running task and next task to run
   tn_next_task_to_run = NULL;
   tn_curr_run_task    = NULL;

   //-- remember user-provided callbacks
   tn_callback_idle_hook = cb_idle;

   //-- Fill interrupt stack space with TN_FILL_STACK_VAL
   for (i = 0; i < int_stack_size; i++){
      int_stack[i] = TN_FILL_STACK_VAL;
   }

   //-- set interrupt's top of the stack
   tn_int_sp = _tn_arch_stack_top_get(
         int_stack,
         int_stack_size
         );

   //-- init timers
   _tn_timers_init();

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
   rc = _idle_task_create(idle_task_stack, idle_task_stack_size);
   if (rc != TN_RC_OK){
      _TN_FATAL_ERROR("failed to create idle task");
   }

   //-- Just for the _tn_task_set_runnable() proper operation
   tn_next_task_to_run = &tn_idle_task; 

   //-- make system tasks runnable
   rc = _tn_task_activate(&tn_idle_task);
   if (rc != TN_RC_OK){
      _TN_FATAL_ERROR("failed to activate idle task");
   }

   //-- set tn_curr_run_task to idle task
   tn_curr_run_task = &tn_idle_task;

   //-- now, we can create user's task(s)
   //   (by user-provided callback)
   cb_user_task_create();

   //-- set flag that system is running
   //   (well, it will be running soon actually)
   tn_sys_state |= TN_STATE_FLAG__SYS_RUNNING;

   //-- Run OS - first context switch
   _tn_arch_context_switch_now_nosave();
}



/*
 * See comments in the header file (tn_sys.h)
 */
enum TN_RCode tn_tick_int_processing(void)
{
   enum TN_RCode rc = TN_RC_OK;

   if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA_INT;

      TN_INT_IDIS_SAVE();

      //-- increment system timer
      tn_sys_time_count++;

      //-- manage round-robin (if used)
      _round_robin_manage();

      //-- manage timers
      _tn_timers_tick_proceed();

      TN_INT_IRESTORE();
      _TN_CONTEXT_SWITCH_IPEND_IF_NEEDED();

   }
   return rc;
}

/*
 * See comments in the header file (tn_sys.h)
 */
enum TN_RCode tn_sys_tslice_set(int priority, int ticks)
{
   enum TN_RCode rc = TN_RC_OK;

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else if (0
         || priority < 0 || priority >= (TN_PRIORITIES_CNT - 1)
         || ticks    < 0 || ticks    >   TN_MAX_TIME_SLICE)
   {
      rc = TN_RC_WPARAM;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();
      tn_tslice_ticks[priority] = ticks;
      TN_INT_RESTORE();
   }
   return rc;
}

/*
 * See comments in the header file (tn_sys.h)
 */
unsigned int tn_sys_time_get(void)
{
   //-- NOTE: it works if only read access to unsigned int is atomic!
   return tn_sys_time_count;
}

/*
 * Returns current state flags (tn_sys_state)
 */
enum TN_StateFlag tn_sys_state_flags_get(void)
{
   return tn_sys_state;
}

/*
 * See comment in tn_sys.h file
 */
void tn_callback_deadlock_set(TN_CBDeadlock *cb)
{
   tn_callback_deadlock = cb;
}

/*
 * See comment in tn_sys.h file
 */
enum TN_Context tn_sys_context_get(void)
{
   enum TN_Context ret;

   if (tn_sys_state & TN_STATE_FLAG__SYS_RUNNING){
      ret = _tn_arch_inside_isr()
         ? TN_CONTEXT_ISR
         : TN_CONTEXT_TASK;
   } else {
      ret = TN_CONTEXT_NONE;
   }

   return ret;
}

/*
 * See comment in tn_sys.h file
 */
struct TN_Task *tn_cur_task_get(void)
{
   return tn_curr_run_task;
}

/*
 * See comment in tn_sys.h file
 */
TN_TaskBody *tn_cur_task_body_get(void)
{
   return tn_curr_run_task->task_func_addr;
}




/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

/**
 * See comment in the _tn_sys.h file
 */
void _tn_wait_queue_notify_deleted(struct TN_ListItem *wait_queue)
{
   struct TN_Task *task;         //-- "cursor" for the loop iteration
   struct TN_Task *tmp_task;     //-- we need for temporary item because
                                 //   item is removed from the list
                                 //   in _tn_mutex_do_unlock().


   //-- iterate through all tasks in the wait_queue,
   //   calling _tn_task_wait_complete() for each task,
   //   and setting TN_RC_DELETED as a wait return code.
   tn_list_for_each_entry_safe(task, tmp_task, wait_queue, task_queue){
      //-- call _tn_task_wait_complete for every task
      _tn_task_wait_complete(task, TN_RC_DELETED);
   }

#if TN_DEBUG
   if (!tn_is_list_empty(wait_queue)){
      _TN_FATAL_ERROR("");
   }
#endif
}

/**
 * See comments in the file _tn_sys.h
 */
enum TN_StateFlag _tn_sys_state_flags_set(enum TN_StateFlag flags)
{
   enum TN_StateFlag ret = tn_sys_state;
   tn_sys_state |= flags;
   return ret;
}

/**
 * See comments in the file _tn_sys.h
 */
enum TN_StateFlag _tn_sys_state_flags_clear(enum TN_StateFlag flags)
{
   enum TN_StateFlag ret = tn_sys_state;
   tn_sys_state &= ~flags;
   return ret;
}


#if TN_MUTEX_DEADLOCK_DETECT
/**
 * See comments in the file _tn_sys.h
 */
void _tn_cry_deadlock(BOOL active, struct TN_Mutex *mutex, struct TN_Task *task)
{
   if (active){
      if (tn_deadlocks_cnt == 0){
         _tn_sys_state_flags_set(TN_STATE_FLAG__DEADLOCK);
      }

      tn_deadlocks_cnt++;
   } else {
      tn_deadlocks_cnt--;

      if (tn_deadlocks_cnt == 0){
         _tn_sys_state_flags_clear(TN_STATE_FLAG__DEADLOCK);
      }
   }

   if (tn_callback_deadlock != NULL){
      tn_callback_deadlock(active, mutex, task);
   }

}
#endif

/**
 * See comments in the file _tn_sys.h
 */
void _tn_memcpy_uword(
      TN_UWord *tgt, const TN_UWord *src, unsigned int size_uwords
      )
{
   int i;
   for (i = 0; i < size_uwords; i++){
      *tgt++ = *src++;
   }
}


