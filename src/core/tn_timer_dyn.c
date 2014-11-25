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

//-- see comments in the file _tn_timer_dyn.h
struct TN_ListItem      _tn_timer_list__gen;

//-- see comments in the file _tn_timer_dyn.h
struct TN_ListItem      _tn_timer_list__fire;




volatile TN_SysTickCnt           _last_sys_tick_cnt;
volatile TN_Timeout              _last_min_timeout;

//TODO: get rid of it
int                              _tmp_min_timeout = 0;


/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/





/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

static TN_Timeout _handle_timers(void)
{
   //-- get current time (tick count)
   TN_SysTickCnt cur_sys_tick_cnt = _tn_timer_sys_time_get();

   //-- get how much time was passed since _last_sys_tick_cnt was updated
   //   last time
   TN_SysTickCnt diff = cur_sys_tick_cnt - _last_sys_tick_cnt;

   TN_Timeout min_timeout = TN_WAIT_INFINITE;

#if TN_DEBUG
   //-- interrupts should be disabled here
   if (!TN_IS_INT_DISABLED()){
      _TN_FATAL_ERROR("");
   }
#endif

   //-- walk through all active timers, reducing timeout by `diff`, 
   //   and moving expired timers to the list _tn_timer_list__fire.
   if (1/*diff > 0*/){
      struct TN_Timer *timer;
      struct TN_Timer *tmp_timer;

      _tn_list_for_each_entry_safe(
            timer, struct TN_Timer, tmp_timer,
            &_tn_timer_list__gen, timer_queue
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
            timer->timeout_cur -= diff;

            //-- remember min timeout
            if (timer->timeout_cur < min_timeout){
               min_timeout = timer->timeout_cur;
            }

         } else {
            //-- it's time to fire the timer, so, move it to the list
            //   _tn_timer_list__fire
            _tn_list_remove_entry(&(timer->timer_queue));
            _tn_list_add_tail(&_tn_timer_list__fire, &(timer->timer_queue));

            //-- set its timeout to zero
            timer->timeout_cur = 0;
         }

      }

      _last_min_timeout = min_timeout;
   } else {
      min_timeout = _last_min_timeout;
   }

   //-- update last system tick count
   _last_sys_tick_cnt = cur_sys_tick_cnt;

   return min_timeout;
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

void _tn_timer_dyn_callback_set(
      TN_CBTickSchedule   *cb_tick_schedule,
      TN_CBTickCntGet     *cb_tick_cnt_get
      )
{
   _tn_cb_tick_schedule = cb_tick_schedule;
   _tn_cb_tick_cnt_get  = cb_tick_cnt_get;
}


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

   //-- set last system tick count to zero
   _last_sys_tick_cnt = 0;

   _last_min_timeout = TN_WAIT_INFINITE;

   //-- reset "generic" timers list
   _tn_list_reset(&_tn_timer_list__gen);

   //-- reset "current" timers list
   _tn_list_reset(&_tn_timer_list__fire);
}


/**
 * See comments in the _tn_timer.h file.
 */
void _tn_timers_tick_proceed(void)
{
   _tmp_min_timeout = _handle_timers();

   //-- handle "fire" timer list
   {
      struct TN_Timer *timer;

      while (!_tn_list_is_empty(&_tn_timer_list__fire)){
         timer = _tn_list_first_entry(
               &_tn_timer_list__fire, struct TN_Timer, timer_queue
               );

         //-- first of all, cancel timer, so that 
         //   callback function could start it again if it wants to.
         _timer_cancel(timer);

         //-- call user callback function
         timer->func(timer, timer->p_user_data);
      }
   }

   //-- schedule next tick
   _tn_cb_tick_schedule(_tmp_min_timeout);
}

/**
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

      _tmp_min_timeout = _handle_timers();

      if (!_tn_list_is_empty(&_tn_timer_list__fire)){
         _tmp_min_timeout = 0;
      }

      //-- don't forget to check if timeout of new timer is the minimal one.
      if (timeout < _tmp_min_timeout){
         _tmp_min_timeout = timeout;
      }

      timer->timeout_cur = timeout;
      _tn_list_add_tail(&_tn_timer_list__gen, &(timer->timer_queue));

      //-- schedule next tick
      _tn_cb_tick_schedule(_tmp_min_timeout);
   }

   return rc;
}

/**
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

      //-- after timer is cancelled, walk through all remaining active timers,
      //   update their timeouts, and find new _tmp_min_timeout
      _tmp_min_timeout = _handle_timers();

      if (!_tn_list_is_empty(&_tn_timer_list__fire)){
         _tmp_min_timeout = 0;
      }

      //-- schedule next tick
      _tn_cb_tick_schedule(_tmp_min_timeout);
   }

   return rc;
}

/**
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



