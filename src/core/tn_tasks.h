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
 * \section tn_tasks__tasks Task
 *
 * In TNeo, a task is a branch of code that runs concurrently with other
 * tasks from the programmer's point of view. Indeed, tasks are actually
 * executed using processor time sharing.  Each task can be considered to be an
 * independed program, which executes in its own context (processor registers,
 * stack pointer, etc.).
 *
 * Actually, the term <i>thread</i> is more accurate than <i>task</i>, but the
 * term <i>task</i> historically was used in TNKernel, so TNeo keeps this
 * convention.
 *
 * When kernel decides that it's time to run another task, it performs
 * <i>context switch</i>: current context (at least, values of all registers)
 * gets saved to the preempted task's stack, pointer to currently running 
 * task is altered as well as stack pointer, and context gets restored from
 * the stack of newly running task.
 *
 * \section tn_tasks__states Task states
 *
 * For list of task states and their description, refer to `enum
 * #TN_TaskState`.
 *
 *
 * \section tn_tasks__creating Creating/starting tasks
 *
 * Create task and start task are two separate actions; although you can
 * perform both of them in one step by passing `#TN_TASK_CREATE_OPT_START` flag
 * to the `tn_task_create()` function.
 *
 * \section tn_tasks__stopping Stopping/deleting tasks
 *
 * Stop task and delete task are two separate actions. If task was just stopped
 * but not deleted, it can be just restarted again by calling
 * `tn_task_activate()`. If task was deleted, it can't be just activated: it
 * should be re-created by `tn_task_create()` first.
 *
 * Task stops execution when:
 *
 * - it calls `tn_task_exit()`;
 * - it returns from its task body function (it is the equivalent to
 *   `tn_task_exit(0)`)
 * - some other task calls `tn_task_terminate()` passing appropriate pointer to
 *   `struct #TN_Task`.
 *
 * \section tn_tasks__scheduling Scheduling rules
 *
 * TNeo always runs the most privileged task in state
 * $(TN_TASK_STATE_RUNNABLE). In no circumstances can task run while there is
 * at least one task is in the $(TN_TASK_STATE_RUNNABLE) state with higher
 * priority. Task will run until:
 *
 * - It becomes non-runnable (say, it may wait for something, etc)
 * - Some other task with higher priority becomes runnable.
 *
 * Tasks with the same priority may be scheduled in round robin fashion by
 * getting a predetermined time slice for each task with this priority. 
 * Time slice is set separately for each priority. By default, round robin
 * is turned off for all priorities.
 *
 * \section tn_tasks__idle Idle task
 *
 * TNeo has one system task: an idle task, which has lowest priority.
 * It is always in the state $(TN_TASK_STATE_RUNNABLE), and it runs only when
 * there are no other runnable tasks.
 *
 * User can provide a callback function to be called from idle task, see
 * #TN_CBIdle. It is useful to bring the processor to some kind of real idle
 * state, so that device draws less current.
 *
 */

#ifndef _TN_TASKS_H
#define _TN_TASKS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_sys.h"
#include "tn_list.h"
#include "tn_common.h"

#include "tn_eventgrp.h"
#include "tn_dqueue.h"
#include "tn_fmem.h"
#include "tn_timer.h"



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Task state
 */
enum TN_TaskState {
   ///
   /// This state should never be publicly available.
   /// It may be stored in task_state only temporarily,
   /// while some system service is in progress.
   TN_TASK_STATE_NONE         = 0,
   ///
   /// Task is ready to run (it doesn't mean that it is running at the moment)
   TN_TASK_STATE_RUNNABLE     = (1 << 0),
   ///
   /// Task is waiting. The reason of waiting can be obtained from
   /// `task_wait_reason` field of the `struct TN_Task`.
   ///
   /// @see `enum #TN_WaitReason`
   TN_TASK_STATE_WAIT         = (1 << 1),
   ///
   /// Task is suspended (by some other task)
   TN_TASK_STATE_SUSPEND      = (1 << 2),
   ///
   /// Task was previously waiting, and after this it was suspended
   TN_TASK_STATE_WAITSUSP     = (TN_TASK_STATE_WAIT | TN_TASK_STATE_SUSPEND),
   ///
   /// Task isn't yet activated or it was terminated by `tn_task_terminate()`.
   TN_TASK_STATE_DORMANT      = (1 << 3),


};


/**
 * Task wait reason
 */
