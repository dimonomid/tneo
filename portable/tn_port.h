
/**
 * \file
 */

#ifndef _TN_PORT_H
#define _TN_PORT_H


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

void tn_cpu_int_enable(void);
void tn_cpu_int_disable(void);

/**
 * Should return start stack address, which may be either the lowest address of
 * the stack array or the highest one.
 *
 * @param   stack_low_address    start address of the stack array.
 * @param   stack_size           size of the stack in `int`-s, not in bytes.
 */
unsigned int *_tn_arch_stack_start_get(
      unsigned int *stack_low_address,
      int stack_size
      );

/**
 * Should initialize stack for new task.
 *
 * @param task_func
 *    Pointer to task body function.
 * @param stack_start
 *    Start address of the stack, returned by `_tn_arch_stack_start_get()`.
 * @param param
 *    User-provided parameter for task body function.
 */
unsigned int *tn_stack_init(
      TN_TaskBody *task_func,
      unsigned int *stack_start,
      void *param
      );

/**
 * Should return 1 if ISR is currently running, 0 otherwise
 */
int  tn_inside_int(void);

/**
 * Called whenever we need to switch context to other task.
 *
 * **Preconditions:**
 *
 * * interrupts are enabled
 * * `tn_curr_run_task` points to currently running (preempted) task
 * * `tn_next_task_to_run` points to new task to run
 *    
 * **Actions to perform:**
 *
 * * save context of the preempted task to its stack
 * * set `tn_curr_run_task` to `tn_next_task_to_run`
 * * switch context to it
 */
void  tn_switch_context(void);

/**
 * Called when some task calls `tn_task_exit()`.
 *
 * **Preconditions:**
 *
 * * interrupts are disabled
 * * `tn_next_task_to_run` is already set to other task
 *    
 * **Actions to perform:**
 *
 * * set `tn_curr_run_task` to `tn_next_task_to_run`
 * * switch context to it
 */
void  tn_switch_context_exit(void);

unsigned int tn_cpu_save_sr(void);
void  tn_cpu_restore_sr(unsigned int sr);
void  tn_start_exe(void);





#ifdef __cplusplus
}  /* extern "C" */
#endif


/*******************************************************************************
 *    ACTUAL PORT IMPLEMENTATION
 ******************************************************************************/

#if defined(__PIC32MX__)
    #include "pic32/tn_port_pic32.h"
#else
    #error "unknown platform"
#endif




#endif  /* _TN_PORT_H */

