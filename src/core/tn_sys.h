/*******************************************************************************
 *
 * TNeo: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright 2013, 2014 Anders Montonen.
 *    TNeo:                      copyright 2014       Dmitry Frank.
 *
 *    TNeo was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeo.
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
#include "tn_cfg_dispatch.h"

#include "tn_timer.h"



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

/**
 * Convenience macro for the definition of stack array. See 
 * `tn_task_create()` for the usage example.
 *
 * @param name 
 *    C variable name of the array
 * @param size
 *    size of the stack array in words (`#TN_UWord`), not in bytes.
 */
#define  TN_STACK_ARR_DEF(name, size)        \
   TN_ARCH_STK_ATTR_BEFORE                   \
   TN_UWord name[ (size) ]                   \
   TN_ARCH_STK_ATTR_AFTER


#if TN_CHECK_BUILD_CFG

/**
 * For internal kernel usage: helper macro that fills architecture-dependent
 * values. This macro is used by `#_TN_BUILD_CFG_STRUCT_FILL()` only.
 */
#if defined (__TN_ARCH_PIC24_DSPIC__)

#  define _TN_BUILD_CFG_ARCH_STRUCT_FILL(_p_struct)               \
{                                                                 \
   (_p_struct)->arch.p24.p24_sys_ipl = TN_P24_SYS_IPL;            \
}

#else
#  define _TN_BUILD_CFG_ARCH_STRUCT_FILL(_p_struct)
#endif




/**
 * For internal kernel usage: fill the structure `#_TN_BuildCfg` with
 * current build-time configuration values.
 *
 * @param _p_struct     Pointer to struct `#_TN_BuildCfg`
 */
#define  _TN_BUILD_CFG_STRUCT_FILL(_p_struct)                           \
{                                                                       \
   memset((_p_struct), 0x00, sizeof(*(_p_struct)));                     \
                                                                        \
   (_p_struct)->priorities_cnt            = TN_PRIORITIES_CNT;          \
   (_p_struct)->check_param               = TN_CHECK_PARAM;             \
   (_p_struct)->debug                     = TN_DEBUG;                   \
   (_p_struct)->use_mutexes               = TN_USE_MUTEXES;             \
   (_p_struct)->mutex_rec                 = TN_MUTEX_REC;               \
   (_p_struct)->mutex_deadlock_detect     = TN_MUTEX_DEADLOCK_DETECT;   \
   (_p_struct)->tick_lists_cnt_minus_one  = (TN_TICK_LISTS_CNT - 1);    \
   (_p_struct)->api_make_alig_arg         = TN_API_MAKE_ALIG_ARG;       \
   (_p_struct)->profiler                  = TN_PROFILER;                \
   (_p_struct)->profiler_wait_time        = TN_PROFILER_WAIT_TIME;      \
   (_p_struct)->stack_overflow_check      = TN_STACK_OVERFLOW_CHECK;    \
   (_p_struct)->dynamic_tick              = TN_DYNAMIC_TICK;            \
   (_p_struct)->old_events_api            = TN_OLD_EVENT_API;           \
                                                                        \
   _TN_BUILD_CFG_ARCH_STRUCT_FILL(_p_struct);                           \
}


#else

#define  _TN_BUILD_CFG_STRUCT_FILL(_p_struct)   /* nothing */

#endif   // TN_CHECK_BUILD_CFG


/**
 * For internal kernel usage: helper macro that allows functions to be inlined
 * or not depending on configuration (see `#TN_MAX_INLINE`)
 */
#if TN_MAX_INLINE
#  define _TN_MAX_INLINED_FUNC _TN_STATIC_INLINE
#else
#  define _TN_MAX_INLINED_FUNC /* nothing */
#endif



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Structure with build-time configurations values; it is needed for run-time
 * check which ensures that build-time options for the kernel match ones for
 * the application. See `#TN_CHECK_BUILD_CFG` for details.
 */