enum TN_WaitReason {
   ///
   /// Task isn't waiting for anything
   TN_WAIT_REASON_NONE,
   ///
   /// Task has called `tn_task_sleep()`
   TN_WAIT_REASON_SLEEP,
   ///
   /// Task waits to acquire a semaphore
   /// @see tn_sem.h
   TN_WAIT_REASON_SEM,
   ///
   /// Task waits for some event in the event group to be set
   /// @see tn_eventgrp.h
   TN_WAIT_REASON_EVENT,
   ///
   /// Task wants to put some data to the data queue, and there's no space
   /// in the queue.
   /// @see tn_dqueue.h
   TN_WAIT_REASON_DQUE_WSEND,
   ///
   /// Task wants to receive some data to the data queue, and there's no data
   /// in the queue
   /// @see tn_dqueue.h
   TN_WAIT_REASON_DQUE_WRECEIVE,
   ///
   /// Task wants to lock a mutex with priority ceiling
   /// @see tn_mutex.h
   TN_WAIT_REASON_MUTEX_C,
   ///
   /// Task wants to lock a mutex with priority inheritance
   /// @see tn_mutex.h
   TN_WAIT_REASON_MUTEX_I,
   ///
   /// Task wants to get memory block from memory pool, and there's no free
   /// memory blocks
   /// @see tn_fmem.h
   TN_WAIT_REASON_WFIXMEM,


   ///
   /// Wait reasons count
   TN_WAIT_REASONS_CNT
};

/**
 * Options for `tn_task_create()`
 */
enum TN_TaskCreateOpt {
   ///
   /// whether task should be activated right after it is created.
   /// If this flag is not set, user must activate task manually by calling
   /// `tn_task_activate()`.
   TN_TASK_CREATE_OPT_START = (1 << 0),
   ///
   /// for internal kernel usage only: this option must be provided
   /// when creating idle task
   _TN_TASK_CREATE_OPT_IDLE = (1 << 1),
};

/**
 * Options for `tn_task_exit()`
 */
enum TN_TaskExitOpt {
   ///
   /// whether task should be deleted right after it is exited.
   /// If this flag is not set, user must either delete it manually by
   /// calling `tn_task_delete()` or re-activate it by calling
   /// `tn_task_activate()`.
   TN_TASK_EXIT_OPT_DELETE = (1 << 0),
};

#if TN_PROFILER || DOXYGEN_ACTIVE
/**
 * Timing structure that is managed by profiler and can be read by
 * `#tn_task_profiler_timing_get()` function. This structure is contained in
 * each `struct #TN_Task` structure. 
 *
 * Available if only `#TN_PROFILER` option is non-zero, also depends on
 * `#TN_PROFILER_WAIT_TIME`.
 */
struct TN_TaskTiming {
   ///
   /// Total time when task was running. 
   ///
   /// \attention
   /// This is NOT the time that task was in $(TN_TASK_STATE_RUNNABLE) state:
   /// if task A is preempted by high-priority task B, task A is not running,
   /// but is still in the $(TN_TASK_STATE_RUNNABLE) state. This counter
   /// represents the time task was actually <b>running</b>.
   unsigned long long   total_run_time;
   ///
   /// How many times task got running. It is useful to find an average
   /// value of consecutive running time: `(total_run_time / got_running_cnt)`
   unsigned long long   got_running_cnt;
   ///
   /// Maximum consecutive time task was running.
   unsigned long        max_consecutive_run_time;

#if TN_PROFILER_WAIT_TIME || DOXYGEN_ACTIVE
   ///
   /// Available if only `#TN_PROFILER_WAIT_TIME` option is non-zero.
   ///
   /// Total time when task was not running; time is broken down by reasons of
   /// waiting. 
   ///
   /// For example, to get the time task was waiting for mutexes with priority
   /// inheritance protocol, use: `total_wait_time[ #TN_WAIT_REASON_MUTEX_I ]`
   ///
   /// To get the time task was runnable but preempted by another task, use:
   /// `total_wait_time[ #TN_WAIT_REASON_NONE ]`
   /// 
   unsigned long long   total_wait_time[ TN_WAIT_REASONS_CNT ];
   ///
   /// Available if only `#TN_PROFILER_WAIT_TIME` option is non-zero.
   ///
   /// Maximum consecutive time task was not running; time is broken down by
   /// reasons of waiting.
   ///
   /// @see `total_wait_time`
   unsigned long        max_consecutive_wait_time[ TN_WAIT_REASONS_CNT ];
#endif
};

