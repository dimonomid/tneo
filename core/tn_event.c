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
#include "tn_event.h"

//-- header of other needed modules
#include "tn_tasks.h"

#if  TN_USE_EVENTS



/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

typedef enum TN_Retval (_worker_t)(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern
      );


/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

static BOOL _cond_check(struct TN_Event *evf, enum TN_EventWCond wait_mode, int wait_pattern)
{
   BOOL cond = FALSE;

   switch (wait_mode){
      case TN_EVENT_WCOND_OR:
         //-- any bit set is enough for release condition
         cond = ((evf->pattern & wait_pattern) != 0);
         break;
      case TN_EVENT_WCOND_AND:
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

static void _clear_pattern_if_needed(struct TN_Event *evf)
{
   if (evf->attr & TN_EVENT_ATTR_CLR){
      evf->pattern = 0;
   }
}

static BOOL _scan_event_waitqueue(struct TN_Event *evf)
{
   struct TN_Task *task;
   BOOL rc = FALSE;

   // checking ALL of the tasks waiting on the event.

   // for the event with attr TN_EVENT_ATTR_SINGLE the only one task
   // may be in the queue

   tn_list_for_each_entry(task, &(evf->wait_queue), task_queue){
      if (_cond_check(evf, task->ewait_mode, task->ewait_pattern)){
         //-- Condition to finish the waiting
         task->ewait_pattern = evf->pattern;
         _tn_task_wait_complete(task, TERR_NO_ERR, (TN_WCOMPL__REMOVE_WQUEUE));

         //-- NOTE: here we don't check _tn_need_context_switch(), because, say,
         //         TN_EVENT_ATTR_CLR should anyway be handled,
         //         even if no context switch is needed
         rc = TRUE;
      }
   }

   return rc;
}


static inline enum TN_Retval _event_wait(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   enum TN_Retval rc = TERR_NO_ERR;

   //-- If event attr is TN_EVENT_ATTR_SINGLE and another task already
   //-- in event wait queue - return ERROR without checking release condition

   if ((evf->attr & TN_EVENT_ATTR_SINGLE) && !tn_is_list_empty(&(evf->wait_queue))){
      rc = TERR_ILUSE;
   } else {
      //-- Check release condition

      if (_cond_check(evf, wait_mode, wait_pattern)){
         *p_flags_pattern = evf->pattern;
         _clear_pattern_if_needed(evf);
         rc = TERR_NO_ERR;
      } else {
         rc = TERR_TIMEOUT;
      }
   }

   return rc;
}

static inline enum TN_Retval _event_set(
      struct TN_Event     *evf,
      unsigned int         pattern,
      enum TN_EventWCond   _unused1,
      unsigned int        *_unused2
      )
{
   enum TN_Retval rc = TERR_NO_ERR;

   evf->pattern |= pattern;

   if (_scan_event_waitqueue(evf)){
      //-- There are tasks whose waiting state is complete
      _clear_pattern_if_needed(evf);
   }

   return rc;
}

static inline enum TN_Retval _event_clear(
      struct TN_Event     *evf,
      unsigned int         pattern,
      enum TN_EventWCond   _unused1,
      unsigned int        *_unused2
      )
{
   evf->pattern &= pattern;

   return TERR_NO_ERR;
}


static inline enum TN_Retval _event_job_perform(
      struct TN_Event     *evf,
      _worker_t            p_worker,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern,
      unsigned long        timeout
      )
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc = TERR_NO_ERR;
   BOOL waited_for_event = FALSE;

#if TN_CHECK_PARAM
   if(evf == NULL 
         || (
            p_worker == _event_wait
            && (wait_pattern == 0 || p_flags_pattern == NULL)
            )
         )
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = p_worker(evf, wait_pattern, wait_mode, p_flags_pattern);

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
      if (tn_curr_run_task->task_wait_rc == TERR_NO_ERR){
         *p_flags_pattern = tn_curr_run_task->ewait_pattern;
      }
      rc = tn_curr_run_task->task_wait_rc;
   }

   return rc;
}

static inline enum TN_Retval _event_job_iperform(
      struct TN_Event     *evf,
      _worker_t            p_worker,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   TN_INTSAVE_DATA_INT;
   enum TN_Retval rc;

#if TN_CHECK_PARAM
   if(evf == NULL || wait_pattern == 0 || p_flags_pattern == NULL)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT;

   tn_idisable_interrupt();

   rc = p_worker(evf, wait_pattern, wait_mode, p_flags_pattern);

   tn_ienable_interrupt();

   return rc;
}




/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/


//----------------------------------------------------------------------------
//  Structure's field evf->id_event have to be set to 0
//----------------------------------------------------------------------------
enum TN_Retval tn_event_create(struct TN_Event * evf,
                    enum TN_EventAttr attr,              //-- Eventflag attribute
                    unsigned int pattern)  //-- Initial value of the eventflag bit pattern
{

#if TN_CHECK_PARAM
   if(evf == NULL)
      return TERR_WRONG_PARAM;
   if(evf->id_event == TN_ID_EVENT ||
        (((attr & TN_EVENT_ATTR_SINGLE) == 0)  &&
            ((attr & TN_EVENT_ATTR_MULTI) == 0)))
      return TERR_WRONG_PARAM;
#endif

   tn_list_reset(&(evf->wait_queue));

   evf->pattern = pattern;
   evf->attr = attr;
   if ((attr & TN_EVENT_ATTR_CLR) && ((attr & TN_EVENT_ATTR_SINGLE)== 0)){
      evf->attr = TN_INVALID_VAL;
      return TERR_WRONG_PARAM;
   }

   evf->id_event = TN_ID_EVENT;

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_event_delete(struct TN_Event * evf)
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
enum TN_Retval tn_event_wait(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern,
      unsigned long        timeout
      )
{
   return _event_job_perform(evf, _event_wait, wait_pattern, wait_mode, p_flags_pattern, timeout);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_event_wait_polling(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   return _event_job_perform(evf, _event_wait, wait_pattern, wait_mode, p_flags_pattern, 0);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_event_iwait(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern
      )
{
   return _event_job_iperform(evf, _event_wait, wait_pattern, wait_mode, p_flags_pattern);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_event_set(struct TN_Event *evf, unsigned int pattern)
{
   return _event_job_perform(evf, _event_set, pattern, 0, 0, 0);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_event_iset(struct TN_Event *evf, unsigned int pattern)
{
   return _event_job_iperform(evf, _event_set, pattern, 0, 0);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_event_clear(struct TN_Event *evf, unsigned int pattern)
{
   return _event_job_perform(evf, _event_clear, pattern, 0, 0, 0);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_event_iclear(struct TN_Event * evf, unsigned int pattern)
{
   return _event_job_iperform(evf, _event_clear, pattern, 0, 0);
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





