/**
 * \file
 *
 * When you need to build a queue of some data objects from one task to
 * another, the common pattern in the TNeoKernel is to use fixed memory pool
 * and queue together. Number of elements in the memory pool is equal to the
 * number of elements in the queue. So when you need to send the message,
 * you get block from the memory pool, fill it with data, and send pointer
 * to it by means of the queue.
 *
 * When you receive the message, you handle the data, and free the memory
 * back to the memory pool.
 *
 */

#ifndef _QUEUE_EXAMPLE_H
#define _QUEUE_EXAMPLE_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- include architecture-specific things,
//   at least, there is SOFTWARE_BREAK macro
#include "example_arch.h"


/*******************************************************************************
 *    EXTERN TYPES
 ******************************************************************************/

struct TN_EventGrp;

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

enum E_QueExampleFlag {
   QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT = (1 << 0),
   QUE_EXAMPLE_FLAG__TASK_PRODUCER_INIT = (1 << 1),
};



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Macro for checking value returned from system service.
 *
 * If you ignore the value returned by any system service, it is almost always
 * a BAD idea: if something goes wrong, the sooner you know it, the better.
 * But, checking error for every system service is a kind of annoying, so,
 * simple macro was invented for that. 
 *
 * In most situations, any values other than `#TN_RC_OK` or `#TN_RC_TIMEOUT`
 * are not allowed (i.e. should never happen in normal device operation).
 *
 * So, if error value is neither `#TN_RC_OK` nor `#TN_RC_TIMEOUT`, this macro
 * issues a software break.
 *
 * If you need to allow `#TN_RC_OK` only, use `SYSRETVAL_CHECK()` instead (see
 * below)
 *
 * Usage is as follows:
 *
 * \code{.c}
 *    enum TN_RCode rc 
 *       = SYSRETVAL_CHECK_TO(tn_queue_send(&my_queue, p_data, MY_TIMEOUT));
 *
 *    switch (rc){
 *       case TN_RC_OK:
 *          //-- handle successfull operation
 *          break;
 *       case TN_RC_TIMEOUT:
 *          //-- handle timeout
 *          break;
 *       default:
 *          //-- should never be here
 *          break;
 *    }
 * \endcode
 * 
 */
#define SYSRETVAL_CHECK_TO(x)                                     \
   ({                                                             \
      int __rv = (x);                                             \
      if (__rv != TN_RC_OK && __rv != TN_RC_TIMEOUT){             \
         SOFTWARE_BREAK();                                        \
      }                                                           \
      /* like, return __rv */                                     \
      __rv;                                                       \
    })

/**
 * The same as `SYSRETVAL_CHECK_TO()`, but it allows `#TN_RC_OK` only.
 *
 * Since there is only one return code allowed, usage is simple:
 *
 * \code{.c}
 *    SYSRETVAL_CHECK(tn_queue_send(&my_queue, p_data, MY_TIMEOUT));
 * \endcode
 */
#define SYSRETVAL_CHECK(x)                                        \
   ({                                                             \
      int __rv = (x);                                             \
      if (__rv != TN_RC_OK){                                      \
         SOFTWARE_BREAK();                                        \
      }                                                           \
      /* like, return __rv */                                     \
      __rv;                                                       \
    })



/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Each example should define this funtion: it creates first application task
 */
void init_task_create(void);

/**
 * Initialization of application common objects, must be called once 
 * from the initial application task (which is created in init_task_create())
 */
void queue_example_init(void);

/**
 * Returns pointer to the application event group.
 * See `enum E_QueExampleFlag`
 *
 * Do note that you must call `queue_example_init()` before
 * you can get eventgrp.
 */
struct TN_EventGrp *queue_example_eventgrp_get(void);

#endif // _QUEUE_EXAMPLE_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


