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
 *    DEFINITIONS
 ******************************************************************************/


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
   /// Note: this feature works if only `#TN_MUTEX_DEADLOCK_DETECT` is non-zero.
   /// @see `#TN_MUTEX_DEADLOCK_DETECT`
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
   /// (flag (`#TN_STATE_FLAG__SYS_RUNNING` is not set in the `tn_sys_state`))
   TN_CONTEXT_NONE,
   ///
   /// Task context
   TN_CONTEXT_TASK,
   ///
   /// ISR context
   TN_CONTEXT_ISR,
};

/**
 * User-provided callback function that is called directly from
 * `tn_sys_start()` as a part of system startup routine; it should merely
 * create at least one (and typically just one) user's task, which should
 * perform all the rest application initialization.
 *
 * When `TN_CBUserTaskCreate()` returned, the kernel performs first context
 * switch to the task with highest priority. If there are several tasks with
 * highest priority, context is switched to the first created one.
 *
 * Refer to the section \ref starting_the_kernel for details about system
 * startup process on the whole.
 *
 * **Note:** Although you're able to create more than one task here, it's
 * usually not so good idea, because many things typically should be done at
 * startup before tasks can go on with their job: we need to initialize various
 * on-board peripherals (displays, flash memory chips, or whatever) as well as
 * initialize software modules used by application. So, if many tasks are
 * created here, you have to provide some synchronization object so that tasks
 * will wait until all the initialization is done.
 *
 * It's usually easier to maintain if we create just one task here, which
 * firstly performs all the necessary initialization, **then** creates the rest
 * of your tasks, and eventually gets to its primary job (the job for which
 * task was created at all). For the usage example, refer to the page \ref
 * starting_the_kernel.
 *
 * \attention 
 *    * The only system service is allowed to call in this function is
 *      `tn_task_create()`.
 *
 * @see `tn_sys_start()`
 */
typedef void (TN_CBUserTaskCreate)(void);

/**
 * User-provided callback function that is called repeatedly from the idle task 
 * loop. Make sure that idle task has enough stack space to call this function.
 *
 * \attention 
 *    * It is illegal to sleep here, because idle task (from which this
 *      function is called) should always be runnable, by design. If `#TN_DEBUG`
 *      option is set, then sleeping in idle task is checked, so if you try to
 *      sleep here, `_TN_FATAL_ERROR()` macro will be called.
 *
 *
 * @see `tn_sys_start()`
 */
typedef void (TN_CBIdle)(void);

/**
 * User-provided callback function that is called whenever 
 * deadlock becomes active or inactive.
 * Note: this feature works if only `#TN_MUTEX_DEADLOCK_DETECT` is non-zero.
 *
 * @param active  if `TRUE`, deadlock becomes active, otherwise it becomes
 *                inactive (say, if task stopped waiting for mutex 
 *                because of timeout)
 * @param mutex   mutex that is involved in deadlock. You may find out other
 *                mutexes involved by means of `mutex->deadlock_list`.
 * @param task    task that is involved in deadlock. You may find out other 
 *                tasks involved by means of `task->deadlock_list`.
 */
typedef void (TN_CBDeadlock)(
      BOOL active,
      struct TN_Mutex *mutex,
      struct TN_Task *task
      );




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Value to pass to `tn_sys_tslice_set()` to turn round-robin off.
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
 * Refer to the \ref starting_the_kernel "Starting the kernel" section for the
 * usage example and additional comments.
 *
 * $(TN_CALL_FROM_MAIN)
 * $(TN_LEGEND_LINK)
 *
 * @param   idle_task_stack      
 *    Pointer to array for idle task stack. 
 *    User must either use the macro `TN_TASK_STACK_DEF()` for the definition
 *    of stack array, or allocate it manually as an array of `#TN_UWord` with
 *    `#TN_ARCH_STK_ATTR_BEFORE` and `#TN_ARCH_STK_ATTR_AFTER` macros.
 * @param   idle_task_stack_size 
 *    Size of idle task stack, in words (`#TN_UWord`)
 * @param   int_stack            
 *    Pointer to array for interrupt stack.
 *    User must either use the macro `TN_TASK_STACK_DEF()` for the definition
 *    of stack array, or allocate it manually as an array of `#TN_UWord` with
 *    `#TN_ARCH_STK_ATTR_BEFORE` and `#TN_ARCH_STK_ATTR_AFTER` macros.
 * @param   int_stack_size       
 *    Size of interrupt stack, in words (`#TN_UWord`)
 * @param   cb_user_task_create
 *    Callback function that should create initial user's task, see
 *    `#TN_CBUserTaskCreate` for details.
 * @param   cb_idle              
 *    Callback function repeatedly called from idle task, see `#TN_CBIdle` for
 *    details.
 */
