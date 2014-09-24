
/**
 * \file
 */

#ifndef _TN_ARCH_INTERNAL_H
#define _TN_ARCH_INTERNAL_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_arch.h"
#include "tn_tasks.h"



#ifdef __cplusplus
extern "C"  {  /*}*/
#endif

/*******************************************************************************
 *    INTERNAL KERNEL FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Should return start stack address, which may be either the lowest address of
 * the stack array or the highest one, depending on the architecture.
 *
 * @param   stack_low_address    start address of the stack array.
 * @param   stack_size           size of the stack in `int`-s, not in bytes.
 */
unsigned int *_tn_arch_stack_start_get(
      unsigned int *stack_low_address,
      int stack_size
      );

/**
 * Should initialize stack for new task and return current stack pointer.
 *
 * @param task_func
 *    Pointer to task body function.
 * @param stack_start
 *    Start address of the stack, returned by `_tn_arch_stack_start_get()`.
 * @param param
 *    User-provided parameter for task body function.
 *
 * @return current stack pointer (top of the stack)
 */
unsigned int *_tn_arch_stack_init(
      TN_TaskBody *task_func,
      unsigned int *stack_start,
      void *param
      );

/**
 * Should return 1 if ISR is currently running, 0 otherwise
 */
int _tn_arch_inside_isr(void);

/**
 * Called whenever we need to switch context to other task.
 *
 * **Preconditions:**
 *
 * * interrupts are enabled;
 * * `tn_curr_run_task` points to currently running (preempted) task;
 * * `tn_next_task_to_run` points to new task to run.
 *    
 * **Actions to perform:**
 *
 * * save context of the preempted task to its stack;
 * * set `tn_curr_run_task` to `tn_next_task_to_run`;
 * * switch context to it.
 *
 * @see `tn_curr_run_task`
 * @see `tn_next_task_to_run`
 */
void _tn_arch_context_switch(void);

/**
 * Called when some task calls `tn_task_exit()`.
 *
 * **Preconditions:**
 *
 * * interrupts are disabled;
 * * `tn_next_task_to_run` is already set to other task.
 *    
 * **Actions to perform:**
 *
 * * set `tn_curr_run_task` to `tn_next_task_to_run`;
 * * switch context to it.
 *
 * @see `tn_curr_run_task`
 * @see `tn_next_task_to_run`
 */
void _tn_arch_context_switch_exit(void);

/**
 * Should perform first context switch (to idle task, but this doesn't matter).
 *
 * **Preconditions:**
 *
 * * no interrupts are set up yet, so, it's like interrupts disabled,
 *   but they actually aren't;
 * * `tn_next_task_to_run` is already set to idle task.
 *    
 * **Actions to perform:**
 *
 * * set `TN_STATE_FLAG__SYS_RUNNING` flag in the `tn_sys_state` variable;
 * * set `tn_curr_run_task` to `tn_next_task_to_run`;
 * * switch context to it.
 *
 * @see `TN_STATE_FLAG__SYS_RUNNING`
 * @see `tn_sys_state`
 * @see `tn_curr_run_task`
 * @see `tn_next_task_to_run`
 */
void _tn_arch_system_start(void);



#ifdef __cplusplus
}  /* extern "C" */
#endif


/*******************************************************************************
 *    ACTUAL PORT IMPLEMENTATION
 ******************************************************************************/

#if defined(__PIC32MX__)
    #include "pic32/tn_arch_pic32_internal.h"
#else
    #error "unknown platform"
#endif




#endif  /* _TN_ARCH_INTERNAL_H */

