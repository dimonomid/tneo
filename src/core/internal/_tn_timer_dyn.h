/*******************************************************************************
 *
 * TNeo: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright 2013, 2014 Anders Montonen.
 *    TNeo:                      copyright 2014       Dmitry Frank.
 *
 *    TNeo was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeo.
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

#ifndef __TN_TIMER_DYN_H
#define __TN_TIMER_DYN_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_common.h"
#include "tn_arch.h"
#include "tn_list.h"
#include "tn_timer.h"


#if TN_DYNAMIC_TICK



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

///
/// $(TN_IF_ONLY_DYNAMIC_TICK_SET)
///
/// Callback function that should schedule next time to call 
/// `tn_tick_int_processing()`.
///
/// See `#TN_CBTickSchedule` for the prototype.
extern TN_CBTickSchedule      *_tn_cb_tick_schedule;
///
/// $(TN_IF_ONLY_DYNAMIC_TICK_SET)
///
/// Callback function that should return current system tick counter value.
///
/// See `#TN_CBTickCntGet` for the prototype.
extern TN_CBTickCntGet        *_tn_cb_tick_cnt_get;




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * $(TN_IF_ONLY_DYNAMIC_TICK_SET)
 *
 * Set callbacks related to dynamic tick. This is actually a worker function
 * for public function `tn_callback_dyn_tick_set()`, so, you might want 
 * to refer to it for comments.
 */
void _tn_timer_dyn_callback_set(
      TN_CBTickSchedule   *cb_tick_schedule,
      TN_CBTickCntGet     *cb_tick_cnt_get
      );




/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/


/**
 * Returns current value of system tick counter.
 */
_TN_STATIC_INLINE TN_TickCnt _tn_timer_sys_time_get(void)
{
   return _tn_cb_tick_cnt_get();
}



#ifdef __cplusplus
}  /* extern "C" */
#endif



#endif // TN_DYNAMIC_TICK

#endif // __TN_TIMER_DYN_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/




