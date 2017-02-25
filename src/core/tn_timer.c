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




/*******************************************************************************
 *    PROTECTED DATA
 ******************************************************************************/



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
_TN_STATIC_INLINE enum TN_RCode _check_param_generic(
      const struct TN_Timer *timer
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (timer == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (!_tn_timer_is_valid(timer)){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _check_param_create(
      const struct TN_Timer  *timer,
      TN_TimerFunc     *func
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (timer == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (_tn_timer_is_valid(timer)){
      rc = TN_RC_WPARAM;
   }

   _TN_UNUSED(func);

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
   enum TN_RCode rc = _check_param_create(timer, func);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      rc = _tn_timer_create(timer, func, p_user_data);
   }

   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_delete(struct TN_Timer *timer)
{
   int sr_saved;
   enum TN_RCode rc = _check_param_generic(timer);

   if (rc == TN_RC_OK){
      sr_saved = tn_arch_sr_save_int_dis();
      //-- if timer is active, cancel it first
      rc = _tn_timer_cancel(timer);

      //-- now, delete timer
      timer->id_timer = TN_ID_NONE;
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_start(struct TN_Timer *timer, TN_TickCnt timeout)
{
   int sr_saved;
   enum TN_RCode rc = _check_param_generic(timer);

   if (rc == TN_RC_OK){
      sr_saved = tn_arch_sr_save_int_dis();
      rc = _tn_timer_start(timer, timeout);
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_cancel(struct TN_Timer *timer)
{
   int sr_saved;
   enum TN_RCode rc = _check_param_generic(timer);

   if (rc == TN_RC_OK){
      sr_saved = tn_arch_sr_save_int_dis();
      rc = _tn_timer_cancel(timer);
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_set_func(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      )
{
   int sr_saved;
   enum TN_RCode rc = _check_param_generic(timer);

   if (rc == TN_RC_OK){
      sr_saved = tn_arch_sr_save_int_dis();
      rc = _tn_timer_set_func(timer, func, p_user_data);
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_is_active(struct TN_Timer *timer, TN_BOOL *p_is_active)
{
   int sr_saved;
   enum TN_RCode rc = _check_param_generic(timer);

   if (rc == TN_RC_OK){
      sr_saved = tn_arch_sr_save_int_dis();
      *p_is_active = _tn_timer_is_active(timer);
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (tn_timer.h)
 */
enum TN_RCode tn_timer_time_left(
      struct TN_Timer *timer,
      TN_TickCnt *p_time_left
      )
{
   int sr_saved;
   enum TN_RCode rc = _check_param_generic(timer);

   if (rc == TN_RC_OK){
      sr_saved = tn_arch_sr_save_int_dis();
      *p_time_left = _tn_timer_time_left(timer);
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}





/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/


/**
 * See comments in the _tn_timer.h file.
 */
enum TN_RCode _tn_timer_create(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      )
{
   enum TN_RCode rc = _tn_timer_set_func(timer, func, p_user_data);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {

      _tn_list_reset(&(timer->timer_queue));

#if TN_DYNAMIC_TICK
      timer->timeout = 0;
      timer->start_tick_cnt = 0;
#else
      timer->timeout_cur   = 0;
#endif
      timer->id_timer      = TN_ID_TIMER;

   }
   return rc;
}

/**
 * See comments in the _tn_timer.h file.
 */
enum TN_RCode _tn_timer_set_func(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (func == TN_NULL){
      rc = TN_RC_WPARAM;
   } else {
      timer->func          = func;
      timer->p_user_data   = p_user_data;
   }

   return rc;
}

/**
 * See comments in the _tn_timer.h file.
 */
TN_BOOL _tn_timer_is_active(struct TN_Timer *timer)
{
   //-- interrupts should be disabled here
   _TN_BUG_ON( !TN_IS_INT_DISABLED() );

   return (!_tn_list_is_empty(&(timer->timer_queue)));
}


