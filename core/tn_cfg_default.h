
/*******************************************************************************
 *    TNKernel configuration
 *
 ******************************************************************************/


#ifndef _TN_CFG_H
#define _TN_CFG_H


/*******************************************************************************
 *    USER-DEFINED OPTIONS
 ******************************************************************************/

/*
 * TODO: explain
 */
#define TN_CHECK_PARAM       1

/*
 * TODO: explain
 */
#define TN_MEAS_PERFORMANCE  1

/*
 * Whenter mutexes API should be available
 */
#define USE_MUTEXES          1

/*
 * Whether mutexes should allow recursive locking/unlocking
 */
#define TN_MUTEX_REC         1

/*
 * Whether events API should be available
 */
#define USE_EVENTS           1

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
#define TN_API_TASK_CREATE   TN_API_TASK_CREATE__NATIVE


#endif // _TN_CFG_H


