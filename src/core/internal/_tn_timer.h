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

#ifndef __TN_TIMER_H
#define __TN_TIMER_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "_tn_sys.h"





#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

///
/// "generic" list of timers, for details, refer to \ref timers_implementation
extern struct TN_ListItem tn_timer_list__gen;
///
/// "tick" lists of timers, for details, refer to \ref timers_implementation
extern struct TN_ListItem tn_timer_list__tick[ TN_TICK_LISTS_CNT ];




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Should be called once at system startup (from `#tn_sys_start()`).
 * It merely resets all timer lists.
 */
void _tn_timers_init(void);

/**
 * Should be called from $(TN_SYS_TIMER_LINK) interrupt. It performs all
 * necessary timers housekeeping: moving them between lists, firing them, etc.
 *
 * See \ref timers_implementation for details.
 */
void _tn_timers_tick_proceed(void);

/**
 * Actual worker function that is called by `#tn_timer_start()`.
 * Interrupts should be disabled when calling it.
 */
enum TN_RCode _tn_timer_start(struct TN_Timer *timer, TN_Timeout timeout);

/**
 * Actual worker function that is called by `#tn_timer_cancel()`.
 * Interrupts should be disabled when calling it.
 */
enum TN_RCode _tn_timer_cancel(struct TN_Timer *timer);

/**
 * Actual worker function that is called by `#tn_timer_create()`.
 */
enum TN_RCode _tn_timer_create(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      );

/**
 * Actual worker function that is called by `#tn_timer_set_func()`.
 */
enum TN_RCode _tn_timer_set_func(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      );

/**
 * Actual worker function that is called by `#tn_timer_is_active()`.
 * Interrupts should be disabled when calling it.
 */
TN_BOOL _tn_timer_is_active(struct TN_Timer *timer);

/**
 * Actual worker function that is called by `#tn_timer_time_left()`.
 * Interrupts should be disabled when calling it.
 */
TN_Timeout _tn_timer_time_left(struct TN_Timer *timer);





/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Checks whether given timer object is valid 
 * (actually, just checks against `id_timer` field, see `enum #TN_ObjId`)
 */
static inline TN_BOOL _tn_timer_is_valid(
      struct TN_Timer   *timer
      )
{
   return (timer->id_timer == TN_ID_TIMER);
}



#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // __TN_TIMER_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


