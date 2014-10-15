/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _QUEUE_EXAMPLE_H
#define _QUEUE_EXAMPLE_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "example_arch.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

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
#define SYSRETVAL_CHECK_TO(x) \
   ({int __rv = (x); if (__rv != TERR_NO_ERR && __rv != TERR_TIMEOUT){SOFTWARE_BREAK();} __rv;})

/**
 * The same as `SYSRETVAL_CHECK_TO()`, but it allows `#TN_RC_OK` only.
 *
 * Since there is only one return code allowed, usage is simple:
 *
 * \code{.c}
 *    SYSRETVAL_CHECK(tn_queue_send(&my_queue, p_data, MY_TIMEOUT));
 * \endcode
 */
#define SYSRETVAL_CHECK(x) \
   ({int __rv = (x); if (__rv != TERR_NO_ERR){SOFTWARE_BREAK();} __rv;})



/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Each example should define this funtion: it creates first application task
 */
void init_task_create(void);


#endif // _QUEUE_EXAMPLE_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


