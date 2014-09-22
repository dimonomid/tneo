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

#ifndef _TN_SYS_H
#define _TN_SYS_H



/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_port.h"

//-- Thanks to megajohn from electronix.ru - for IAR Embedded C++ compatibility
#ifdef __cplusplus
extern "C"  {  /*}*/
#endif


/*******************************************************************************
 *    EXTERNAL TYPES
 ******************************************************************************/

struct TN_Task;
struct TN_Mutex;



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

enum TN_StateFlag {
   TN_STATE_FLAG__SYS_RUNNING    = (1 << 0), //-- system is running
   TN_STATE_FLAG__DEADLOCK       = (1 << 1), //-- deadlock is active
};

typedef void (*TNApplInitCallback)(void);
typedef void (*TNIdleCallback)(void);

/**
 * User-provided callback function that is called whenever 
 * deadlock becomes active or inactive.
 *
 * @param active  if TRUE, deadlock becomes active, otherwise it becomes inactive
 *                (say, if task stopped waiting for mutex because of timeout)
 * @param mutex   mutex that is involved in deadlock. You may find out other mutexes
 *                involved by means of mutex->deadlock_list.
 * @param task    task that is involved in deadlock. You may find out other tasks
 *                involved by means of task->deadlock_list.
 */
typedef void (*TNCallbackDeadlock)(BOOL active, struct TN_Mutex *mutex, struct TN_Task *task);





/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

extern struct TN_ListItem tn_ready_list[TN_NUM_PRIORITY];   //-- all ready to run(RUNNABLE) tasks
extern struct TN_ListItem tn_create_queue;                  //-- all created tasks(now - for statictic only)
extern volatile int tn_created_tasks_qty;           //-- num of created tasks
extern struct TN_ListItem tn_wait_timeout_list;             //-- all tasks that wait timeout expiration


extern volatile enum TN_StateFlag tn_sys_state;

extern struct TN_Task * tn_curr_run_task;       //-- Task that  run now
extern struct TN_Task * tn_next_task_to_run;    //-- Task to be run after switch context

extern volatile unsigned int tn_ready_to_run_bmp;
extern volatile unsigned long tn_idle_count;
extern volatile unsigned long tn_curr_performance;

extern volatile unsigned int  tn_sys_time_count;

extern volatile int tn_int_nest_count;

extern void * tn_user_sp;               //-- Saved task stack pointer
extern void * tn_int_sp;                //-- Saved ISR stack pointer



/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

void tn_start_system(
      unsigned int  *idle_task_stack,        //-- pointer to array for idle task stack
      unsigned int   idle_task_stack_size,   //-- size of idle task stack
      unsigned int  *int_stack,              //-- pointer to array for interrupt stack
      unsigned int   int_stack_size,         //-- size of interrupt stack
      void          (*app_in_cb)(void),      //-- callback function used for setup user tasks etc.
      void          (*idle_user_cb)(void)    //-- callback function repeatedly called from idle task
      );
void tn_tick_int_processing(void);
enum TN_Retval tn_sys_tslice_ticks(int priority, int value);
unsigned int tn_sys_time_get(void);
void tn_sys_time_set(unsigned int value);


/**
 * Set callback function that should be called whenever
 * deadlock occurs or becomes inactive
 * (say, if one of tasks stopped waiting because of timeout)
 *
 * Should be called before tn_start_system()
 *
 * @see TNCallbackDeadlock for callback function prototype
 */
void tn_callback_deadlock_set(TNCallbackDeadlock cb);

/**
 * Returns current state flags
 */
enum TN_StateFlag tn_sys_state_flags_get(void);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _TN_SYS_H
