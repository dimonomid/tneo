
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
#define TN_API_MAKE_ALIG_ARG     TN_API_MAKE_ALIG_ARG__TYPE


#endif // _TN_CFG_H


