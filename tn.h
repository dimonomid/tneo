/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

/**
 * \mainpage TNeoKernel overview
 *
 * TNeoKernel was born as a thorough review and re-implementation of
 * TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 * as well as new features being added, and it is tested carefully with
 * unit-tests.
 *
 * API is changed somewhat, so it's not 100% compatible with TNKernel,
 * hence the new name: TNeoKernel.
 *
 * TNeoKernel is hosted at bitbucket: http://bitbucket.org/dfrank/tneokernel
 *
 * Here is a quick overview on system startup and important implementation
 * details:
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
 *     //-- allocate arrays for idle task and interrupt statically.
 *     //   notice special architecture-dependent macros we use here,
 *     //   they are needed to make sure that all requirements
 *     //   regarding to stack are met.
 *     TN_ARCH_STK_ATTR_BEFORE
 *     unsigned int idle_task_stack[ IDLE_TASK_STACK_SIZE ]
 *     TN_ARCH_STK_ATTR_AFTER;
 *
 *     TN_ARCH_STK_ATTR_BEFORE
 *     unsigned int interrupt_stack[ INTERRUPT_STACK_SIZE ]
 *     TN_ARCH_STK_ATTR_AFTER;
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

/**
 * \file
 *
 * The main kernel header file that should be included by user application;
 * it merely includes subsystem-specific kernel headers.
 */

#ifndef _TN_H
#define _TN_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "core/tn_sys.h"
#include "core/tn_common.h"
#include "core/tn_dqueue.h"
#include "core/tn_eventgrp.h"
#include "core/tn_fmem.h"
#include "core/tn_mutex.h"
#include "core/tn_sem.h"
#include "core/tn_tasks.h"
#include "core/tn_list.h"


//-- include old symbols for compatibility with old projects
#include "core/tn_oldsymbols.h"


/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#endif // _TN_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


