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
/// by the value of `timeout_cur`
struct TN_ListItem      _timer_list__gen;


/// List of expired timers, it is typically used only inside
/// `_tn_timers_tick_proceed()` (it is filled and emptied there),
/// but if it isn't called in time for some reason, then this list
/// might be filled with timers when some new timer is added.
///
/// In this case, `_tn_cb_tick_schedule()` will be called with argument `0`,
/// which means that `tn_tick_int_processing()` should be called by application
/// as soon as possible.
struct TN_ListItem      _timer_list__fire;


///
/// The value returned by `_tn_timer_sys_time_get()`, relative to which
/// all the timeouts of timers in the `_timer_list__gen` list are specified
TN_SysTickCnt           _last_sys_tick_cnt;



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
 * Find out when the kernel needs `tn_tick_int_processing()` to be called next
 * time, and eventually call application callback `_tn_cb_tick_schedule()`
 * with found value.
 */
static void _next_tick_schedule(void)
{
   TN_Timeout next_timeout;

   if (!_tn_list_is_empty(&_timer_list__fire)){
      //-- need to execute tn_tick_int_processing() as soon as possible
      next_timeout = 0;
   } else if (!_tn_list_is_empty(&_timer_list__gen)){
      //-- need to get first timer from the list and get its timeout
      //   (this list is sorted, so, the first timer is the one with 
      //   minimum timeout value)
      struct TN_Timer *timer_next = _tn_get_timer_by_timer_queue(
            _timer_list__gen.next
            );

      next_timeout = timer_next->timeout_cur;
   } else {
      //-- no timers are active, so, no ticks needed at all
      next_timeout = TN_WAIT_INFINITE;
   }

   //-- schedule next tick
   _tn_cb_tick_schedule(next_timeout);
}


/**
 * Walk through all the active timers, updating their timeouts and probably
 * find the place where new timer should be added (depending on the timeout
 * value).
 *
 * For each timer in the list:
 *
 * - If it's time to fire timer right now, move that timer to the "fire" list;
 * - If it's not, just update its `timeout_cur` so that it represents
 *   timeout from now;
 * - Check if given `timeout_to_add` is more than timer's timeout;
 *   if so, it's probably the place where new timer should be added
 */
static struct TN_ListItem *_handle_timers(TN_Timeout timeout_to_add)
{
   //-- get current time (tick count)
   TN_SysTickCnt cur_sys_tick_cnt = _tn_timer_sys_time_get();

   //-- get how much time was passed since _last_sys_tick_cnt was updated
   //   last time
   TN_SysTickCnt diff = cur_sys_tick_cnt - _last_sys_tick_cnt;

   //-- update last system tick count
   _last_sys_tick_cnt = cur_sys_tick_cnt;


   struct TN_ListItem *ret_item = &_timer_list__gen;

#if TN_DEBUG
   //-- interrupts should be disabled here
   if (!TN_IS_INT_DISABLED()){
      _TN_FATAL_ERROR("");
   }
#endif

   //-- walk through all active timers, updating their timeouts to represent
   //   actual timeout fom now, and moving expired timers to the "fire" list
   //   _timer_list__fire.
   {
      struct TN_Timer *timer;
      struct TN_Timer *tmp_timer;

      _tn_list_for_each_entry_safe(
            timer, struct TN_Timer, tmp_timer,
            &_timer_list__gen, timer_queue
            )
      {
#if TN_DEBUG
         if (timer->timeout_cur == TN_WAIT_INFINITE){
            //-- should never be here: timeout value should never be
            //   TN_WAIT_INFINITE.
            _TN_FATAL_ERROR();
         }
#endif
         if (timer->timeout_cur > diff){
            //-- it's not time to fire the timer right now, so, just
            //   update timeout_cur so that it represents actual timeout
            //   from now (to be precise, from `_last_sys_tick_cnt`).

            timer->timeout_cur -= diff;

            //-- check if this is a place where new timer (with timeout 
            //   `timeout_to_add`) should be placed
            if (timer->timeout_cur < timeout_to_add){
               ret_item = &timer->timer_queue;
            }

         } else {
            //-- it's time to fire the timer, so, move it to the "fire" list
            //   _timer_list__fire
            _tn_list_remove_entry(&(timer->timer_queue));
            _tn_list_add_tail(&_timer_list__fire, &(timer->timer_queue));

            //-- set its timeout to zero
            timer->timeout_cur = 0;
         }

      }

   }

