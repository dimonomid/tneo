/*

  TNKernel real-time kernel

  Copyright © 2004, 2013 Yuri Tiomkin
  All rights reserved.

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

  /* ver 2.7  */

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

#if  TN_USE_EVENTS



/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

static BOOL _cond_check(struct TN_Event *evf, enum TN_EGrpWaitMode wait_mode, int wait_pattern)
{
   BOOL cond = FALSE;

   switch (wait_mode){
      case TN_EGRP_WMODE_OR:
         //-- any bit set is enough for release condition
         cond = ((evf->pattern & wait_pattern) != 0);
         break;
      case TN_EGRP_WMODE_AND:
         //-- all bits should be set for release condition
         cond = ((evf->pattern & wait_pattern) == wait_pattern);
         break;
#if TN_DEBUG
      default:
         TN_FATAL_ERROR("");
         break;
#endif
   }

   return cond;
}

static void _clear_pattern_if_needed(
      struct TN_Event        *evf,
      enum TN_EGrpWaitMode    wait_mode,
      unsigned int            pattern
      )
{
   //-- probably, we should add one more wait mode flag, like 
   //   TN_EGRP_WMODE_CLR, and if it is set, then clear pattern here
}

static void _scan_event_waitqueue(struct TN_Event *evf)
{
   struct TN_Task *task;
   struct TN_Task *tmp_task;

   // checking ALL of the tasks waiting on the event.

   tn_list_for_each_entry_safe(task, tmp_task, &(evf->wait_queue), task_queue){
      if (_cond_check(evf, task->ewait_mode, task->ewait_pattern)){
         //-- Condition to finish the waiting
         task->eactual_pattern = evf->pattern;
         _tn_task_wait_complete(task, TERR_NO_ERR);

         _clear_pattern_if_needed(evf, task->ewait_mode, task->ewait_pattern);
      }
   }
}


static enum TN_Retval _eventgrp_wait(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   enum TN_Retval rc = TERR_NO_ERR;

#if TN_CHECK_PARAM
   if (wait_pattern == 0){
      rc = TERR_WRONG_PARAM;
      goto out;
   }
#endif

   //-- Check release condition

   if (_cond_check(evf, wait_mode, wait_pattern)){
      if (p_flags_pattern != NULL){
         *p_flags_pattern = evf->pattern;
      }
      _clear_pattern_if_needed(evf, wait_mode, wait_pattern);
      rc = TERR_NO_ERR;
   } else {
      rc = TERR_TIMEOUT;
   }

out:
   return rc;
}

static enum TN_Retval _eventgrp_modify(
      struct TN_Event     *evf,
      enum TN_EGrpOp       operation,
      unsigned int         pattern
      )
{
   enum TN_Retval rc = TERR_NO_ERR;

#if TN_CHECK_PARAM
   if (pattern == 0){
      rc = TERR_WRONG_PARAM;
      goto out;
   }
#endif

   switch (operation){
      case TN_EGRP_OP_CLEAR:
         evf->pattern &= ~pattern;
         break;

      case TN_EGRP_OP_SET:
         evf->pattern |= pattern;
         _scan_event_waitqueue(evf);
         break;

      case TN_EGRP_OP_TOGGLE:
         evf->pattern ^= pattern;
         _scan_event_waitqueue(evf);
         break;
   }

out:
   return rc;
}





/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/


//----------------------------------------------------------------------------
//  Structure's field evf->id_event have to be set to 0
//----------------------------------------------------------------------------
enum TN_Retval tn_eventgrp_create(
      struct TN_Event * evf,
      unsigned int initial_pattern       //-- Initial value of the eventflag bit pattern
      )  
{

#if TN_CHECK_PARAM
   if (evf == NULL){
      return TERR_WRONG_PARAM;
   }

   if (evf->id_event == TN_ID_EVENT){
      return TERR_WRONG_PARAM;
   }
#endif

   tn_list_reset(&(evf->wait_queue));

   evf->pattern = initial_pattern;
   evf->id_event = TN_ID_EVENT;

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_eventgrp_delete(struct TN_Event * evf)
{
   TN_INTSAVE_DATA;

#if TN_CHECK_PARAM
   if(evf == NULL)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();    // v.2.7 - thanks to Eugene Scopal

   _tn_wait_queue_notify_deleted(&(evf->wait_queue),  TN_INTSAVE_DATA_ARG_GIVE);

   evf->id_event = 0; //-- event does not exist now

   tn_enable_interrupt();

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_eventgrp_wait(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern,
      unsigned long        timeout
      )
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc = TERR_NO_ERR;
   BOOL waited_for_event = FALSE;

#if TN_CHECK_PARAM
   if (evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = _eventgrp_wait(evf, wait_pattern, wait_mode, p_flags_pattern);

   if (rc == TERR_TIMEOUT && timeout != 0){
      tn_curr_run_task->ewait_mode = wait_mode;
      tn_curr_run_task->ewait_pattern = wait_pattern;
      _tn_task_curr_to_wait_action(
            &(evf->wait_queue),
            TSK_WAIT_REASON_EVENT,
            timeout
            );
      waited_for_event = TRUE;
   }

#if TN_DEBUG
   if (!_tn_need_context_switch() && waited_for_event){
      TN_FATAL_ERROR("");
   }
#endif

   tn_enable_interrupt();
   _tn_switch_context_if_needed();
   if (waited_for_event){
      if (     tn_curr_run_task->task_wait_rc == TERR_NO_ERR
            && p_flags_pattern != NULL )
      {
         *p_flags_pattern = tn_curr_run_task->eactual_pattern;
      }
      rc = tn_curr_run_task->task_wait_rc;
   }

   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_eventgrp_wait_polling(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc = TERR_NO_ERR;

#if TN_CHECK_PARAM
   if (evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = _eventgrp_wait(evf, wait_pattern, wait_mode, p_flags_pattern);

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_eventgrp_iwait(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   TN_INTSAVE_DATA_INT;
   enum TN_Retval rc;

#if TN_CHECK_PARAM
   if (evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT;

   tn_idisable_interrupt();

   rc = _eventgrp_wait(evf, wait_pattern, wait_mode, p_flags_pattern);

   tn_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_eventgrp_modify(
      struct TN_Event     *evf,
      enum TN_EGrpOp       operation,
      unsigned int         pattern
      )
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc = TERR_NO_ERR;

#if TN_CHECK_PARAM
   if (evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = _eventgrp_modify(evf, operation, pattern);

   tn_enable_interrupt();
   _tn_switch_context_if_needed();

   return rc;
}

enum TN_Retval tn_eventgrp_imodify(
      struct TN_Event     *evf,
      enum TN_EGrpOp       operation,
      unsigned int         pattern
      )
{
   TN_INTSAVE_DATA_INT;
   enum TN_Retval rc;

#if TN_CHECK_PARAM
   if (evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT;

   tn_idisable_interrupt();

   rc = _eventgrp_modify(evf, operation, pattern);

   tn_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif   //#ifdef  TN_USE_EVENTS

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------





