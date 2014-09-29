/*******************************************************************************
 *
 * TNeoKernel: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeoKernel:                copyright © 2014       Dmitry Frank.
 *
 *    TNeoKernel was born as a thorough review and re-implementation 
 *    of TNKernel. New kernel has well-formed code, bugs of ancestor are fixed
 *    as well as new features added, and it is tested carefully with unit-tests.
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
#include "tn_eventgrp.h"

//-- header of other needed modules
#include "tn_tasks.h"



/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
static inline enum TN_RCode _check_param_generic(
      struct TN_EventGrp  *eventgrp
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (eventgrp == NULL){
      rc = TN_RC_WPARAM;
   } else if (eventgrp->id_event != TN_ID_EVENTGRP){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

static inline enum TN_RCode _check_param_job_perform(
      struct TN_EventGrp  *eventgrp,
      unsigned int         pattern
      )
{
   enum TN_RCode rc = _check_param_generic(eventgrp);

   if (pattern == 0){
      rc = TN_RC_WPARAM;
   }

   return rc;
}

static inline enum TN_RCode _check_param_create(
      struct TN_EventGrp  *eventgrp
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (eventgrp == NULL || eventgrp->id_event == TN_ID_EVENTGRP){
      rc = TN_RC_WPARAM;
   }

   return rc;
}

#else
#  define _check_param_generic(event)                 (TN_RC_OK)
#  define _check_param_job_perform(event, pattern)    (TN_RC_OK)
#  define _check_param_create(event)                  (TN_RC_OK)
#endif
// }}}


static BOOL _cond_check(
      struct TN_EventGrp *eventgrp,
      enum TN_EGrpWaitMode wait_mode,
      int wait_pattern
      )
{
   BOOL cond = FALSE;

   switch (wait_mode){
      case TN_EVENTGRP_WMODE_OR:
         //-- any bit set is enough for release condition
         cond = ((eventgrp->pattern & wait_pattern) != 0);
         break;
      case TN_EVENTGRP_WMODE_AND:
         //-- all bits should be set for release condition
         cond = ((eventgrp->pattern & wait_pattern) == wait_pattern);
         break;
#if TN_DEBUG
      default:
         _TN_FATAL_ERROR("");
         break;
#endif
   }

   return cond;
}

static void _clear_pattern_if_needed(
      struct TN_EventGrp     *eventgrp,
      enum TN_EGrpWaitMode    wait_mode,
      unsigned int            pattern
      )
{
   //-- probably, we should add one more wait mode flag, like 
   //   TN_EVENTGRP_WMODE_CLR, and if it is set, then clear pattern here
}

static void _scan_event_waitqueue(struct TN_EventGrp *eventgrp)
{
   struct TN_Task *task;
   struct TN_Task *tmp_task;

   // checking ALL of the tasks waiting on the event.

   tn_list_for_each_entry_safe(
         task, tmp_task, &(eventgrp->wait_queue), task_queue
         )
   {

      if ( _cond_check(
               eventgrp,
               task->subsys_wait.eventgrp.wait_mode,
               task->subsys_wait.eventgrp.wait_pattern
               )
         )
      {
         //-- Condition to finish the waiting
         task->subsys_wait.eventgrp.actual_pattern = eventgrp->pattern;
         _tn_task_wait_complete(task, TN_RC_OK);

         _clear_pattern_if_needed(
               eventgrp,
               task->subsys_wait.eventgrp.wait_mode,
               task->subsys_wait.eventgrp.wait_pattern
               );
      }
   }
}


static enum TN_RCode _eventgrp_wait(
      struct TN_EventGrp  *eventgrp,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_job_perform(eventgrp, wait_pattern);
   if (rc != TN_RC_OK){
      goto out;
   }

   //-- Check release condition

   if (_cond_check(eventgrp, wait_mode, wait_pattern)){
      if (p_flags_pattern != NULL){
         *p_flags_pattern = eventgrp->pattern;
      }
      _clear_pattern_if_needed(eventgrp, wait_mode, wait_pattern);
      rc = TN_RC_OK;
   } else {
      rc = TN_RC_TIMEOUT;
   }

out:
   return rc;
}

static enum TN_RCode _eventgrp_modify(
      struct TN_EventGrp  *eventgrp,
      enum TN_EGrpOp       operation,
      unsigned int         pattern
      )
{
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_job_perform(eventgrp, pattern);
   if (rc != TN_RC_OK){
      goto out;
   }

   switch (operation){
      case TN_EVENTGRP_OP_CLEAR:
         eventgrp->pattern &= ~pattern;
         break;

      case TN_EVENTGRP_OP_SET:
         eventgrp->pattern |= pattern;
         _scan_event_waitqueue(eventgrp);
         break;

      case TN_EVENTGRP_OP_TOGGLE:
         eventgrp->pattern ^= pattern;
         _scan_event_waitqueue(eventgrp);
         break;
   }

out:
   return rc;
}





/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/


/*
 * See comments in the header file (tn_eventgrp.h)
 */