/**
 * Internal kernel structure for profiling data of task.
 *
 * Available if only `#TN_PROFILER` option is non-zero.
 */
struct _TN_TaskProfiler {
   ///
   /// Tick count of when the task got running or non-running last time.
   TN_TickCnt        last_tick_cnt;
#if TN_PROFILER_WAIT_TIME || DOXYGEN_ACTIVE
   ///
   /// Available if only `#TN_PROFILER_WAIT_TIME` option is non-zero.
   ///
   /// Value of `task->task_wait_reason` when task got non-running last time.
   enum TN_WaitReason   last_wait_reason;
#endif

#if TN_DEBUG
   ///
   /// For internal profiler self-check only: indicates whether task is 
   /// running or not. Available if only `#TN_DEBUG` is non-zero.
   int                  is_running;
#endif
   ///
   /// Main timing structure managed by profiler. Contents of this structure
   /// can be read by `#tn_task_profiler_timing_get()` function.
   struct TN_TaskTiming timing;
};
#endif

/**
 * Task
 */
struct TN_Task {
   /// pointer to task's current top of the stack;
   /// Note that this field **must** be a first field in the struct,
   /// this fact is exploited by platform-specific routines.
   TN_UWord *stack_cur_pt;
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   /// For `struct TN_Task`, we can't make it the very first field, since
   /// stack pointer should be there.
   enum TN_ObjId id_task;
   ///
   /// queue is used to include task in ready/wait lists
   struct TN_ListItem task_queue;     
   ///
   /// timer object to implement task waiting for timeout
   struct TN_Timer timer;
   ///
   /// pointer to object's (semaphore, mutex, event, etc) wait list in which 
   /// task is included for waiting
   struct TN_ListItem *pwait_queue;
   ///
   /// queue is used to include task in creation list
   /// (currently, this list is used for statistics only)
   struct TN_ListItem create_queue;

#if TN_USE_MUTEXES
   ///
   /// list of all mutexes that are locked by task
   struct TN_ListItem mutex_queue;
#if TN_MUTEX_DEADLOCK_DETECT
   ///
   /// list of other tasks involved in deadlock. This list is non-empty
   /// only in emergency cases, and it is here to help you fix your bug
   /// that led to deadlock.
   ///
   /// @see `#TN_MUTEX_DEADLOCK_DETECT`
   struct TN_ListItem deadlock_list;
#endif
#endif

   ///-- lowest address of stack. It is independent of architecture:
   ///   it's always the lowest address (which may be actually origin 
   ///   or end of stack, depending on the architecture)
   TN_UWord *stack_low_addr;
   ///-- Highest address of stack. It is independent of architecture:
   ///   it's always the highest address (which may be actually origin 
   ///   or end of stack, depending on the architecture)
   TN_UWord *stack_high_addr;
   ///
   /// pointer to task's body function given to `tn_task_create()`
   TN_TaskBody *task_func_addr;
   ///
   /// pointer to task's parameter given to `tn_task_create()`
   void *task_func_param;
   ///
   /// base priority of the task (actual current priority may be higher than 
   /// base priority because of mutex)
   int base_priority;
   ///
   /// current task priority
   int priority;
   ///
   /// task state
   enum TN_TaskState task_state;
   ///
   /// reason for waiting (relevant if only `task_state` is
   /// $(TN_TASK_STATE_WAIT) or $(TN_TASK_STATE_WAITSUSP))
   enum TN_WaitReason task_wait_reason;
   ///
   /// waiting result code (reason why waiting finished)
   enum TN_RCode task_wait_rc;
   //
   // remaining time until timeout; may be `#TN_WAIT_INFINITE`.
   //TN_TickCnt tick_count;
   ///
   /// time slice counter
   int tslice_count;
#if 0
   ///
   /// last operation result code, might be used if some service
   /// does not return that code directly
   int last_rc;
#endif
   ///
   /// subsystem-specific fields that are used while task waits for something.
   /// Do note that these fields are grouped by union, so, they must not
   /// interfere with each other. It's quite ok here because task can't wait
   /// for different things.
   union {
      /// fields specific to tn_eventgrp.h
      struct TN_EGrpTaskWait eventgrp;
      ///
      /// fields specific to tn_dqueue.h
      struct TN_DQueueTaskWait dqueue;
      ///
      /// fields specific to tn_fmem.h
      struct TN_FMemTaskWait fmem;
   } subsys_wait;
   ///
   /// Task name for debug purposes, user may want to set it by hand
   const char *name;          
#if TN_PROFILER || DOXYGEN_ACTIVE
   /// Profiler data, available if only `#TN_PROFILER` is non-zero.
   struct _TN_TaskProfiler    profiler;
#endif

