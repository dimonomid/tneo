/**
 * \file
 *
 * Example project that demonstrates usage of queues in TNeoKernel.
 */

#ifndef _EXAMPLE_QUEUE_EVENTGRP_CONN_H
#define _EXAMPLE_QUEUE_EVENTGRP_CONN_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- include architecture-specific things,
//   at least, there is SOFTWARE_BREAK macro
#include "example_queue_eventgrp_conn_arch.h"


/*******************************************************************************
 *    EXTERN TYPES
 ******************************************************************************/

/// Application common event group (see `enum E_QueExampleFlag`)
struct TN_EventGrp;



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

enum E_QueExampleFlag {
   ///
   /// Flag indicating that consumer task is initialized
   QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT = (1 << 0),
   ///
   /// Flag indicating that producer task is initialized
   QUE_EXAMPLE_FLAG__TASK_PRODUCER_INIT = (1 << 1),
   ///
   /// Flag indicating that consumer's queue A is not empty
   QUE_EXAMPLE_FLAG__MSG_A = (1 << 2),
   ///
   /// Flag indicating that consumer's queue B is not empty
   QUE_EXAMPLE_FLAG__MSG_B = (1 << 3),
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



/**
 * Architecture-dependent:
 * At least, we need to initialize GPIO pins here
 */
void queue_example_arch_init(void);

/**
 * Architecture-dependent:
 * Toggle pins specified by pin_mask
 */
void queue_example_arch_pins_toggle(int pin_mask);

/**
 * Architecture-dependent:
 * Set pins specified by pin_mask
 */
void queue_example_arch_pins_set(int pin_mask);

/**
 * Architecture-dependent:
 * Clear pins specified by pin_mask
 */
void queue_example_arch_pins_clear(int pin_mask);



#endif // _EXAMPLE_QUEUE_EVENTGRP_CONN_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


