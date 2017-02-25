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


#if !TN_DYNAMIC_TICK




/*******************************************************************************
 *    PROTECTED DATA
 ******************************************************************************/

//-- see comments in the file _tn_timer_static.h
struct TN_ListItem _tn_timer_list__gen;

//-- see comments in the file _tn_timer_static.h
struct TN_ListItem _tn_timer_list__tick[ TN_TICK_LISTS_CNT ];

//-- see comments in the file _tn_timer_static.h
volatile TN_TickCnt _tn_sys_time_count;




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define TN_TICK_LISTS_MASK    (TN_TICK_LISTS_CNT - 1)

//-- configuration check
#if ((TN_TICK_LISTS_MASK & TN_TICK_LISTS_CNT) != 0)
#  error TN_TICK_LISTS_CNT must be a power of two
#endif

#if (TN_TICK_LISTS_CNT < 2)
#  error TN_TICK_LISTS_CNT must be >= 2
#endif

//-- The limitation of 256 is actually because struct _TN_BuildCfg has
//   just 8-bit field `tick_lists_cnt_minus_one` for this value.
//   This should never be the problem, because for embedded systems
//   it makes little sense to use even more than 64 lists, as it takes
//   significant amount of RAM.
#if (TN_TICK_LISTS_CNT > 256)
#  error TN_TICK_LISTS_CNT must be <= 256
#endif



/**
 * Return index in the array `#_tn_timer_list__tick`, based on given timeout.
 *
 * @param timeout    should be < TN_TICK_LISTS_CNT
 */
#define _TICK_LIST_INDEX(timeout)    \
   (((TN_TickCnt)_tn_sys_time_count + timeout) & TN_TICK_LISTS_MASK)




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
   int i;

   //-- reset system time
   _tn_sys_time_count = 0;

   //-- reset "generic" timers list
   _tn_list_reset(&_tn_timer_list__gen);

   //-- reset all "tick" timer lists
   for (i = 0; i < TN_TICK_LISTS_CNT; i++){
      _tn_list_reset(&_tn_timer_list__tick[i]);
   }
}

/**
 * See comments in the _tn_timer.h file.
 */
void _tn_timers_tick_proceed(TN_UWord TN_INTSAVE_VAR)
{
   //-- first of all, increment system timer
   _tn_sys_time_count++;

   int tick_list_index = _TICK_LIST_INDEX(0);

   //-- interrupts should be disabled here
   _TN_BUG_ON( !TN_IS_INT_DISABLED() );

   if (tick_list_index == 0){
      //-- it happens each TN_TICK_LISTS_CNT-th system tick:
      //   now we should walk through all the timers in the "generic" timer
      //   list, decrement the timeout of each one by TN_TICK_LISTS_CNT,
      //   and if resulting timeout is less than TN_TICK_LISTS_CNT,
      //   then move that timer to the appropriate "tick" timer list.

      //-- handle "generic" timer list {{{
      {
         struct TN_Timer *timer;
         struct TN_Timer *tmp_timer;

         _tn_list_for_each_entry_safe(
               timer, struct TN_Timer, tmp_timer,
               &_tn_timer_list__gen, timer_queue
               )
         {

            //-- timeout value should always be >= TN_TICK_LISTS_CNT here.
            //   And it should never be TN_WAIT_INFINITE.
            _TN_BUG_ON(0
                  || timer->timeout_cur == TN_WAIT_INFINITE 
                  || timer->timeout_cur < TN_TICK_LISTS_CNT
                  );

            timer->timeout_cur -= TN_TICK_LISTS_CNT;

            if (timer->timeout_cur < TN_TICK_LISTS_CNT){
               //-- it's time to move this timer to the "tick" list
               _tn_list_remove_entry(&(timer->timer_queue));
               _tn_list_add_tail(
                     &_tn_timer_list__tick[_TICK_LIST_INDEX(timer->timeout_cur)],
                     &(timer->timer_queue)
                     );
            }
         }
      }
      //}}}
   }

   //-- it happens every system tick:
   //   we should walk through all the timers in the current "tick" timer list,
   //   and fire them all, unconditionally.

   //-- handle current "tick" timer list {{{
   {
      struct TN_Timer *timer;

      struct TN_ListItem *p_cur_timer_list = 
         &_tn_timer_list__tick[ tick_list_index ];

      //-- now, p_cur_timer_list is a list of timers that we should
      //   fire NOW, unconditionally.

      //-- NOTE that we shouldn't use iterators like 
      //   `_tn_list_for_each_entry_safe()` here, because timers can be 
      //   removed from the list while we are iterating through it: 
      //   this may happen if user-provided function cancels timer which 
      //   is in the same list.
      //
      //   Although timers could be removed from the list, note that
      //   new timer can't be added to it
      //   (because timeout 0 is disallowed, and timer with timeout
      //   TN_TICK_LISTS_CNT is added to the "generic" list),
      //   see implementation details in the tn_timer.h file
      while (!_tn_list_is_empty(p_cur_timer_list)){
         timer = _tn_list_first_entry(
               p_cur_timer_list, struct TN_Timer, timer_queue
               );

         //-- first of all, cancel timer, so that 
         //   callback function could start it again if it wants to.
         _tn_timer_cancel(timer);

         //-- call user callback function
         _tn_timer_callback_call(timer, TN_INTSAVE_VAR);
      }

      _TN_BUG_ON( !_tn_list_is_empty(p_cur_timer_list) );
   }
   // }}}
}