   /// Internal flag used to optimize mutex priority algorithms.
   /// For the comments on it, see file tn_mutex.c,
   /// function `_mutex_do_unlock()`.
   unsigned          priority_already_updated : 1;

   /// Flag indicates that task waited for something
   /// This flag is set automatially in `_tn_task_set_waiting()`
   /// Must be cleared manually before calling any service that could sleep,
   /// if the caller is interested in the relevant value of this flag.
   unsigned          waited : 1;


// Other implementation specific fields may be added below

};



/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Construct task and probably start it (depends on options, see below).
 * `id_task` member should not contain `#TN_ID_TASK`, otherwise,
 * `#TN_RC_WPARAM` is returned.
 *
 * Usage example:
 *
 * \code{.c}
 *     #define MY_TASK_STACK_SIZE   (TN_MIN_STACK_SIZE + 200)
 *     #define MY_TASK_PRIORITY     5
 *
 *     struct TN_Task my_task;
 *
 *     //-- define stack array, we use convenience macro TN_STACK_ARR_DEF()
 *     //   for that
 *     TN_STACK_ARR_DEF(my_task_stack, MY_TASK_STACK_SIZE);
 *
 *     void my_task_body(void *param)
 *     {
 *        //-- an endless loop
 *        for (;;){
 *           tn_task_sleep(1);
 *
 *           //-- probably do something useful
 *        }
 *     }
 * \endcode
 *
 *
 *
 * And then, somewhere from other task or from the callback 
 * `#TN_CBUserTaskCreate` given to `tn_sys_start()` :
 * \code{.c}
 *    enum TN_RCode rc = tn_task_create(
 *          &my_task,
 *          my_task_body,
 *          MY_TASK_PRIORITY,
 *          my_task_stack,
 *          MY_TASK_STACK_SIZE,
 *          TN_NULL,                     //-- parameter isn't used
 *          TN_TASK_CREATE_OPT_START  //-- start task on creation
 *          );
 *
 *    if (rc != TN_RC_OK){
 *       //-- handle error
 *    }
 * \endcode
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_LEGEND_LINK)
 *
 * @param task 
 *    Ready-allocated `struct TN_Task` structure. `id_task` member should not
 *    contain `#TN_ID_TASK`, otherwise `#TN_RC_WPARAM` is returned.
 * @param task_func  
 *    Pointer to task body function.
 * @param priority 
 *    Priority for new task. **NOTE**: the lower value, the higher priority.
 *    Must be > `0` and < `(#TN_PRIORITIES_CNT - 1)`.
 * @param task_stack_low_addr    
 *    Pointer to the stack for task.
 *    User must either use the macro `TN_STACK_ARR_DEF()` for the definition
 *    of stack array, or allocate it manually as an array of `#TN_UWord` with
 *    `#TN_ARCH_STK_ATTR_BEFORE` and `#TN_ARCH_STK_ATTR_AFTER` macros.
 * @param task_stack_size 
 *    Size of task stack array, in words (`#TN_UWord`), not in bytes.
 * @param param 
 *    Parameter that is passed to `task_func`.
 * @param opts 
 *    Options for task creation, refer to `enum #TN_TaskCreateOpt`
 *
 * @return
 *    * `#TN_RC_OK` on success;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * `#TN_RC_WPARAM` if wrong params were given;
 *
 * @see `#tn_task_create_wname()`
 * @see `#TN_ARCH_STK_ATTR_BEFORE`
 * @see `#TN_ARCH_STK_ATTR_AFTER`
 */
enum TN_RCode tn_task_create(
      struct TN_Task         *task,
      TN_TaskBody            *task_func,
      int                     priority,
      TN_UWord               *task_stack_low_addr,
      int                     task_stack_size,
      void                   *param,
      enum TN_TaskCreateOpt   opts
      );


/**
 * The same as `tn_task_create()` but with additional argument `name`,
 * which could be very useful for debug.
 */
enum TN_RCode tn_task_create_wname(
      struct TN_Task         *task,
      TN_TaskBody            *task_func,
      int                     priority,
      TN_UWord               *task_stack_low_addr,
      int                     task_stack_size,
      void                   *param,
      enum TN_TaskCreateOpt   opts,
      const char             *name
      );

