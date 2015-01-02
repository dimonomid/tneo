
/*******************************************************************************
 *    TNKernel configuration
 *
 ******************************************************************************/


#ifndef _TN_CFG_H
#define _TN_CFG_H


/*******************************************************************************
 *    USER-DEFINED OPTIONS
 ******************************************************************************/

/**
 * Enables additional param checking for most of the system functions.
 * It's surely useful for debug, but probably better to remove in release.
 * If it is set, most of the system functions are able to return two additional
 * codes:
 *
 *    * `TN_RC_WPARAM` if wrong params were given;
 *    * `TN_RC_INVALID_OBJ` if given pointer doesn't point to a valid object.
 *      Object validity is checked by means of the special ID field of type
 *      `enum TN_ObjId`.
 *
 * @see `enum TN_ObjId`
 */
#define TN_CHECK_PARAM       1

/**
 * Allows additional internal self-checking, useful to catch internal
 * TNeo bugs as well as illegal kernel usage (e.g. sleeping in the idle 
 * task callback). Produces a couple of extra instructions which usually just
 * causes debugger to stop if something goes wrong.
 */
#define TN_DEBUG             1

/**
 * Whether old TNKernel names (definitions, functions, etc) should be available.
 * If you're porting your existing application written for TNKernel,
 * it is definitely worth enabling.
 * If you start new project with TNeo, it's better to avoid old names.
 */
#define TN_OLD_TNKERNEL_NAMES  0

/*
 * Whenter mutexes API should be available
 */
#define TN_USE_MUTEXES       1

/*
 * Whether mutexes should allow recursive locking/unlocking
 */
#define TN_MUTEX_REC         1

/*
 * Whether RTOS should detect deadlocks and notify user about them
 * via callback (see tn_event_callback_set() function)
 */
#define TN_MUTEX_DEADLOCK_DETECT  1

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
#define TN_API_MAKE_ALIG_ARG     TN_API_MAKE_ALIG_ARG__SIZE


#endif // _TN_CFG_H


