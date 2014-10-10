/*******************************************************************************
 *
 * TNeoKernel: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeoKernel:                copyright © 2014       Dmitry Frank.
 *
 *    TNeoKernel was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeoKernel.
 *
 *    Permission to use, copy, modify, and distribute this software in source
 *    and binary forms and its documentation for any purpose and without fee
 *    is hereby granted, provided that the above copyright notice appear
 *    in all copies and that both that copyright notice and this permission
 *    notice appear in supporting documentation.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE DMITRY FRANK AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DMITRY FRANK OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *    THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/**
 * \file
 *
 * A timer
 *
 */

#ifndef _TN_TIMER_H
#define _TN_TIMER_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_Timer;


/**
 * Prototype of the function that should be called by timer.
 *
 * When timer fires, function gets called by the kernel. Be aware of the
 * following:
 *    - Function is called from ISR context (namely, from *system timer* ISR);
 *    - Function is called with global interrupts disabled;
 *
 * So, it's legal to call interrupt services from this function, and the
 * function should be as fast as possible.
 *
 * @param timer
 *    Timer that caused function to be called
 * @param p_user_data
 *    The user-provided pointer given to `tn_timer_create()`.
 */
typedef void (TN_TimerFunc)(struct TN_Timer *timer, void *p_user_data);

/**
 * Timer
 */
struct TN_Timer {
   ///
   /// A list item to be included in the system timer queue
   struct TN_ListItem timer_queue;
   ///
   /// Function to be called by timer
   TN_TimerFunc *func;
   ///
   /// User data pointer that is given to user-provided `func`.
   void *p_user_data;
   //
   // Timeout value that is set by user
   //TN_Timeout timeout_base;
   ///
   /// Current (left) timeout value
   TN_Timeout timeout_cur;
   ///
   /// id for object validity verification
   enum TN_ObjId id_timer;
};


/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Construct the timer. `id_timer` field should not contain
 * `#TN_ID_TIMER`, otherwise, `#TN_RC_WPARAM` is returned.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param timer
 *    Pointer to already allocated `struct TN_Timer`
 * @param func
 *    Function to be called by timer. See `TN_TimerFunc()`
 * @param p_user_data
 *    User data pointer that is given to user-provided `func`.
 *
 * @return 
 *    * `#TN_RC_OK` if timer was successfully created;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#TN_RC_WPARAM`.
 */
enum TN_RCode tn_timer_create(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      );

/**
 * Destruct the timer. If the timer is active, it is cancelled first.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_LEGEND_LINK)
 *
 * @param timer     timer to destruct
 *
 * @return 
 *    * `#TN_RC_OK` if timer was successfully deleted;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_timer_delete(struct TN_Timer *timer);

/**
 * Start the timer, that is, schedule the timer's function (given to
 * `tn_timer_create()`) to be called later by the kernel. See `TN_TimerFunc()`.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_LEGEND_LINK)
 *
 * @param timer
 *    Timer to start
 * @param timeout
 *    Number of system ticks after which timer should fire (i.e. function 
 *    should be called). **Note** that `timeout` can't be `#TN_WAIT_INFINITE` or
 *    `0`.
 *
 * @return 
 *    * `#TN_RC_OK` if timer was successfully started;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * `#TN_RC_WPARAM` if wrong params were given: say, `timeout` is either 
 *      `#TN_WAIT_INFINITE` or `0`.
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_timer_start(struct TN_Timer *timer, TN_Timeout timeout);

/**
 * The same as `tn_timer_start()` but for using in the ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_timer_istart(struct TN_Timer *timer, TN_Timeout timeout);

/**
 * If timer is active, cancel it. If timer is already inactive, nothing is
 * changed.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_LEGEND_LINK)
 *
 * @param timer
 *    Timer to cancel
 *
 * @return 
 *    * `#TN_RC_OK` if timer was successfully cancelled;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_timer_cancel(struct TN_Timer *timer);

/**
 * The same as `tn_timer_cancel()` but for using in the ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_timer_icancel(struct TN_Timer *timer);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_TIMER_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