/**
 * If the task is $(TN_TASK_STATE_RUNNABLE), it is moved to the
 * $(TN_TASK_STATE_SUSPEND) state. If the task is in the $(TN_TASK_STATE_WAIT)
 * state, it is moved to the $(TN_TASK_STATE_WAITSUSP) state.  (waiting +
 * suspended)
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param task    Task to suspend
 *
 * @return
 *    * `#TN_RC_OK` on success;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * `#TN_RC_WSTATE` if task is already suspended or dormant;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 *
 * @see `enum #TN_TaskState`
 */
enum TN_RCode tn_task_suspend(struct TN_Task *task);

/**
 * Release task from $(TN_TASK_STATE_SUSPEND) state. If the given task is in
 * the $(TN_TASK_STATE_SUSPEND) state, it is moved to $(TN_TASK_STATE_RUNNABLE)
 * state; afterwards it has the lowest precedence among runnable tasks with the
 * same priority. If the task is in $(TN_TASK_STATE_WAITSUSP) state, it is
 * moved to $(TN_TASK_STATE_WAIT) state.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param task    Task to release from suspended state
 *
 * @return
 *    * `#TN_RC_OK` on success;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * `#TN_RC_WSTATE` if task is not suspended;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 *
 * @see enum TN_TaskState
 */
enum TN_RCode tn_task_resume(struct TN_Task *task);

/**
 * Put current task to sleep for at most timeout ticks. When the timeout
 * expires and the task was not suspended during the sleep, it is switched to
 * runnable state. If the timeout value is `#TN_WAIT_INFINITE` and the task was
 * not suspended during the sleep, the task will sleep until another function
 * call (like `tn_task_wakeup()` or similar) will make it runnable.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_CAN_SLEEP)
 * $(TN_LEGEND_LINK)
 *
 * @param timeout
 *    Refer to `#TN_TickCnt`
 *
 * @returns
 *    * `#TN_RC_TIMEOUT` if task has slept specified timeout;
 *    * `#TN_RC_OK` if task was woken up from other task by `tn_task_wakeup()`
 *    * `#TN_RC_FORCED` if task was released from wait forcibly by 
 *       `tn_task_release_wait()`
 *    * `#TN_RC_WCONTEXT` if called from wrong context
 *
 * @see TN_TickCnt
 */
enum TN_RCode tn_task_sleep(TN_TickCnt timeout);

/**
 * Wake up task from sleep.
 *
 * Task is woken up if only it sleeps because of call to `tn_task_sleep()`.
 * If task sleeps for some another reason, task won't be woken up, 
 * and `tn_task_wakeup()` returns `#TN_RC_WSTATE`.
 *
 * After this call, `tn_task_sleep()` returns `#TN_RC_OK`.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param task    sleeping task to wake up
 *
 * @return
 *    * `#TN_RC_OK` if successful
 *    * `#TN_RC_WSTATE` if task is not sleeping, or it is sleeping for
 *       some reason other than `tn_task_sleep()` call.
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 *
 */
enum TN_RCode tn_task_wakeup(struct TN_Task *task);

/**
 * The same as `tn_task_wakeup()` but for using in the ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_task_iwakeup(struct TN_Task *task);

/**
 * Activate task that is in $(TN_TASK_STATE_DORMANT) state, that is, it was
 * either just created by `tn_task_create()` without
 * `#TN_TASK_CREATE_OPT_START` option, or terminated.
 *
 * Task is moved from $(TN_TASK_STATE_DORMANT) state to the
 * $(TN_TASK_STATE_RUNNABLE) state.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param task    dormant task to activate
 *
 * @return
 *    * `#TN_RC_OK` if successful
 *    * `#TN_RC_WSTATE` if task is not dormant
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 *
 * @see TN_TaskState
 */
enum TN_RCode tn_task_activate(struct TN_Task *task);

/**
 * The same as `tn_task_activate()` but for using in the ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_task_iactivate(struct TN_Task *task);

/**
 * Release task from $(TN_TASK_STATE_WAIT) state, independently of the reason
 * of waiting.
 *
 * If task is in $(TN_TASK_STATE_WAIT) state, it is moved to
 * $(TN_TASK_STATE_RUNNABLE) state.  If task is in $(TN_TASK_STATE_WAITSUSP)
 * state, it is moved to $(TN_TASK_STATE_SUSPEND) state.
 *
 * `#TN_RC_FORCED` is returned to the waiting task.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * \attention Usage of this function is discouraged, since the need for
 * it indicates bad software design
 *
 * @param task    task waiting for anything
 *
 * @return
 *    * `#TN_RC_OK` if successful
 *    * `#TN_RC_WSTATE` if task is not waiting for anything
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 *
 *
 * @see TN_TaskState
 */