enum TN_RCode tn_eventgrp_create(
      struct TN_EventGrp *eventgrp,
      unsigned int initial_pattern //-- initial value of the pattern
      )  
{
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_create(eventgrp);
   if (rc != TN_RC_OK){
      goto out;
   }

   tn_list_reset(&(eventgrp->wait_queue));

   eventgrp->pattern    = initial_pattern;
   eventgrp->id_event   = TN_ID_EVENTGRP;

out:
   return rc;
}


/*
 * See comments in the header file (tn_eventgrp.h)
 */
enum TN_RCode tn_eventgrp_delete(struct TN_EventGrp *eventgrp)
{
   enum TN_RCode rc = TN_RC_OK;
   TN_INTSAVE_DATA;

   rc = _check_param_generic(eventgrp);
   if (rc != TN_RC_OK){
      goto out;
   }

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   _tn_wait_queue_notify_deleted(&(eventgrp->wait_queue));

   eventgrp->id_event = 0; //-- event does not exist now

   TN_INT_RESTORE();

   //-- we might need to switch context if _tn_wait_queue_notify_deleted()
   //   has woken up some high-priority task
   _tn_switch_context_if_needed();

out:
   return rc;
}


/*
 * See comments in the header file (tn_eventgrp.h)
 */
enum TN_RCode tn_eventgrp_wait(
      struct TN_EventGrp  *eventgrp,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern,
      TN_Timeout           timeout
      )
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;
   BOOL waited_for_event = FALSE;

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   rc = _eventgrp_wait(eventgrp, wait_pattern, wait_mode, p_flags_pattern);

   if (rc == TN_RC_TIMEOUT && timeout != 0){
      tn_curr_run_task->subsys_wait.eventgrp.wait_mode = wait_mode;
      tn_curr_run_task->subsys_wait.eventgrp.wait_pattern = wait_pattern;
      _tn_task_curr_to_wait_action(
            &(eventgrp->wait_queue),
            TN_WAIT_REASON_EVENT,
            timeout
            );
      waited_for_event = TRUE;
   }

#if TN_DEBUG
   if (!_tn_need_context_switch() && waited_for_event){
      _TN_FATAL_ERROR("");
   }
#endif

   TN_INT_RESTORE();
   _tn_switch_context_if_needed();
   if (waited_for_event){
      if (     tn_curr_run_task->task_wait_rc == TN_RC_OK
            && p_flags_pattern != NULL )
      {
         *p_flags_pattern = 
            tn_curr_run_task->subsys_wait.eventgrp.actual_pattern;
      }
      rc = tn_curr_run_task->task_wait_rc;
   }

out:
   return rc;
}


/*
 * See comments in the header file (tn_eventgrp.h)
 */
enum TN_RCode tn_eventgrp_wait_polling(
      struct TN_EventGrp  *eventgrp,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   rc = _eventgrp_wait(eventgrp, wait_pattern, wait_mode, p_flags_pattern);

   TN_INT_RESTORE();

out:
   return rc;
}


/*
 * See comments in the header file (tn_eventgrp.h)
 */
enum TN_RCode tn_eventgrp_iwait_polling(
      struct TN_EventGrp  *eventgrp,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   TN_INTSAVE_DATA_INT;
   enum TN_RCode rc;

   if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_IDIS_SAVE();

   rc = _eventgrp_wait(eventgrp, wait_pattern, wait_mode, p_flags_pattern);

   TN_INT_IRESTORE();

out:
   return rc;
}


/*
 * See comments in the header file (tn_eventgrp.h)
 */
enum TN_RCode tn_eventgrp_modify(
      struct TN_EventGrp  *eventgrp,
      enum TN_EGrpOp       operation,
      unsigned int         pattern
      )
{
   TN_INTSAVE_DATA;
   enum TN_RCode rc = TN_RC_OK;

   if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_DIS_SAVE();

   rc = _eventgrp_modify(eventgrp, operation, pattern);

   TN_INT_RESTORE();
   _tn_switch_context_if_needed();

out:
   return rc;
}


/*
 * See comments in the header file (tn_eventgrp.h)
 */
enum TN_RCode tn_eventgrp_imodify(
      struct TN_EventGrp  *eventgrp,
      enum TN_EGrpOp       operation,
      unsigned int         pattern
      )
{
   TN_INTSAVE_DATA_INT;
   enum TN_RCode rc;

   if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
      goto out;
   }

   TN_INT_IDIS_SAVE();

   rc = _eventgrp_modify(eventgrp, operation, pattern);

   TN_INT_IRESTORE();

out:
   return rc;
}


