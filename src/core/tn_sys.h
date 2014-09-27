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
 */

#ifndef _TN_SYS_H
#define _TN_SYS_H



/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "../arch/tn_arch.h"




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
 * system is just started. 
 *
 * This function **must** do the following:
 *
 *    * Initialize *system timer* here. (*system timer* is some kind of
 *      hardware timer whose ISR calls `tn_tick_int_processing()`)
 *    * Create at least one (and typically just one) user task that will
 *      perform all the rest system initialization.
 *
 * \attention 
 *    * It is illegal to sleep here, because idle task (from which this
 *      function is called) should always be runnable, by design. That's why
 *      this function should perform only the minimum: init system timer
 *      interrupts and create typically just one task, which **is** able to
 *      sleep, and in which all the rest system initialization is typically
 *      done. If `TN_DEBUG` option is set, then sleeping in idle task
 *      is checked, so if you try to sleep here, `_TN_FATAL_ERROR()` macro
 *      will be called.
 *
 *    * This function is called with interrupts disabled, in order to guarantee
 *      that idle task won't be preempted by any other task until callback
 *      function finishes its job. User should not enable interrupts there:
 *      they are enabled by idle task when callback function returns.
 *
 * @see `tn_sys_start()`
 * @see `TN_DEBUG`
 */
typedef void (TNCallbackApplInit)(void);

/**
 * User-provided callback function that is called repeatedly from the idle task 
 * loop. Make sure that idle task has enough stack space to call this function.
 *
 * \attention 
 *    * It is illegal to sleep here, because idle task (from which this
 *      function is called) should always be runnable, by design. If `TN_DEBUG`
 *      option is set, then sleeping in idle task is checked, so if you try to
 *      sleep here, `_TN_FATAL_ERROR()` macro will be called.
 *
 *
 * @see `tn_sys_start()`
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
 * Initial TNeoKernel system start function, never returns. Typically called
 * from main().
 *
 * Refer to the \ref starting_the_kernel
 * "Starting the kernel" section for the usage example and additional comments.
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
void tn_sys_start(
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
 * **Note:** this function should be called before `tn_sys_start()`
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

/**
 * Returns whether current system context is `TN_CONTEXT_TASK`
 *
 * @return `TRUE` if current system context is `TN_CONTEXT_TASK`,
 *         `FALSE` otherwise.
 *
 * @see `tn_sys_context_get()`
 * @see `enum TN_Context`
 */
static inline BOOL tn_is_task_context(void)
{
   return (tn_sys_context_get() == TN_CONTEXT_TASK);
}

/**
 * Returns whether current system context is `TN_CONTEXT_ISR`
 *
 * @return `TRUE` if current system context is `TN_CONTEXT_ISR`,
 *         `FALSE` otherwise.
 *
 * @see `tn_sys_context_get()`
 * @see `enum TN_Context`
 */
static inline BOOL tn_is_isr_context(void)
{
   return (tn_sys_context_get() == TN_CONTEXT_ISR);
}

/**
 * Returns pointer to the currently running task.
 */
struct TN_Task *tn_cur_task_get(void);

/**
 * Returns pointer to the body function of the currently running task.
 */
TN_TaskBody *tn_cur_task_body_get(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _TN_SYS_H

