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

#ifndef __TN_SYS_H
#define __TN_SYS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_arch.h"
#include "tn_list.h"
#include "tn_tasks.h"





#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/// list of all ready to run (TN_TASK_STATE_RUNNABLE) tasks
extern struct TN_ListItem tn_ready_list[TN_PRIORITIES_CNT];

/// list all created tasks (now it is used for statictic only)
extern struct TN_ListItem tn_create_queue;

/// count of created tasks
extern volatile int tn_created_tasks_cnt;           

/// system state flags
extern volatile enum TN_StateFlag tn_sys_state;

/// task that runs now
extern struct TN_Task *tn_curr_run_task;

/// task that should run as soon as possible (after context switch)
extern struct TN_Task *tn_next_task_to_run;

/// bitmask of priorities with runnable tasks.
/// lowest priority bit (1 << (TN_PRIORITIES_CNT - 1)) should always be set,
/// since this priority is used by idle task and it is always runnable.
extern volatile unsigned int tn_ready_to_run_bmp;

/// system time that can be returned by `tn_sys_time_get()`; it is also used
/// by tn_timer.h subsystem.
extern volatile unsigned int tn_sys_time_count;

/// current interrupt nesting count. Used by macros
/// `tn_soft_isr()` and `tn_srs_isr()`.
extern volatile int tn_int_nest_count;

/// saved task stack pointer. Needed when switching stack pointer from 
/// task stack to interrupt stack.
extern void *tn_user_sp;

/// saved ISR stack pointer. Needed when switching stack pointer from
/// interrupt stack to task stack.
extern void *tn_int_sp;

///
/// idle task structure
extern struct TN_Task tn_idle_task;




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#if !defined(container_of)
/* given a pointer @ptr to the field @member embedded into type (usually
 * struct) @type, return pointer to the embedding instance of @type. */
#define container_of(ptr, type, member) \
   ((type *)((char *)(ptr)-(char *)(&((type *)0)->member)))
#endif


#ifndef TN_DEBUG
#  error TN_DEBUG is not defined
#endif

#if TN_DEBUG
#define  _TN_BUG_ON(cond, ...){              \
   if (cond){                                \
      _TN_FATAL_ERROR(__VA_ARGS__);          \
   }                                         \
}
#else
#define  _TN_BUG_ON(cond)     /* nothing */
#endif




/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Remove all tasks from wait queue, returning the TN_RC_DELETED code.
 */
void _tn_wait_queue_notify_deleted(struct TN_ListItem *wait_queue);


/**
 * Set system flags by bitmask.
 * Given flags value will be OR-ed with existing flags.
 *
 * @return previous tn_sys_state value.
 */
enum TN_StateFlag _tn_sys_state_flags_set(enum TN_StateFlag flags);

/**
 * Clear flags by bitmask
 * Given flags value will be inverted and AND-ed with existing flags.
 *
 * @return previous tn_sys_state value.
 */
enum TN_StateFlag _tn_sys_state_flags_clear(enum TN_StateFlag flags);

#if TN_MUTEX_DEADLOCK_DETECT
/**
 * This function is called when deadlock becomes active or inactive 
 * (this is detected by mutex subsystem).
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
 *    
 */
void _tn_cry_deadlock(BOOL active, struct TN_Mutex *mutex, struct TN_Task *task);
#endif

/**
 * memcpy that operates with `#TN_UWord`s. Note: memory overlapping isn't
 * handled: data is always copied from lower addresses to higher ones.
 *
 * @param tgt
 *    Target memory address
 *
 * @param src
 *    Source memory address
 *
 * @param size_uwords
 *    Number of words to copy
 *
 */
void _tn_memcpy_uword(
      TN_UWord *tgt, const TN_UWord *src, unsigned int size_uwords
      );




/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Checks whether context switch is needed (that is, if currently running task 
 * is not the highest-priority task in the $(TN_TASK_STATE_RUNNABLE) state)
 *
 * @return `#TRUE` if context switch is needed
 */
static inline BOOL _tn_need_context_switch(void)
{
   return (tn_curr_run_task != tn_next_task_to_run);
}

/**
 * If context switch is needed (see `#_tn_need_context_switch()`), 
 * context switch is pended (see `#_tn_arch_context_switch_pend()`)
 */
static inline void _tn_context_switch_pend_if_needed(void)
{
   if (_tn_need_context_switch()){
      _tn_arch_context_switch_pend();
   }
}


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // __TN_SYS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


