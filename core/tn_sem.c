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

//----------------------------------------------------------------------------
//   Structure's field sem->id_sem have to be set to 0
//----------------------------------------------------------------------------
int tn_sem_create(TN_SEM * sem,
                  int start_value,
                  int max_val)
{

#if TN_CHECK_PARAM
   if(sem == NULL) //-- Thanks to Michael Fisher
      return  TERR_WRONG_PARAM;
   if(max_val <= 0 || start_value < 0 ||
         start_value > max_val || sem->id_sem != 0) //-- no recreation
   {
      sem->max_count = 0;
      return  TERR_WRONG_PARAM;
   }
#endif

   TN_CHECK_NON_INT_CONTEXT

   queue_reset(&(sem->wait_queue));

   sem->count     = start_value;
   sem->max_count = max_val;
   sem->id_sem    = TN_ID_SEMAPHORE;

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_sem_delete(TN_SEM * sem)
{
   TN_INTSAVE_DATA
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(sem == NULL)
      return TERR_WRONG_PARAM;
   if(sem->id_sem != TN_ID_SEMAPHORE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt(); // v.2.7 - thanks to Eugene Scopal

   while(!is_queue_empty(&(sem->wait_queue)))
   {
     //--- delete from the sem wait queue

      que = queue_remove_head(&(sem->wait_queue));
      task = get_task_by_tsk_queue(que);
      if(task_wait_complete(task))
      {
         task->task_wait_rc = TERR_DLT;
         tn_enable_interrupt();
         tn_switch_context();
         tn_disable_interrupt(); // v.2.7
      }
   }

   sem->id_sem = 0; // Semaphore not exists now

   tn_enable_interrupt();

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
//  Release Semaphore Resource
//----------------------------------------------------------------------------
int tn_sem_signal(TN_SEM * sem)
{
   TN_INTSAVE_DATA
   int rc; //-- return code
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(sem == NULL)
      return  TERR_WRONG_PARAM;
   if(sem->max_count == 0)
      return  TERR_WRONG_PARAM;
   if(sem->id_sem != TN_ID_SEMAPHORE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(!(is_queue_empty(&(sem->wait_queue))))
   {
      //--- delete from the sem wait queue

      que = queue_remove_head(&(sem->wait_queue));
      task = get_task_by_tsk_queue(que);

      if(task_wait_complete(task))
      {
         tn_enable_interrupt();
         tn_switch_context();

         return TERR_NO_ERR;
      }
      rc = TERR_NO_ERR;
   }
   else
   {
      if(sem->count < sem->max_count)
      {
         sem->count++;
         rc = TERR_NO_ERR;
      }
      else
         rc = TERR_OVERFLOW;
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
// Release Semaphore Resource inside Interrupt
//----------------------------------------------------------------------------
int tn_sem_isignal(TN_SEM * sem)
{
   TN_INTSAVE_DATA_INT
   int rc;
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(sem == NULL)
      return  TERR_WRONG_PARAM;
   if(sem->max_count == 0)
      return  TERR_WRONG_PARAM;
   if(sem->id_sem != TN_ID_SEMAPHORE)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

   if(!(is_queue_empty(&(sem->wait_queue))))
   {
      //--- delete from the sem wait queue

      que = queue_remove_head(&(sem->wait_queue));
      task = get_task_by_tsk_queue(que);

      if(task_wait_complete(task))
      {
         tn_ienable_interrupt();

         return TERR_NO_ERR;
      }
      rc = TERR_NO_ERR;
   }
   else
   {
      if(sem->count < sem->max_count)
      {
         sem->count++;
         rc = TERR_NO_ERR;
      }
      else
         rc = TERR_OVERFLOW;
   }

   tn_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//   Acquire Semaphore Resource
//----------------------------------------------------------------------------
int tn_sem_acquire(TN_SEM * sem, unsigned long timeout)
{
   TN_INTSAVE_DATA
   int rc; //-- return code

#if TN_CHECK_PARAM
   if(sem == NULL || timeout == 0)
      return  TERR_WRONG_PARAM;
   if(sem->max_count == 0)
      return  TERR_WRONG_PARAM;
   if(sem->id_sem != TN_ID_SEMAPHORE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(sem->count >= 1)
   {
      sem->count--;
      rc = TERR_NO_ERR;
   }
   else
   {
      task_curr_to_wait_action(&(sem->wait_queue), TSK_WAIT_REASON_SEM, timeout);
      tn_enable_interrupt();
      tn_switch_context();

      return tn_curr_run_task->task_wait_rc;
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//  Acquire(Polling) Semaphore Resource (do not call  in the interrupt)
//----------------------------------------------------------------------------
int tn_sem_polling(TN_SEM * sem)
{
   TN_INTSAVE_DATA
   int rc;

#if TN_CHECK_PARAM
   if(sem == NULL)
      return  TERR_WRONG_PARAM;
   if(sem->max_count == 0)
      return  TERR_WRONG_PARAM;
   if(sem->id_sem != TN_ID_SEMAPHORE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   if(sem->count >= 1)
   {
      sem->count--;
      rc = TERR_NO_ERR;
   }
   else
      rc = TERR_TIMEOUT;

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
// Acquire(Polling) Semaphore Resource inside interrupt
//----------------------------------------------------------------------------
int tn_sem_ipolling(TN_SEM * sem)
{
   TN_INTSAVE_DATA_INT
   int rc;

#if TN_CHECK_PARAM
   if(sem == NULL)
      return  TERR_WRONG_PARAM;
   if(sem->max_count == 0)
      return  TERR_WRONG_PARAM;
   if(sem->id_sem != TN_ID_SEMAPHORE)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

   if(sem->count >= 1)
   {
      sem->count--;
      rc = TERR_NO_ERR;
   }
   else
      rc = TERR_TIMEOUT;

   tn_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