enum TN_RCode tn_task_release_wait(struct TN_Task *task);

/**
 * The same as `tn_task_release_wait()` but for using in the ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_task_irelease_wait(struct TN_Task *task);

/**
 * This function terminates the currently running task. The task is moved to
 * the $(TN_TASK_STATE_DORMANT) state.
 *
 * After exiting, the task may be either deleted by the `tn_task_delete()`
 * function call or reactivated by the `tn_task_activate()` /
 * `tn_task_iactivate()` function call. In this case task starts execution from
 * beginning (as after creation/activation).  The task will have the lowest
 * precedence among all tasks with the same priority in the
 * $(TN_TASK_STATE_RUNNABLE) state.
 *
 * If this function is invoked with `#TN_TASK_EXIT_OPT_DELETE` option set, the
 * task will be deleted after termination and cannot be reactivated (needs
 * recreation).
 *
 * Please note that returning from task body function has the same effect as
 * calling `tn_task_exit(0)`.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 * 
 * @return
 *    Returns if only called from wrong context. Normally, it never returns
 *    (since calling task becomes terminated)
 *
 * @see `#TN_TASK_EXIT_OPT_DELETE`
 * @see `tn_task_delete()`
 * @see `tn_task_activate()`
 * @see `tn_task_iactivate()`
 */
void tn_task_exit(enum TN_TaskExitOpt opts);


/**
 * This function is similar to `tn_task_exit()` but it terminates any task
 * other than currently running one.
 *
 * After task is terminated, the task may be either deleted by the
 * `tn_task_delete()` function call or reactivated by the `tn_task_activate()`
 * / `tn_task_iactivate()` function call. In this case task starts execution
 * from beginning (as after creation/activation).  The task will have the
 * lowest precedence among all tasks with the same priority in the
 * $(TN_TASK_STATE_RUNNABLE) state.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param task    task to terminate
 *
 * @return
 *    * `#TN_RC_OK` if successful
 *    * `#TN_RC_WSTATE` if task is already dormant
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_task_terminate(struct TN_Task *task);

/**
 * This function deletes the task specified by the task. The task must be in
 * the $(TN_TASK_STATE_DORMANT) state, otherwise `#TN_RC_WCONTEXT` will be
 * returned.
 *
 * This function resets the `id_task` field in the task structure to 0 and
 * removes the task from the system tasks list. The task can not be reactivated
 * after this function call (the task must be recreated).
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_LEGEND_LINK)
 *
 * @param task    dormant task to delete
 *
 * @return
 *    * `#TN_RC_OK` if successful
 *    * `#TN_RC_WSTATE` if task is not dormant
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 *
 */
enum TN_RCode tn_task_delete(struct TN_Task *task);

/**
 * Get current state of the task; note that returned state is a bitmask,
 * that is, states could be combined with each other.
 *
 * Currently, only $(TN_TASK_STATE_WAIT) and $(TN_TASK_STATE_SUSPEND) states 
 * are allowed to be set together. Nevertheless, it would be probably good
 * idea to test individual bits in the returned value instead of plain 
 * comparing values.
 *
 * Note that if something goes wrong, variable pointed to by `p_state`
 * isn't touched.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_LEGEND_LINK)
 *
 * @param task
 *    task to get state of
 * @param p_state 
 *    pointer to the location where to store state of the task
 *
 * @return state of the task
 */
enum TN_RCode tn_task_state_get(
      struct TN_Task *task,
      enum TN_TaskState *p_state
      );

#if TN_PROFILER || DOXYGEN_ACTIVE
/**
 * Read profiler timing data of the task. See `struct #TN_TaskTiming` for
 * details on timing data.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param task
 *    Task to get timing data of
 * @param tgt
 *    Target structure to fill with data, should be allocated by caller
 */
enum TN_RCode tn_task_profiler_timing_get(
      const struct TN_Task *task,
      struct TN_TaskTiming *tgt
      );
#endif


/**
 * Set new priority for task.
 * If priority is 0, then task's base_priority is set.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_LEGEND_LINK)
 *
 * \attention this function is obsolete and will probably be removed
 */
enum TN_RCode tn_task_change_priority(struct TN_Task *task, int new_priority);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_TASKS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


