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
 * Kernel system routines: system start, tick processing, time slice managing.
 *
 * ## Time ticks
 *
 * For the purpose of calculating timeouts, the kernel uses a time tick timer.
 * In TNeoKernel, this time tick timer must to be a some kind of hardware timer
 * that produces interrupt for time ticks processing. The period of this timer
 * is determined by user (typically 1 ms, but user is free to set different
 * value).  Within the time ticks interrupt processing, it is only necessary to
 * call the `tn_tick_int_processing()` function:
 *
 *     //-- example for PIC32, hardware timer 5 interrupt:
 *     tn_soft_isr(_TIMER_5_VECTOR)
 *     {
 *        INTClearFlag(INT_T5);
 *        tn_tick_int_processing();
 *     }
 *
 * Notice the `tn_soft_isr()` ISR wrapper macro we've used here.
 *
 * ## Round-robin scheduling
 *
 * TNKernel has the ability to make round robin scheduling for tasks with
 * identical priority.  By default, round robin scheduling is turned off for
 * all priorities. To enable round robin scheduling for tasks on certain
 * priority level and to set time slices for these priority, user must call the
 * `tn_sys_tslice_ticks()` function.  The time slice value is the same for all
 * tasks with identical priority but may be different for each priority level.
 * If the round robin scheduling is enabled, every system time tick interrupt
 * increments the currently running task time slice counter. When the time
 * slice interval is completed, the task is placed at the tail of the ready to
 * run queue of its priority level (this queue contains tasks in the
 * `TN_TASK_STATE_RUNNABLE` state) and the time slice counter is cleared. Then
 * the task may be preempted by tasks of higher or equal priority.
 *
 * In most cases, there is no reason to enable round robin scheduling. For
 * applications running multiple copies of the same code, however, (GUI
 * windows, etc), round robin scheduling is an acceptable solution.
 *
 * ## Starting the kernel
 *
 * ### Quick guide on startup process
 *
 * * You allocate arrays for idle task stack and interrupt stack. Typically,
 *   these are just static arrays of `int`.
 * * You provide callback function like `void appl_init(void) { ... }`, in which
 *   all the peripheral and interrupts should be initialized. Note that
 *   this function runs with interrupts disabled, in order to make sure nobody 
 *   will preempt idle task until `appl_init` gets its job done.
 *   You shouldn't enable them.
 * * You provide idle callback function to be called periodically from 
 *   idle task. It's quite fine to leave it empty.
 * * In the `main()`, you firstly perform some essential CPU settings, such as
 *   oscillator settings and similar things. Don't set up any peripheral and
 *   interrupts here, these things should be done in your callback
 *   `appl_init()`.
 * * You call `tn_start_system()` providing all necessary information:
 *   pointers to stacks, their sizes, and your callback functions.
 * * Kernel acts as follows:
 *   * performs all necessary housekeeping;
 *   * creates idle task;
 *   * performs first context switch (to idle task).
 * * Idle task acts as follows:
 *   * disables interrupts to make sure nobody will preempt it until all the
 *     job is done;
 *   * calls your `appl_init()` function;
 *   * enables interrupts
 *
 * At this point, system is started and operates normally.
 *
 * 
 *
 * ### Basic example for PIC32
 *
 *     #define SYS_FREQ 80000000UL
 *
 *     //-- idle task stack size, in words
 *     #define IDLE_TASK_STACK_SIZE          64
 *
 *     //-- interrupt stack size, in words
 *     #define INTERRUPT_STACK_SIZE          128
 *
 *     //-- allocate arrays for idle task and interrupt statically
 *     unsigned int idle_task_stack[ IDLE_TASK_STACK_SIZE ];
 *     unsigned int interrupt_stack[ INTERRUPT_STACK_SIZE ];
 *
 *     static void _appl_init(void)
 *     {
 *        //-- initialize system timer, initialize and enable its interrupt
 *        //   NOTE: ISR won't be called until _appl_init() returns because
 *        //   interrupts are now disabled globally by idle task.
 *        //   TODO
 *
 *        //-- initialize user task(s) and probably other kernel objects
 *        my_tasks_create();
 *     }
 *
 *     static void _idle_task_callback(void)
 *     {
 *        //-- do nothing here
 *     }
 *
 *     int main(void)
 *     {
 *        
 *        //-- some essential CPU initialization:
 *        //   say, for PIC32, it might be something like:
 *        SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE)
 *
 *        //-- NOTE: at this point, no interrupts were set up,
 *        //   this is important. All interrupts should be set up in 
 *        //   _appl_init() that is called from idle task, or later from 
 *        //   other user tasks.
 *
 *        //-- call to tn_start_system() never returns
 *        tn_start_system(
 *           idle_task_stack,
 *           IDLE_TASK_STACK_SIZE,
 *           interrupt_stack,
 *           INTERRUPT_STACK_SIZE,
 *           _appl_init,
 *           _idle_task_callback
 *           );
 *
 *        //-- unreachable
 *        return 0;
 *     }
 *
 *   
 */

#ifndef _TN_SYS_H
#define _TN_SYS_H



/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_arch.h"




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

/**
 * System state flags
 */
enum TN_StateFlag {
   ///
   /// system is running
   TN_STATE_FLAG__SYS_RUNNING    = (1 << 0),
   ///
   /// deadlock is active
   /// Note: this feature works if only `TN_MUTEX_DEADLOCK_DETECT` is non-zero.
   /// @see `TN_MUTEX_DEADLOCK_DETECT`
   TN_STATE_FLAG__DEADLOCK       = (1 << 1),
};

