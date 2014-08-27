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

#include "tn.h"
#include "tn_utils.h"

#ifdef  USE_EVENTS

static int scan_event_waitqueue(TN_EVENT * evf);

//----------------------------------------------------------------------------
//  Structure's field evf->id_event have to be set to 0
//----------------------------------------------------------------------------
int tn_event_create(TN_EVENT * evf,
                    int attr,              //-- Eventflag attribute
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

   queue_reset(&(evf->wait_queue));
   evf->pattern = pattern;
   evf->attr = attr;
   if((attr & TN_EVENT_ATTR_CLR) && ((attr & TN_EVENT_ATTR_SINGLE)== 0))
   {
      evf->attr = TN_INVALID_VAL;
      return TERR_WRONG_PARAM;
   }
   evf->id_event = TN_ID_EVENT;

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_event_delete(TN_EVENT * evf)
{
   TN_INTSAVE_DATA
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(evf == NULL)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();    // v.2.7 - thanks to Eugene Scopal

   while(!is_queue_empty(&(evf->wait_queue)))
   {
     //--- delete from sem wait queue

      que = queue_remove_head(&(evf->wait_queue));
      task = get_task_by_tsk_queue(que);
      if(task_wait_complete(task))
      {
         task->task_wait_rc = TERR_DLT;
         tn_enable_interrupt();
         tn_switch_context();
         tn_disable_interrupt();    // v.2.7
      }
   }

   evf->id_event = 0; // Event not exists now

   tn_enable_interrupt();

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_event_wait(TN_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern,
                    unsigned long timeout)
{
   TN_INTSAVE_DATA
   int rc;
   int fCond;

#if TN_CHECK_PARAM
   if(evf == NULL || wait_pattern == 0 ||
          p_flags_pattern == NULL || timeout == 0)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   //-- If event attr is TN_EVENT_ATTR_SINGLE and another task already
   //-- in event wait queue - return ERROR without checking release condition

   if((evf->attr & TN_EVENT_ATTR_SINGLE) && !is_queue_empty(&(evf->wait_queue)))
   {
      rc = TERR_ILUSE;
   }
   else
   {
       //-- Check release condition

      if(wait_mode & TN_EVENT_WCOND_OR) //-- any setted bit is enough for release condition
         fCond = ((evf->pattern & wait_pattern) != 0);
      else                              //-- TN_EVENT_WCOND_AND is default mode
         fCond = ((evf->pattern & wait_pattern) == wait_pattern);

      if(fCond)
      {
         *p_flags_pattern = evf->pattern;
         if(evf->attr & TN_EVENT_ATTR_CLR)
            evf->pattern = 0;
          rc = TERR_NO_ERR;
      }
      else
      {
         tn_curr_run_task->ewait_mode = wait_mode;
         tn_curr_run_task->ewait_pattern = wait_pattern;
         task_curr_to_wait_action(&(evf->wait_queue),
                                  TSK_WAIT_REASON_EVENT,
                                  timeout);
         tn_enable_interrupt();
         tn_switch_context();

         if(tn_curr_run_task->task_wait_rc == TERR_NO_ERR)
            *p_flags_pattern = tn_curr_run_task->ewait_pattern;
         return tn_curr_run_task->task_wait_rc;
      }
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_event_wait_polling(TN_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern)
{
   TN_INTSAVE_DATA
   int rc;
   int fCond;

#if TN_CHECK_PARAM
   if(evf == NULL || wait_pattern == 0 || p_flags_pattern == NULL)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   //-- If event attr is TN_EVENT_ATTR_SINGLE and another task already
   //-- in event wait queue - return ERROR without checking release condition

   if((evf->attr & TN_EVENT_ATTR_SINGLE) && !is_queue_empty(&(evf->wait_queue)))
   {
      rc = TERR_ILUSE;
   }
   else
   {
       //-- Check release condition

      if(wait_mode & TN_EVENT_WCOND_OR) //-- any setted bit is enough for release condition
         fCond = ((evf->pattern & wait_pattern) != 0);
      else                              //-- TN_EVENT_WCOND_AND is default mode
         fCond = ((evf->pattern & wait_pattern) == wait_pattern);

      if(fCond)
      {
         *p_flags_pattern = evf->pattern;
         if(evf->attr & TN_EVENT_ATTR_CLR)
            evf->pattern = 0;
          rc = TERR_NO_ERR;
      }
      else
         rc = TERR_TIMEOUT;
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_event_iwait(TN_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern)
{
   TN_INTSAVE_DATA_INT
   int rc;
   int fCond;

#if TN_CHECK_PARAM
   if(evf == NULL || wait_pattern == 0 || p_flags_pattern == NULL)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

   //-- If event attr is TN_EVENT_ATTR_SINGLE and another task already
   //-- in event wait queue - return ERROR without checking release condition

   if((evf->attr & TN_EVENT_ATTR_SINGLE) && !is_queue_empty(&(evf->wait_queue)))
   {
      rc = TERR_ILUSE;
   }
   else
   {
       //-- Check release condition

      if(wait_mode & TN_EVENT_WCOND_OR) //-- any setted bit is enough for release condition
         fCond = ((evf->pattern & wait_pattern) != 0);
      else                              //-- TN_EVENT_WCOND_AND is default mode
         fCond = ((evf->pattern & wait_pattern) == wait_pattern);

      if(fCond)
      {
         *p_flags_pattern = evf->pattern;
         if(evf->attr & TN_EVENT_ATTR_CLR)
            evf->pattern = 0;
          rc = TERR_NO_ERR;
      }
      else
         rc = TERR_TIMEOUT;
   }

   tn_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_event_set(TN_EVENT * evf, unsigned int pattern)
{
   TN_INTSAVE_DATA

#if TN_CHECK_PARAM
   if(evf == NULL || pattern == 0)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   evf->pattern |= pattern;

   if(scan_event_waitqueue(evf)) //-- There are task(s) that waiting state is complete
   {
      if(evf->attr & TN_EVENT_ATTR_CLR)
         evf->pattern = 0;

      tn_enable_interrupt();
      tn_switch_context();

      return TERR_NO_ERR;
   }

   tn_enable_interrupt();

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_event_iset(TN_EVENT * evf, unsigned int pattern)
{
   TN_INTSAVE_DATA_INT

#if TN_CHECK_PARAM
   if(evf == NULL || pattern == 0)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

   evf->pattern |= pattern;

   if(scan_event_waitqueue(evf)) //-- There are task(s) that waiting  state is complete
   {
      if(evf->attr & TN_EVENT_ATTR_CLR)
         evf->pattern = 0;

      tn_ienable_interrupt();

      return TERR_NO_ERR;
   }

   tn_ienable_interrupt();

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_event_clear(TN_EVENT * evf, unsigned int pattern)
{
   TN_INTSAVE_DATA

   if(evf == NULL || pattern == TN_INVALID_VAL)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   evf->pattern &= pattern;

   tn_enable_interrupt();
   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_event_iclear(TN_EVENT * evf, unsigned int pattern)
{
   TN_INTSAVE_DATA_INT

#if TN_CHECK_PARAM
   if(evf == NULL || pattern == TN_INVALID_VAL)
      return TERR_WRONG_PARAM;
   if(evf->id_event != TN_ID_EVENT)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

   evf->pattern &= pattern;

   tn_ienable_interrupt();

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
static int scan_event_waitqueue(TN_EVENT * evf)
{
   CDLL_QUEUE * que;
   TN_TCB * task;
   int fCond;
   int rc = 0;

   que = evf->wait_queue.next;

   // checking ALL of the tasks waiting on the event.

   // for the event with attr TN_EVENT_ATTR_SINGLE the only one task
   // may be in the queue

   while(que != &(evf->wait_queue))
   {
      task = get_task_by_tsk_queue(que);
      que  = que->next;

      //-- cond ---

      if(task->ewait_mode & TN_EVENT_WCOND_OR)
         fCond = ((evf->pattern & task->ewait_pattern) != 0);
      else                         //-- TN_EVENT_WCOND_AND is default mode
         fCond = ((evf->pattern & task->ewait_pattern) == task->ewait_pattern);

      if(fCond)   //-- Condition to finish the waiting
      {
         queue_remove_entry(&task->task_queue);
         task->ewait_pattern = evf->pattern;
         if(task_wait_complete(task))   // v.2.7 - thanks to Eugene Scopal
            rc = 1;
      }
   }

   return rc;
}

//----------------------------------------------------------------------------

#endif   //#ifdef  USE_EVENTS

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------





