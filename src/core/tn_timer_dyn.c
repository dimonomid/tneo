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


#if TN_DYNAMIC_TICK




/*******************************************************************************
 *    PROTECTED DATA
 ******************************************************************************/

//-- see comments in the file _tn_timer_dyn.h
TN_CBTickSchedule      *_tn_cb_tick_schedule = TN_NULL;

//-- see comments in the file _tn_timer_dyn.h
TN_CBTickCntGet        *_tn_cb_tick_cnt_get  = TN_NULL;




/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

///
/// List of active non-expired timers. Timers are sorted in ascending order
/// by the timeout value.
static struct TN_ListItem     _timer_list__gen;


/// List of expired timers; after it is initialized, it is used only inside
/// `_tn_timers_tick_proceed()`
static struct TN_ListItem     _timer_list__fire;




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Get pointer to `struct #TN_Task` by pointer to the `task_queue` member
 * of the `struct #TN_Task`.
 */
#define _tn_get_timer_by_timer_queue(que)                               \
   (que ? container_of(que, struct TN_Timer, timer_queue) : 0)





/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Get expiration time left. If timer is expired, 0 is returned, no matter
 * how much it is expired.
 */
static TN_TickCnt _time_left_get(
      struct TN_Timer *timer,
      TN_TickCnt cur_sys_tick_cnt
      )
{
   TN_TickCnt time_left;

   //-- Since return value type `TN_TickCnt` is unsigned, we should check if 
   //   it is going to be negative. If it is, then return 0.
   TN_TickCnt time_elapsed = cur_sys_tick_cnt - timer->start_tick_cnt;

   if (time_elapsed <= timer->timeout){
      time_left = timer->timeout - time_elapsed;
   } else {
      time_left = 0;
   }

   return time_left;
}

/**
 * Find out when the kernel needs `tn_tick_int_processing()` to be called next
 * time, and eventually call application callback `_tn_cb_tick_schedule()` with
 * found value.
 */
static void _next_tick_schedule(TN_TickCnt cur_sys_tick_cnt)
{
   TN_TickCnt next_timeout;

   if (!_tn_list_is_empty(&_timer_list__gen)){
      //-- need to get first timer from the list and get its timeout
      //   (this list is sorted, so, the first timer is the one with 
      //   minimum timeout value)
      struct TN_Timer *timer_next = _tn_get_timer_by_timer_queue(
            _timer_list__gen.next
            );

      next_timeout = _time_left_get(timer_next, cur_sys_tick_cnt);
   } else {
      //-- no timers are active, so, no ticks needed at all
      next_timeout = TN_WAIT_INFINITE;
   }

   //-- schedule next tick
   _tn_cb_tick_schedule(next_timeout);
}


/**
 * Cancel the timer: the main thing is that timer is removed from the linked
 * list.
 */
static void _timer_cancel(struct TN_Timer *timer)
{
   //-- reset timeout and start_tick_cnt to zero (but this is actually not
   //   necessary)
   timer->timeout = 0;
   timer->start_tick_cnt = 0;

   //-- remove entry from timer queue
   _tn_list_remove_entry(&(timer->timer_queue));

   //-- reset the list
   _tn_list_reset(&(timer->timer_queue));
}



/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/



/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the _tn_timer.h file.
 */
void _tn_timer_dyn_callback_set(
      TN_CBTickSchedule   *cb_tick_schedule,
      TN_CBTickCntGet     *cb_tick_cnt_get
      )
{
   _tn_cb_tick_schedule = cb_tick_schedule;
   _tn_cb_tick_cnt_get  = cb_tick_cnt_get;
}


/*
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

   //-- reset "generic" timers list
   _tn_list_reset(&_timer_list__gen);

   //-- reset "current" timers list
   _tn_list_reset(&_timer_list__fire);
}


/*
 * See comments in the _tn_timer.h file.
 */