/**
 * System context
 *
 * @see `tn_sys_context_get()`
 */
enum TN_Context {
   ///
   /// None: this code is possible if only system is not running
   /// (flag (`TN_STATE_FLAG__SYS_RUNNING` is not set in the `tn_sys_state`))
   TN_CONTEXT_NONE,
   ///
   /// Task context
   TN_CONTEXT_TASK,
   ///
   /// ISR context
   TN_CONTEXT_ISR,
};

/**
 * User-provided callback function that is called once from idle task when
 * system is just started.  User must initialize *system timer* here. (*system
 * timer* is some kind of hardware timer whose ISR calls
 * `tn_tick_int_processing()`) User may want to initialize other tasks and
 * kernel objects in this function. Make sure that idle task has enough stack
 * space to call this function (array for idle task stack is given to
 * tn_start_system()).
 *
 * **Note:** this function is called with interrupts disabled, in order to
 * guarantee that idle task won't be preempted by any other task until callback
 * function finishes its job. User should not enable interrupts there: they are
 * enabled by idle task when callback function returns.
 *
 * @see `tn_start_system()`
 */
typedef void (TNCallbackApplInit)(void);

/**
 * User-provided callback function that is called repeatedly from the idle task 
 * loop. Make sure that idle task has enough stack space to call this function.
 *
 * **This function should not sleep**, because idle task should always 
 * be runnable, by design. Sleeping in this function leads to instable system.
 *
 *
 * @see `tn_start_system()`
 */
typedef void (TNCallbackIdle)(void);

/**
 * User-provided callback function that is called whenever 
 * deadlock becomes active or inactive.
 * Note: this feature works if only `TN_MUTEX_DEADLOCK_DETECT` is non-zero.
 *
 * @param active  if `TRUE`, deadlock becomes active, otherwise it becomes
 *                inactive (say, if task stopped waiting for mutex 
 *                because of timeout)
 * @param mutex   mutex that is involved in deadlock. You may find out other
 *                mutexes involved by means of `mutex->deadlock_list`.
 * @param task    task that is involved in deadlock. You may find out other 
 *                tasks involved by means of `task->deadlock_list`.
 *
 * @see `TN_MUTEX_DEADLOCK_DETECT`
 */
typedef void (TNCallbackDeadlock)(
      BOOL active,
      struct TN_Mutex *mutex,
      struct TN_Task *task
      );




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Value to pass to `tn_sys_tslice_ticks()` to turn round-robin off.
 */
#define  TN_NO_TIME_SLICE              0

/**
 * Max value of time slice
 */
#define  TN_MAX_TIME_SLICE             0xFFFE




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Initial TNeoKernel system start function, never returns.
 * Typically called from main().
 * See the section '**Starting the kernel**' above for the usage example and
 * additional comments.
 *
 * @param   idle_task_stack      pointer to array for idle task stack. User must
 *                               allocate it as an array of `unsigned int`.
 * @param   idle_task_stack_size size of idle task stack, in `int`s.
 * @param   int_stack            pointer to array for interrupt stack. User must
 *                               allocate it as an array of `unsigned int`.
 * @param   int_stack_size       size of interrupt stack, in `int`s.
 * @param   cb_appl_init         callback function used for setup user tasks,
 *                               etc. Called from idle task, so, make sure
 *                               idle task has enough stack space for it.
 * @param   cb_idle              callback function repeatedly called 
 *                               from idle task.
 */
void tn_start_system(
      unsigned int        *idle_task_stack,
      unsigned int         idle_task_stack_size,
      unsigned int        *int_stack,
      unsigned int         int_stack_size,
      TNCallbackApplInit  *cb_appl_init,
      TNCallbackIdle      *cb_idle
      );

/**
 * Process system tick; should be called periodically, typically
 * from some kind of timer ISR.
 *
 * The period of this timer is determined by user 
 * (typically 1 ms, but user is free to set different value)
 */
void tn_tick_int_processing(void);

/**
 * Set time slice ticks value for specified priority (round-robin scheduling).
 * 
 * @param priority   priority of tasks for which time slice value should be set
 * @param value      time slice value. Set to `TN_NO_TIME_SLICE` for no
 *                   round-robin scheduling for given priority
 *                   (it's default value).
 *                   Value can't be higher than `TN_MAX_TIME_SLICE`.
 */
enum TN_RCode tn_sys_tslice_ticks(int priority, int value);

/**
 * Get current system ticks count.
 */
unsigned int tn_sys_time_get(void);

/**
 * Set system ticks count. Note that this value does **not** affect any of the
 * internal TNeoKernel routines, it is just incremented each system tick (i.e.
 * in `tn_tick_int_processing()`) and is returned to user by
 * `tn_sys_time_get()`.
 *
 * It is not used by TNeoKernel itself at all.
 */
void tn_sys_time_set(unsigned int value);


/**
 * Set callback function that should be called whenever deadlock occurs or
 * becomes inactive (say, if one of tasks involved in the deadlock was released
 * from wait because of timeout)
 *
 *
 * **Note:** this function should be called before `tn_start_system()`
 *
 * @see `TN_MUTEX_DEADLOCK_DETECT`
 * @see `TNCallbackDeadlock` for callback function prototype
 */
void tn_callback_deadlock_set(TNCallbackDeadlock *cb);

/**
 * Returns current system state flags
 */
enum TN_StateFlag tn_sys_state_flags_get(void);

/**
 * Returns system context: task or ISR.
 *
 * @see `enum TN_Context`
 */
enum TN_Context tn_sys_context_get(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _TN_SYS_H

