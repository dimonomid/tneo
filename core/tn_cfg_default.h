
/*******************************************************************************
 *    TNKernel configuration
 *
 ******************************************************************************/


#ifndef _TN_CFG_DEFAULT_H
#define _TN_CFG_DEFAULT_H


/*******************************************************************************
 *    USER-DEFINED OPTIONS
 ******************************************************************************/

/*
 * Allows additional param checking for most of the system functions.
 * It's surely useful for debug, but probably better to remove in release.
 */
#ifndef TN_CHECK_PARAM
#  define TN_CHECK_PARAM         1
#endif

/*
 * Whether timer task should be used.
 * If it isn't used:
 *    - its job (managing tn_wait_timeout_list) is performed
 *      right in tn_tick_int_processing(), and we avoid useless context switch;
 *    - application callback is called from idle task, so its stack size
 *      now depends on application.
 *
 * When tnkernel uses separate interrupt stack (as it is in PIC32 port),
 * timer task is completely useless.
 * Benchmarks of system tick:
 *    - with timer task: 682 cycles
 *    - with timer task: 251 cycle.
 *
 * It's about 2.7 times faster without timer task, and you can save RAM
 * (since you shouldn't allocate it for timer task)
 * You just need to make sure your interrupt stack size is enough.
 */
#ifndef TN_USE_TIMER_TASK
#  define TN_USE_TIMER_TASK      0
#endif

/*
 * If set, you can check tn_idle_count value that is incremented in idle-task,
 * so that you can see how busy is your system.
 */
#ifndef TN_MEAS_PERFORMANCE
#  define TN_MEAS_PERFORMANCE    1
#endif

/*
 * Whenter mutexes API should be available
 */
#ifndef TN_USE_MUTEXES
#  define TN_USE_MUTEXES         1
#endif

/*
 * Whether mutexes should allow recursive locking/unlocking
 */
#ifndef TN_MUTEX_REC
#  define TN_MUTEX_REC           0
#endif

/*
 * Whether events API should be available
 */
#ifndef TN_USE_EVENTS
#  define TN_USE_EVENTS          1
#endif

/*
 * API option for tn_task_create() :
 *
 *    TN_API_TASK_CREATE__NATIVE: 
 *             task_stack_start should be given to tn_task_create()
 *             as a bottom address of the stack.
 *             Typically done as follows:
 *             (assuming you have array of int named "my_stack")
 *
 *                &(my_stack[my_stack_size - 1])
 *
 *
 *    TN_API_TASK_CREATE__CONVENIENT:
 *             task_stack_start should be given to tn_task_create()
 *             as an address of array. Just give your array to
 *             tn_task_create(), and you're done.
 */
#ifndef TN_API_TASK_CREATE
#  define TN_API_TASK_CREATE   TN_API_TASK_CREATE__NATIVE
#endif

/*
 * API option for MAKE_ALIG() macro.
 *
 * There is a terrible mess with MAKE_ALIG() macro: TNKernel docs specify
 * that the argument of it should be the size to align, but almost
 * all ports, including "original" one, defined it so that it takes
 * type, not size.
 *
 * But the port by AlexB implemented it differently
 * (i.e. accordingly to the docs)
 *
 * When I was moving from the port by AlexB to another one, 
 * do you have any idea how much time it took me to figure out
 * why do I have rare weird bug? :)
 *
 * So, available options:
 *
 *    TN_API_MAKE_ALIG_ARG__TYPE: 
 *             In this case, you should use macro like this: 
 *                MAKE_ALIG(struct my_struct)
 *             This way is used in the majority of TNKernel ports.
 *             (actually, in all ports except the one by AlexB)
 *
 *    TN_API_MAKE_ALIG_ARG__SIZE:
 *             In this case, you should use macro like this: 
 *                MAKE_ALIG(sizeof(struct my_struct))
 *             This way is stated in TNKernel docs
 *             and used in the port for dsPIC/PIC24/PIC32 by AlexB.
 */
#ifndef TN_API_MAKE_ALIG_ARG
#  define TN_API_MAKE_ALIG_ARG     TN_API_MAKE_ALIG_ARG__TYPE
#endif


#endif // _TN_CFG_DEFAULT_H


