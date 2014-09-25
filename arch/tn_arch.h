
/**
 * \file
 *
 * Architecture-dependent routines.
 */

#ifndef _TN_ARCH_H
#define _TN_ARCH_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_common.h"
#include "tn_tasks.h"


#ifdef __cplusplus
extern "C"  {  /*}*/
#endif

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


/**
 * Unconditionally disable interrupts
 */
void tn_arch_int_dis(void);

/**
 * Unconditionally enable interrupts
 */
void tn_arch_int_en(void);

/**
 * Save current status register value and disable interrupts atomically
 *
 * @see `tn_arch_sr_restore()`
 */
unsigned int tn_arch_sr_save_int_dis(void);

/**
 * Restore previously saved status register
 *
 * @param sr   status register value previously from
 *             `tn_arch_sr_save_int_dis()`
 *
 * @see `tn_arch_sr_save_int_dis()`
 */
void tn_arch_sr_restore(unsigned int sr);




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
 * * no interrupts are set up yet, so, it's like interrupts disabled
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
    #include "pic32/tn_arch_pic32.h"
#else
    #error "unknown platform"
#endif




#endif  /* _TN_ARCH_H */

