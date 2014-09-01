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

#ifndef _TH_H_
#define _TH_H_




#include "tn_utils.h"
#include "tn_port.h"

struct _TN_TCB;



//-- Thanks to megajohn from electronix.ru - for IAR Embedded C++ compatibility
#ifdef __cplusplus
extern "C"  {
#endif

 //-- Global vars

extern struct tn_que_head tn_ready_list[TN_NUM_PRIORITY];   //-- all ready to run(RUNNABLE) tasks
extern struct tn_que_head tn_create_queue;                  //-- all created tasks(now - for statictic only)
extern volatile int tn_created_tasks_qty;           //-- num of created tasks
extern struct tn_que_head tn_wait_timeout_list;             //-- all tasks that wait timeout expiration


extern volatile int tn_system_state;    //-- System state -(running/not running,etc.)

extern struct _TN_TCB * tn_curr_run_task;       //-- Task that  run now
extern struct _TN_TCB * tn_next_task_to_run;    //-- Task to be run after switch context

extern volatile unsigned int tn_ready_to_run_bmp;
extern volatile unsigned long tn_idle_count;
extern volatile unsigned long tn_curr_performance;

extern volatile unsigned int  tn_sys_time_count;

extern volatile int tn_int_nest_count;

extern void * tn_user_sp;               //-- Saved task stack pointer
extern void * tn_int_sp;                //-- Saved ISR stack pointer

//-- v.2.7

//----- tn.c ----------------------------------

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
      );
void tn_tick_int_processing(void);
int tn_sys_tslice_ticks(int priority, int value);
unsigned int tn_sys_time_get(void);
void tn_sys_time_set(unsigned int value);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