struct _TN_BuildCfg {
   ///
   /// Value of `#TN_PRIORITIES_CNT`
   unsigned          priorities_cnt             : 7;
   ///
   /// Value of `#TN_CHECK_PARAM`
   unsigned          check_param                : 1;
   ///
   /// Value of `#TN_DEBUG`
   unsigned          debug                      : 1;
   //-- Note: we don't include TN_OLD_TNKERNEL_NAMES since it doesn't
   //   affect behavior of the kernel in any way.
   ///
   /// Value of `#TN_USE_MUTEXES`
   unsigned          use_mutexes                : 1;
   ///
   /// Value of `#TN_MUTEX_REC`
   unsigned          mutex_rec                  : 1;
   ///
   /// Value of `#TN_MUTEX_DEADLOCK_DETECT`
   unsigned          mutex_deadlock_detect      : 1;
   ///
   /// Value of `#TN_TICK_LISTS_CNT` minus one
   unsigned          tick_lists_cnt_minus_one   : 8;
   ///
   /// Value of `#TN_API_MAKE_ALIG_ARG`
   unsigned          api_make_alig_arg          : 2;
   ///
   /// Value of `#TN_PROFILER`
   unsigned          profiler                   : 1;
   ///
   /// Value of `#TN_PROFILER_WAIT_TIME`
   unsigned          profiler_wait_time         : 1;
   ///
   /// Value of `#TN_STACK_OVERFLOW_CHECK`
   unsigned          stack_overflow_check       : 1;
   ///
   /// Value of `#TN_DYNAMIC_TICK`
   unsigned          dynamic_tick               : 1;
   ///
   /// Value of `#TN_OLD_EVENT_API`
   unsigned          old_events_api             : 1;
   ///
   /// Architecture-dependent values
   union {
      ///
      /// On some architectures, we don't have any arch-dependent build-time
      /// options, but we need this "dummy" value to avoid errors of crappy
      /// compilers that don't allow empty structure initializers (like ARMCC)
      TN_UWord dummy;
      ///
      /// PIC24/dsPIC-dependent values
      struct {
         ///
         /// Value of `#TN_P24_SYS_IPL`
         unsigned    p24_sys_ipl                : 3;
      } p24;
   } arch;
};

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
   /// (flag (`#TN_STATE_FLAG__SYS_RUNNING` is not set in the `_tn_sys_state`))
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
 * User-provided callback function which is called repeatedly from the idle
 * task loop. Make sure that idle task has enough stack space to call this
 * function.
 *
 * Typically, this callback can be used for things like:
 *
 *   * MCU sleep/idle mode. When system has nothing to do, it often makes sense
 *     to bring processor to some power-saving mode. Of course, the application
 *     is responsible for setting some condition to wake up: typically, it's an
 *     interrupt.
 *   * Calculation of system load. The easiest implementation is to just
 *     increment some variable in the idle task. The faster value grows, the
 *     less busy system is.
 *
 * \attention 
 *    * From withing this callback, it is illegal to invoke `#tn_task_sleep()`
 *      or any other service which could put task to waiting state, because
 *      idle task (from which this function is called) should always be
 *      runnable, by design. If `#TN_DEBUG` option is set, then this is
 *      checked, so if idle task becomes non-runnable, `_TN_FATAL_ERROR()`
 *      macro will be called.
 *
 * @see `tn_sys_start()`
 */
typedef void (TN_CBIdle)(void);

/**
 * User-provided callback function that is called when the kernel detects stack
 * overflow (see `#TN_STACK_OVERFLOW_CHECK`).
 *
 * @param task
 *    Task whose stack is overflowed
 */
typedef void (TN_CBStackOverflow)(struct TN_Task *task);

/**
 * User-provided callback function that is called whenever 
 * deadlock becomes active or inactive.
 * Note: this feature works if only `#TN_MUTEX_DEADLOCK_DETECT` is non-zero.
 *
 * @param active
 *    Boolean value indicating whether deadlock becomes active or inactive.
 *    Note: deadlock might become inactive if, for example, one of tasks
 *    involved in deadlock exits from waiting by timeout.
 *
 * @param mutex
 *    mutex that is involved in deadlock. You may find out other mutexes
 *    involved by means of `mutex->deadlock_list`.
 *
 * @param task
 *    task that is involved in deadlock. You may find out other tasks involved
 *    by means of `task->deadlock_list`.
 */