   return ret_item;
}

static void _timer_cancel(struct TN_Timer *timer)
{
   //-- reset timeout to zero (but this is actually not necessary)
   timer->timeout_cur = 0;

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

   //-- set last system tick count to zero
   _last_sys_tick_cnt = 0;

   //-- reset "generic" timers list
   _tn_list_reset(&_timer_list__gen);

   //-- reset "current" timers list
   _tn_list_reset(&_timer_list__fire);
}


/*
 * See comments in the _tn_timer.h file.
 */
void _tn_timers_tick_proceed(void)
{
   //-- walk through all active timers, moving expired ones to the
   //   "fire" list (`_timer_list__fire`).
   _handle_timers(0);

   //-- now, we have "fire" list containing expired timers. Let's walk
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
         timer->func(timer, timer->p_user_data);
      }
   }

   //-- find out when `tn_tick_int_processing()` should be called next time,
   //   and tell that to application
   _next_tick_schedule();
}

/*
 * See comments in the _tn_timer.h file.
 */
enum TN_RCode _tn_timer_start(struct TN_Timer *timer, TN_Timeout timeout)
{
   enum TN_RCode rc = TN_RC_OK;

#if TN_DEBUG
   //-- interrupts should be disabled here
   if (!TN_IS_INT_DISABLED()){
      _TN_FATAL_ERROR("");
   }
#endif

   if (timeout == TN_WAIT_INFINITE || timeout == 0){
      rc = TN_RC_WPARAM;
   } else {
      //-- cancel the timer
      _timer_cancel(timer);

      //-- walk through active timers list and get the position at which
      //   new timer should be placed. Also note that `timeout_cur` value
      //   of each timer is updated so that it represents timeout from now.
      struct TN_ListItem *list_item = _handle_timers(timeout);

      //-- place timer object at the right position.
      timer->timeout_cur = timeout;
      _tn_list_add_head(list_item, &(timer->timer_queue));

      //-- find out when `tn_tick_int_processing()` should be called next time,
      //   and tell that to application
      _next_tick_schedule();
   }

   return rc;
}

/*
 * See comments in the _tn_timer.h file.
 */
enum TN_RCode _tn_timer_cancel(struct TN_Timer *timer)
{
   enum TN_RCode rc = TN_RC_OK;

#if TN_DEBUG
   //-- interrupts should be disabled here
   if (!TN_IS_INT_DISABLED()){
      _TN_FATAL_ERROR("");
   }
#endif

   if (_tn_timer_is_active(timer)){

      //-- cancel the timer
      _timer_cancel(timer);

      //-- Note: we don't need to call _handle_timers(0) here, because
      //   nothing will break if we just remove one timer from the list.
      //   We don't need to recalculate all timeouts.
      //   But if we've just removed timer that was the next one,
      //   then we need to find out new next timer. For that,
      //   we call _next_tick_schedule().
      //
      //   But DO NOTE that even if we don't call _next_tick_schedule(),
      //   nothing will break, anyway! In that case, we might just have one
      //   useless call to `tn_tick_int_processing()`, but inside that call,
      //   we will found out that there's nothing to do, and that's it.
      //
      //   Probably it's a good idea to avoid useless interrupt, so, we call
      //   `_next_tick_schedule()` here.

      //-- find out when `tn_tick_int_processing()` should be called next time,
      //   and tell that to application
      _next_tick_schedule();
   }

   return rc;
}

/*
 * See comments in the _tn_timer.h file.
 */
TN_Timeout _tn_timer_time_left(struct TN_Timer *timer)
{
   TN_Timeout time_left = TN_WAIT_INFINITE;

#if TN_DEBUG
   //-- interrupts should be disabled here
   if (!TN_IS_INT_DISABLED()){
      _TN_FATAL_ERROR("");
   }
#endif

   if (_tn_timer_is_active(timer)){
      //-- get current time (tick count)
      TN_SysTickCnt cur_sys_tick_cnt = _tn_timer_sys_time_get();

      //-- get how much time was passed since _last_sys_tick_cnt was updated
      //   last time
      TN_SysTickCnt diff = cur_sys_tick_cnt - _last_sys_tick_cnt;

      time_left = timer->timeout_cur - diff;
   }

   return time_left;
}


#endif // !TN_DYNAMIC_TICK



