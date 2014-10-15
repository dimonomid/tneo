/**
 * \file
 *
 * This is an usage example of queues in TNeoKernel.
 *
 * When you need to build a queue of some data objects from one task to
 * another, the common pattern in the TNeoKernel is to use fixed memory pool
 * and queue together. Number of elements in the memory pool is equal to the
 * number of elements in the queue. So when you need to send the message,
 * you get block from the memory pool, fill it with data, and send pointer
 * to it by means of the queue.
 *
 * When you receive the message, you handle the data, and release the memory
 * back to the memory pool.
 *
 * This example project demonstrates this technique: there are two tasks:
 *
 * - producer task (which sends messages to the consumer)
 * - consumer task (which receives messages from the producer)
 * 
 * Actual functionality is, of course, very simple: each message contains 
 * the following:
 *
 * - command to perform: currently there is just a single command: toggle pin
 * - pin number: obviously, number of the pin for which command should
 *   be performed.
 *
 * For the message structure, refer to `struct TaskConsumerMsg` in the 
 * file `task_consumer.c`. Note, however, that it is a "private" structure:
 * producer doesn't touch it directly; instead, it uses special function:
 * `task_consumer_msg_send()`, which sends message to the queue.
 *
 * So the producer just sends new message each 100 ms, specifying
 * one of three pins. Consumer, consequently, toggles the pin specified
 * in the message.
 *
 * Before we can get to the business, we need to initialize things.
 * How it is done:
 *
 * - `init_task_create()`, which is called from the `tn_sys_start()`, 
 *   creates first application task: the producer task.
 * - producer task gets executed and first of all it initializes the
 *   essential application stuff, by calling `appl_init()`:
 *
 *   - `queue_example_init()` is called, in which common application
 *     objects are created: in this project, there's just a single 
 *     object: event group that is used to synchronize task
 *     initialization. See `enum E_QueExampleFlag`.
 *   - create consumer task
 *   - wait until it is initialized (i.e. wait for the flag 
 *     QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT in the event group returned 
 *     by `queue_example_eventgrp_get()`)
 *
 * - consumer task gets executed
 *   - gpio is configured: we need to set up certain pins to output
 *     as well as initially clear them
 *   - consumer initializes its own objects:
 *     - fixed memory pool
 *     - queue
 *   - sets the flag QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT
 *   - enters endless loop in which it will receive and 
 *     handle messages. Now, it is going to wait for first message.
 *
 * - producer task gets executed and enters its endless loop in which
 *   it will send messages to the consumer.
 *
 * At this point, everything is initialized.
 *
 */

#ifndef _QUEUE_EXAMPLE_H
#define _QUEUE_EXAMPLE_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- include architecture-specific things,
//   at least, there is SOFTWARE_BREAK macro
#include "queue_example_arch.h"


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