typedef void (TN_CBDeadlock)(
      TN_BOOL active,
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
 * Initial TNeo system start function, never returns. Typically called
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
 *    User must either use the macro `TN_STACK_ARR_DEF()` for the definition
 *    of stack array, or allocate it manually as an array of `#TN_UWord` with
 *    `#TN_ARCH_STK_ATTR_BEFORE` and `#TN_ARCH_STK_ATTR_AFTER` macros.
 * @param   idle_task_stack_size 
 *    Size of idle task stack, in words (`#TN_UWord`)
 * @param   int_stack            
 *    Pointer to array for interrupt stack.
 *    User must either use the macro `TN_STACK_ARR_DEF()` for the definition
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
 * Among other things, expired \ref tn_timer.h "timers" are fired from this
 * function.
 *
 * For further information, refer to \ref quick_guide "Quick guide".
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
void tn_tick_int_processing(void);

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
 *    Current system ticks count. 
 */
TN_TickCnt tn_sys_time_get(void);


/**
 * Set callback function that should be called whenever deadlock occurs or
 * becomes inactive (say, if one of tasks involved in the deadlock was released
 * from wait because of timeout)
 *
 * $(TN_CALL_FROM_MAIN)
 * $(TN_LEGEND_LINK)
 *
 * **Note:** this function should be called from `main()`, before
 * `tn_sys_start()`.
 *
 * @param cb
 *    Pointer to user-provided callback function.
 *
 * @see `#TN_MUTEX_DEADLOCK_DETECT`
 * @see `#TN_CBDeadlock` for callback function prototype
 */
void tn_callback_deadlock_set(TN_CBDeadlock *cb);

/**
 * Set callback function that is called when the kernel detects stack overflow
 * (see `#TN_STACK_OVERFLOW_CHECK`).
 *
 * For function prototype, refer to `#TN_CBStackOverflow`.
 */
void tn_callback_stack_overflow_set(TN_CBStackOverflow *cb);

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
 * @return `TN_TRUE` if current system context is `#TN_CONTEXT_TASK`,
 *         `TN_FALSE` otherwise.
 *
 * @see `tn_sys_context_get()`
 * @see `enum #TN_Context`
 */
_TN_STATIC_INLINE TN_BOOL tn_is_task_context(void)
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
 * @return `TN_TRUE` if current system context is `#TN_CONTEXT_ISR`,
 *         `TN_FALSE` otherwise.
 *
 * @see `tn_sys_context_get()`
 * @see `enum #TN_Context`
 */
_TN_STATIC_INLINE TN_BOOL tn_is_isr_context(void)
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

/**
 * Disable kernel scheduler and return previous scheduler state.
 *
 * Actual behavior depends on the platform:
 *
 * - On Microchip platforms, only scheduler's interrupt gets disabled. All
 *   other interrupts are not affected, independently of their priorities.
 * - On Cortex-M3/M4 platforms, we can only disable interrupts based on 
 *   priority. So, this function disables all interrupts with lowest priority
 *   (since scheduler works at lowest interrupt priority).
 * - On Cortex-M0/M0+, we have to disable all interrupts.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_CALL_FROM_MAIN)
 * $(TN_LEGEND_LINK)
 *
 * @return
 *    State to be restored later by `#tn_sched_restore()`
 */
_TN_STATIC_INLINE TN_UWord tn_sched_dis_save(void)
{
   return tn_arch_sched_dis_save();
}

/**
 * Restore state of the kernel scheduler. See `#tn_sched_dis_save()`.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_CALL_FROM_MAIN)
 * $(TN_LEGEND_LINK)
 *
 * @param sched_state
 *    Value returned from `#tn_sched_dis_save()`
 */
_TN_STATIC_INLINE void tn_sched_restore(TN_UWord sched_state)
{
   tn_arch_sched_restore(sched_state);
}


#if TN_DYNAMIC_TICK || defined(DOXYGEN_ACTIVE)
/**
 * $(TN_IF_ONLY_DYNAMIC_TICK_SET)
 *
 * Set callbacks related to dynamic tick.
 *
 * \attention This function should be called <b>before</b> `tn_sys_start()`,
 * otherwise, you'll run into run-time error `_TN_FATAL_ERROR()`.
 *
 * $(TN_CALL_FROM_MAIN)
 * $(TN_LEGEND_LINK)
 *
 * @param cb_tick_schedule
 *    Pointer to callback function to schedule next time to call
 *    `tn_tick_int_processing()`, see `#TN_CBTickSchedule` for the prototype.
 * @param cb_tick_cnt_get
 *    Pointer to callback function to get current system tick counter value,
 *    see `#TN_CBTickCntGet` for the prototype.
 */
void tn_callback_dyn_tick_set(
      TN_CBTickSchedule   *cb_tick_schedule,
      TN_CBTickCntGet     *cb_tick_cnt_get
      );
#endif


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _TN_SYS_H

