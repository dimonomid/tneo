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

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- common tnkernel headers
#include "tn_common.h"
#include "tn_sys.h"

//-- internal tnkernel headers
#include "_tn_timer.h"
#include "_tn_list.h"


//-- header of current module
#include "tn_timer.h"

//-- header of other needed modules
#include "tn_tasks.h"


#if TN_DYNAMIC_TICK




/*******************************************************************************
 *    PUBLIC DATA
 ******************************************************************************/

//-- see comments in the file _tn_timer_dyn.h
TN_CBTickSchedule      *_tn_cb_tick_schedule = TN_NULL;

//-- see comments in the file _tn_timer_dyn.h
TN_CBTickCntGet        *_tn_cb_tick_cnt_get  = TN_NULL;




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/





/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/


/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/



/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

/**
 * See comments in the _tn_timer.h file.
 */
void _tn_timers_init(void)
{
   //-- when dynamic tick is used, check that we have callbacks set.
   //   (they should be set by tn_callback_dyn_tick_set() before calling
   //   tn_sys_start())
   if (_tn_cb_tick_schedule == TN_NULL || _tn_cb_tick_cnt_get == TN_NULL){
      _TN_FATAL_ERROR("");
   }
}

void _tn_timer_dyn_callback_set(
      TN_CBTickSchedule   *cb_tick_schedule,
      TN_CBTickCntGet     *cb_tick_cnt_get
      )
{
   _tn_cb_tick_schedule = cb_tick_schedule;
   _tn_cb_tick_cnt_get  = cb_tick_cnt_get;
}



#endif // !TN_DYNAMIC_TICK



