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
#include "tn_internal.h"

//-- header of current module
#include "tn_timer.h"

//-- header of other needed modules
#include "tn_tasks.h"





/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * @param timeout    should be < TN_TIMERS_QUE_CNT
 */
#define _DEDICATED_LIST_INDEX(timeout)    \
   ((tn_sys_time_count + timeout) & TN_TIMERS_QUE_MASK)




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
static inline enum TN_RCode _check_param_generic(
      struct TN_Timer *timer
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (timer == NULL){
      rc = TN_RC_WPARAM;
   } else if (timer->id_timer != TN_ID_TIMER){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

static inline enum TN_RCode _check_param_create(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (timer == NULL){
      rc = TN_RC_WPARAM;
   } else if (0
         || timer->id_timer == TN_ID_TIMER
         || func == NULL
         )
   {
      rc = TN_RC_WPARAM;
   }

   return rc;
}

#else
#  define _check_param_generic(timer)           (TN_RC_OK)
#  define _check_param_create(timer, func)      (TN_RC_OK)
#endif
// }}}



/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_create(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      )
{
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_create(timer, func);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   rc = _tn_timer_create(timer, func, p_user_data);

out:
   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_delete(struct TN_Timer *timer)
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(timer);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   //-- if timer is active, cancel it first
   if (_tn_timer_is_active(timer)){
      rc = _tn_timer_cancel(timer);
   }

   //-- now, delete timer
   timer->id_timer = 0;

   TN_INT_RESTORE();

out:
   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_start(struct TN_Timer *timer, TN_Timeout timeout)
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(timer);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   rc = _tn_timer_start(timer, timeout);

   TN_INT_RESTORE();

out:
   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_istart(struct TN_Timer *timer, TN_Timeout timeout)
{
   TN_INTSAVE_DATA_INT;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(timer);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_IDIS_SAVE();

   rc = _tn_timer_start(timer, timeout);

   TN_INT_IRESTORE();

out:
   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_cancel(struct TN_Timer *timer)
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(timer);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   if (_tn_timer_is_active(timer)){
      rc = _tn_timer_cancel(timer);
   }

   TN_INT_RESTORE();

out:
   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_icancel(struct TN_Timer *timer)
{
   TN_INTSAVE_DATA_INT;
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(timer);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_IDIS_SAVE();

   if (_tn_timer_is_active(timer)){
      rc = _tn_timer_cancel(timer);
   }

   TN_INT_IRESTORE();

out:
   return rc;
}




/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

#if 0
/**
 * See comments in the tn_internal.h file
 */
void _tn_timer_tick_proceed(struct TN_Timer *timer)
{
   if (timer->timeout_cur == TN_WAIT_INFINITE || timer->timeout_cur == 0){
      //-- should never be here
      _TN_FATAL_ERROR();
   }

   timer->timeout_cur--;

   if (timer->timeout_cur == 0){
      //-- it's time to fire.
      //-- first of all, cancel timer, so that 
      //   function could start it again if it wants to.
      _tn_timer_cancel(timer);
      
      //-- call user function
      timer->func(timer, timer->p_user_data);
   }
}
#endif

/**
 * See comments in the tn_internal.h file
 */
void _tn_timers_tick_proceed(void)
{
   struct TN_Timer *timer;
   struct TN_Timer *tmp_timer;

   int dedicated_list_index = _DEDICATED_LIST_INDEX(0);

   if (dedicated_list_index == 0){
      //-- handle generic timer list {{{
      {
         tn_list_for_each_entry_safe(
               timer, tmp_timer, &tn_timer_list__gen, timer_queue
               )
         {
#if TN_DEBUG
            if (timer->timeout_cur == TN_WAIT_INFINITE || timer->timeout_cur < TN_TIMERS_QUE_CNT){
               //-- should never be here
               _TN_FATAL_ERROR();
            }
#endif
            timer->timeout_cur -= TN_TIMERS_QUE_CNT;

            if (timer->timeout_cur < TN_TIMERS_QUE_CNT){
               //-- it's time to move this timer to the dedicated list
               tn_list_remove_entry(&(timer->timer_queue));
               tn_list_add_tail(
                     &tn_timer_list__dedicated[ _DEDICATED_LIST_INDEX(timer->timeout_cur) ],
                     &(timer->timer_queue)
                     );
            }
         }
      }
      //}}}
   }

   //-- handle current dedicated timer list {{{
   {
      struct TN_ListItem *p_cur_timer_list = 
         &tn_timer_list__dedicated[ dedicated_list_index ];

      {
         tn_list_for_each_entry_safe(
               timer, tmp_timer, p_cur_timer_list, timer_queue
               )
         {
            //-- first of all, cancel timer, so that 
            //   function could start it again if it wants to.
            _tn_timer_cancel(timer);

            //-- call user function
            timer->func(timer, timer->p_user_data);
         }
      }

#if TN_DEBUG
      //-- current dedicated list should be empty now
      if (!tn_is_list_empty(p_cur_timer_list)){
         _TN_FATAL_ERROR("");
      }
#endif
   }
   // }}}
}

enum TN_RCode _tn_timer_start(struct TN_Timer *timer, TN_Timeout timeout)
{
   enum TN_RCode rc = TN_RC_OK;

   if (timeout == TN_WAIT_INFINITE || timeout == 0){
      rc = TN_RC_WPARAM;
   } else {
      timer->timeout_cur = timeout + _DEDICATED_LIST_INDEX(0);
      //tn_list_add_tail(&tn_timer_list, &(timer->timer_queue));

      if (timeout < TN_TIMERS_QUE_CNT){
         tn_list_add_tail(
               &tn_timer_list__dedicated[ _DEDICATED_LIST_INDEX(timeout) ],
               &(timer->timer_queue)
               );
      } else {
         tn_list_add_tail(&tn_timer_list__gen, &(timer->timer_queue));
      }
   }

   return rc;
}

enum TN_RCode _tn_timer_cancel(struct TN_Timer *timer)
{
   enum TN_RCode rc = TN_RC_OK;

   //-- reset timeout to zero
   timer->timeout_cur = 0;

   //-- remove entry from timer queue
   tn_list_remove_entry(&(timer->timer_queue));

   //-- reset the list (but this is actually not necessary)
   tn_list_reset(&(timer->timer_queue));

   return rc;
}

enum TN_RCode _tn_timer_create(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      )
{
   enum TN_RCode rc = TN_RC_OK;

   tn_list_reset(&(timer->timer_queue));

   timer->func          = func;
   timer->p_user_data   = p_user_data;
   timer->timeout_cur   = 0;

   timer->id_timer      = TN_ID_TIMER;

   return rc;
}

BOOL _tn_timer_is_active(struct TN_Timer *timer)
{
   return (timer->timeout_cur != 0);
}



