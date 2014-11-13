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
 * Architecture-dependent routines declaration.
 */

#ifndef _TN_ARCH_H
#define _TN_ARCH_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "../core/tn_common.h"



/*******************************************************************************
 *    ACTUAL PORT IMPLEMENTATION
 ******************************************************************************/

#if defined(__TN_ARCH_PIC32MX__)
#  include "pic32/tn_arch_pic32.h"
#elif defined(__TN_ARCH_PIC24_DSPIC__)
#  include "pic24_dspic/tn_arch_pic24.h"
#elif defined(__TN_ARCH_CORTEX_M__)
#  include "cortex_m/tn_arch_cortex_m.h"
#else
#  error "unknown platform"
#endif



#ifdef __cplusplus
extern "C"  {  /*}*/
#endif

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


/**
 * Unconditionally disable <i>system interrupts</i>. 
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * interrupt</i>.
 */
void tn_arch_int_dis(void);

/**
 * Unconditionally enable interrupts
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * interrupt</i>.
 */
void tn_arch_int_en(void);

/**
 * Disable <i>system interrupts</i> and return previous value of status
 * register, atomically.
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * interrupt</i>.
 *
 * @see `tn_arch_sr_restore()`
 */
TN_UWord tn_arch_sr_save_int_dis(void);

/**
 * Restore previously saved status register.
 *
 * @param sr   status register value previously from
 *             `tn_arch_sr_save_int_dis()`
 *
 * @see `tn_arch_sr_save_int_dis()`
 */
void tn_arch_sr_restore(TN_UWord sr);

/**
 * Architecture-dependent system startup routine. Called from `tn_sys_start()`.
 */
void _tn_arch_sys_init(
      TN_UWord            *int_stack,
      unsigned int         int_stack_size
      );


/**
 * Should return top of the stack.
 *
 * A stack implementation is characterized by two attributes:
 *
 * - Where the stack pointer points:
 *   - *Full stack*: The stack pointer points to the last full location;
 *   - *Empty stack*: The stack pointer points to the first empty location.
 * - How the stack grows:
 *   - *Descending stack*: The stack grows downward (i.e., toward lower memory
 *     addresses);
 *   - *Ascending stack*: The stack grows upward (i.e., toward lower higher
 *     addresses).
 *
 * So, depending on the stack implementation used in the particular
 * architecture, the value returned from this function can be one of the
 * following:
 *
 * - `(stack_low_address - 1)` (*Full ascending stack*)
 * - `(stack_low_address + stack_size)` (*Full descending stack*)
 * - `(stack_low_address)` (*Empty ascending stack*)
 * - `(stack_low_address + stack_size - 1)` (*Empty descending stack*)
 *
 * @param   stack_low_address
 *    Start address of the stack array.
 * @param   stack_size
 *    Size of the stack in `#TN_UWord`-s, not in bytes.
 */
TN_UWord *_tn_arch_stack_top_get(
      TN_UWord   *stack_low_address,
      int         stack_size
      );

/**
 * Should put initial CPU context to the provided stack pointer for new task
 * and return current stack pointer.
 *
 * When resulting context gets restored by 
 * `_tn_arch_context_switch_now_nosave()` or `_tn_arch_context_switch_pend()`,
 * the following conditions should be met:
 *
 * - Interrupts are enabled;
 * - Return address is set to `tn_task_exit()`, so that when task body function
 *   returns, `tn_task_exit()` gets automatially called;
 * - Argument 0 contains `param` pointer
 *
 * @param task_func
 *    Pointer to task body function.
 * @param stack_top
 *    Top of the stack, returned by `_tn_arch_stack_top_get()`.
 * @param stack_size
 *    Size of the stack in `#TN_UWord`-s, not in bytes.
 * @param param
 *    User-provided parameter for task body function.
 *
 * @return current stack pointer (top of the stack)
 */
TN_UWord *_tn_arch_stack_init(
      TN_TaskBody   *task_func,
      TN_UWord      *stack_top,
      int            stack_size,
      void          *param
      );

/**
 * Should return 1 if <i>system ISR</i> is currently running, 0 otherwise.
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * ISR</i>.
 */
int _tn_arch_inside_isr(void);

/**
 * Should return 1 if <i>system interrupts</i> are currently disabled, 0
 * otherwise.
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * interrupt</i>.
 */
int _tn_arch_is_int_disabled(void);

/**
 * Called whenever we need to switch context from one task to another.
 *
 * This function typically does NOT switch context; it merely pends it,
 * that is, it sets appropriate interrupt flag. If current level is an
 * application level, interrupt is fired immediately, and context gets
 * switched.
 *
 * But, if it's hard or impossible on particular platform to use dedicated 
 * interrupt flag, this function may just switch the context on its own.
 *
 * **Preconditions:**
 *
 * - interrupts are enabled;
 * - `tn_curr_run_task` points to currently running (preempted) task;
 * - `tn_next_task_to_run` points to new task to run.
 *    
 * **Actions to perform in actual context switching routine:**
 *
 * - save context of the preempted task to its stack;
 * - set `tn_curr_run_task` to `tn_next_task_to_run`;
 * - restore context of the newly activated task from its stack.
 *
 * @see `tn_curr_run_task`
 * @see `tn_next_task_to_run`
 */
void _tn_arch_context_switch_pend(void);

/**
 * Called whenever we need to switch context to new task, but don't save
 * current context. This happens:
 * - At system start, inside `tn_sys_start()` 
 *   (well, it is actually called indirectly but from `_tn_arch_sys_start()`);
 * - At task exit, inside `tn_task_exit()`.
 *
 * This function doesn't need to pend context switch, it switches context
 * immediately.
 *
 * **Preconditions:**
 *
 * - interrupts are disabled;
 * - `tn_next_task_to_run` is already set to needed task.
 *    
 * **Actions to perform:**
 *
 * - set `tn_curr_run_task` to `tn_next_task_to_run`;
 * - restore context of the newly activated task from its stack.
 *
 * @see `tn_curr_run_task`
 * @see `tn_next_task_to_run`
 */
void _tn_arch_context_switch_now_nosave(void);

/**
 * Performs first context switch to the first task (`tn_next_task_to_run` is 
 * already set to needed task).
 *
 * Typically, this function just calls `_tn_arch_context_switch_now_nosave()`,
 * but it also can perform any architecture-dependent actions first, if needed.
 */
void _tn_arch_sys_start(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif









#endif  /* _TN_ARCH_H */