/**
 * See comments in the _tn_timer.h file.
 */
enum TN_RCode _tn_timer_start(struct TN_Timer *timer, TN_TickCnt timeout)
{
   enum TN_RCode rc = TN_RC_OK;

   //-- interrupts should be disabled here
   _TN_BUG_ON( !TN_IS_INT_DISABLED() );

   if (timeout == TN_WAIT_INFINITE || timeout == 0){
      rc = TN_RC_WPARAM;
   } else {

      //-- if timer is active, cancel it first
      if ((rc = _tn_timer_cancel(timer)) == TN_RC_OK){

         if (timeout < TN_TICK_LISTS_CNT){
            //-- timer should be added to the one of "tick" lists.
            int tick_list_index = _TICK_LIST_INDEX(timeout);
            timer->timeout_cur = tick_list_index;

            _tn_list_add_tail(
                  &_tn_timer_list__tick[ tick_list_index ],
                  &(timer->timer_queue)
                  );
         } else {
            //-- timer should be added to the "generic" list.
            //   We should set timeout_cur adding current "tick" index to it.
            timer->timeout_cur = timeout + _TICK_LIST_INDEX(0);

            _tn_list_add_tail(&_tn_timer_list__gen, &(timer->timer_queue));
         }
      }
   }

   return rc;
}

/**
 * See comments in the _tn_timer.h file.
 */
enum TN_RCode _tn_timer_cancel(struct TN_Timer *timer)
{
   enum TN_RCode rc = TN_RC_OK;

   //-- interrupts should be disabled here
   _TN_BUG_ON( !TN_IS_INT_DISABLED() );

   if (_tn_timer_is_active(timer)){
      //-- reset timeout to zero (but this is actually not necessary)
      timer->timeout_cur = 0;

      //-- remove entry from timer queue
      _tn_list_remove_entry(&(timer->timer_queue));

      //-- reset the list
      _tn_list_reset(&(timer->timer_queue));
   }

   return rc;
}

/**
 * See comments in the _tn_timer.h file.
 */
TN_TickCnt _tn_timer_time_left(struct TN_Timer *timer)
{
   TN_TickCnt time_left = TN_WAIT_INFINITE;

   //-- interrupts should be disabled here
   _TN_BUG_ON( !TN_IS_INT_DISABLED() );

   if (_tn_timer_is_active(timer)){
      TN_TickCnt tick_list_index = _TICK_LIST_INDEX(0);

      if (timer->timeout_cur > tick_list_index){
         time_left = timer->timeout_cur - tick_list_index;
      } else if (timer->timeout_cur < tick_list_index){
         time_left = timer->timeout_cur + TN_TICK_LISTS_CNT - tick_list_index;
      } else {
#if TN_DEBUG
         //-- timer->timeout_cur should never be equal to tick_list_index here
         //   (if it is, timer is inactive, so, we don't get here)
         _TN_FATAL_ERROR();
#endif
      }
   }

   return time_left;
}


#endif // !TN_DYNAMIC_TICK