void tn_sys_start(
      TN_UWord            *idle_task_stack,
      unsigned int         idle_task_stack_size,
      TN_UWord            *int_stack,
      unsigned int         int_stack_size,
      TN_CBUserTaskCreate *cb_user_task_create,
      TN_CBIdle           *cb_idle
      );

/**
 * Process system tick; should be called periodically, typically
 * from some kind of timer ISR.
 *
 * The period of this timer is determined by user 
 * (typically 1 ms, but user is free to set different value)
 *
 * For further information, refer to \ref quick_guide "Quick guide".
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @return
 *    * `#TN_RC_OK` on success;
 *    * `#TN_RC_WCONTEXT` if called from wrong context.
 */
enum TN_RCode tn_tick_int_processing(void);

/**
 * Set time slice ticks value for specified priority (see \ref round_robin).
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_LEGEND_LINK)
 * 
 * @param priority
 *    Priority of tasks for which time slice value should be set
 * @param ticks
 *    Time slice value, in ticks. Set to `#TN_NO_TIME_SLICE` for no round-robin
 *    scheduling for given priority (it's default value). Value can't be
 *    higher than `#TN_MAX_TIME_SLICE`.
 *
 * @return
 *    * `#TN_RC_OK` on success;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * `#TN_RC_WPARAM` if given `priority` or `ticks` are invalid.
 */
enum TN_RCode tn_sys_tslice_set(int priority, int ticks);

/**
 * Get current system ticks count.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @return
 *    Current system ticks count. Note that this value does **not** affect any
 *    of the internal TNeoKernel routines, it is just incremented each system
 *    tick (i.e. in `tn_tick_int_processing()`) and is returned to user by
 *    `tn_sys_time_get()`.
 *
 *    It is not used by TNeoKernel itself at all.
 */
unsigned int tn_sys_time_get(void);

/**
 * Set system ticks count. Note that this value does **not** affect any of the
 * internal TNeoKernel routines, it is just incremented each system tick (i.e.
 * in `tn_tick_int_processing()`) and is returned to user by
 * `tn_sys_time_get()`.
 *
 * It is not used by TNeoKernel itself at all.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 */
void tn_sys_time_set(unsigned int value);


/**
 * Set callback function that should be called whenever deadlock occurs or
 * becomes inactive (say, if one of tasks involved in the deadlock was released
 * from wait because of timeout)
 *
 * $(TN_CALL_FROM_MAIN)
 * $(TN_LEGEND_LINK)
 *
 * **Note:** this function should be called before `tn_sys_start()`
 *
 * @param cb
 *    Pointer to user-provided callback function.
 *
 * @see `#TN_MUTEX_DEADLOCK_DETECT`
 * @see `#TN_CBDeadlock` for callback function prototype
 */
void tn_callback_deadlock_set(TN_CBDeadlock *cb);

/**
 * Returns current system state flags
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 */
enum TN_StateFlag tn_sys_state_flags_get(void);

/**
 * Returns system context: task or ISR.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_CALL_FROM_MAIN)
 * $(TN_LEGEND_LINK)
 *
 * @see `enum #TN_Context`
 */
enum TN_Context tn_sys_context_get(void);

/**
 * Returns whether current system context is `#TN_CONTEXT_TASK`
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_CALL_FROM_MAIN)
 * $(TN_LEGEND_LINK)
 *
 * @return `TRUE` if current system context is `#TN_CONTEXT_TASK`,
 *         `FALSE` otherwise.
 *
 * @see `tn_sys_context_get()`
 * @see `enum #TN_Context`
 */
static inline BOOL tn_is_task_context(void)
{
   return (tn_sys_context_get() == TN_CONTEXT_TASK);
}

/**
 * Returns whether current system context is `#TN_CONTEXT_ISR`
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_CALL_FROM_MAIN)
 * $(TN_LEGEND_LINK)
 *
 * @return `TRUE` if current system context is `#TN_CONTEXT_ISR`,
 *         `FALSE` otherwise.
 *
 * @see `tn_sys_context_get()`
 * @see `enum #TN_Context`
 */
static inline BOOL tn_is_isr_context(void)
{
   return (tn_sys_context_get() == TN_CONTEXT_ISR);
}

/**
 * Returns pointer to the currently running task.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 */
struct TN_Task *tn_cur_task_get(void);

/**
 * Returns pointer to the body function of the currently running task.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 */
TN_TaskBody *tn_cur_task_body_get(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _TN_SYS_H