void _tn_timers_tick_proceed(TN_UWord TN_INTSAVE_VAR)
{
   //-- Since user is allowed to manage timers from timer callbacks, we can't
   //   just iterate through timers list and fire expired timers, because
   //   timers list could change under our feet then.
   //
   //   Instead, we should first move all expired timers to the dedicated
   //   "fire" list, and then, as a second step, fire all the timers from this
   //   list.

   //-- First of all, get current time
   TN_TickCnt cur_sys_tick_cnt = _tn_timer_sys_time_get();

   //-- Now, walk through timers list from start until we get non-expired timer
   //   (timers list is sorted)
   {
      struct TN_Timer *timer;
      struct TN_Timer *tmp_timer;

      _tn_list_for_each_entry_safe(
            timer, struct TN_Timer, tmp_timer,
            &_timer_list__gen, timer_queue
            )
      {
         //-- timeout value should never be TN_WAIT_INFINITE.
         _TN_BUG_ON(timer->timeout == TN_WAIT_INFINITE);

         if (_time_left_get(timer, cur_sys_tick_cnt) == 0){
            //-- it's time to fire the timer, so, move it to the "fire" list
            //   `_timer_list__fire`
            _tn_list_remove_entry(&(timer->timer_queue));
            _tn_list_add_tail(&_timer_list__fire, &(timer->timer_queue));
         } else {
            //-- We've got non-expired timer, therefore there are no more
            //   expired timers.
            break;
         }
      }
   }

   //-- Now, we have "fire" list containing expired timers. Let's walk
   //   through them, firing each one.
   {
      struct TN_Timer *timer;

      while (!_tn_list_is_empty(&_timer_list__fire)){
         timer = _tn_list_first_entry(
               &_timer_list__fire, struct TN_Timer, timer_queue
               );

         //-- first of all, cancel timer *before* calling callback function, so
         //   that function could start it again if it wants to.
         _timer_cancel(timer);

         //-- call user callback function
         _tn_timer_callback_call(timer, TN_INTSAVE_VAR);
      }
   }

   //-- Find out when `tn_tick_int_processing()` should be called next time,
   //   and tell that to application
   _next_tick_schedule(cur_sys_tick_cnt);
}

/*
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
      //-- cancel the timer
      _timer_cancel(timer);

      //-- walk through active timers list and get the position at which
      //   new timer should be placed.

      //-- First of all, get current time
      TN_TickCnt cur_sys_tick_cnt = _tn_timer_sys_time_get();

      //-- Since timers list is sorted, we need to find the correct place
      //   to put new timer at.
      //
      //   Initially, we set it to the head of the list, and then walk
      //   through timers until we found needed place (or until list is over)
      struct TN_ListItem *list_item = &_timer_list__gen;
      {
         struct TN_Timer *timer;
         struct TN_Timer *tmp_timer;

         _tn_list_for_each_entry_safe(
               timer, struct TN_Timer, tmp_timer,
               &_timer_list__gen, timer_queue
               )
         {
            //-- timeout value should never be TN_WAIT_INFINITE.
            _TN_BUG_ON(timer->timeout == TN_WAIT_INFINITE);

            if (_time_left_get(timer, cur_sys_tick_cnt) < timeout){
               //-- Probably this is the place for new timer..
               list_item = &timer->timer_queue;
            } else {
               //-- Found timer with larger timeout than that of new timer.
               //   So, list_item now contains the correct place for new timer.
               break;
            }
         }
      }

      //-- put timer object at the right position.
      _tn_list_add_head(list_item, &(timer->timer_queue));

      //-- initialize timer with given timeout
      timer->timeout = timeout;
      timer->start_tick_cnt = cur_sys_tick_cnt;

      //-- find out when `tn_tick_int_processing()` should be called next time,
      //   and tell that to application
      _next_tick_schedule(cur_sys_tick_cnt);
   }

   return rc;
}

/*
 * See comments in the _tn_timer.h file.
 */
enum TN_RCode _tn_timer_cancel(struct TN_Timer *timer)
{
   enum TN_RCode rc = TN_RC_OK;

   //-- interrupts should be disabled here
   _TN_BUG_ON( !TN_IS_INT_DISABLED() );

   if (_tn_timer_is_active(timer)){

      //-- cancel the timer
      _timer_cancel(timer);

      //-- find out when `tn_tick_int_processing()` should be called next time,
      //   and tell that to application
      _next_tick_schedule( _tn_timer_sys_time_get() );
   }

   return rc;
}

/*
 * See comments in the _tn_timer.h file.
 */
TN_TickCnt _tn_timer_time_left(struct TN_Timer *timer)
{
   TN_TickCnt time_left = TN_WAIT_INFINITE;

   //-- interrupts should be disabled here
   _TN_BUG_ON( !TN_IS_INT_DISABLED() );

   if (_tn_timer_is_active(timer)){
      //-- get current time (tick count)
      time_left = _time_left_get(timer, _tn_timer_sys_time_get());
   }

   return time_left;
}


#endif // !TN_DYNAMIC_TICK



