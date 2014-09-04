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


//-- common tnkernel headers
#include "tn_common.h"
#include "tn_sys.h"
#include "tn_internal.h"

//-- header of current module
#include "tn_sem.h"

//-- header of other needed modules
#include "tn_tasks.h"


//----------------------------------------------------------------------------
//   Structure's field sem->id_sem have to be set to 0
//----------------------------------------------------------------------------
enum TN_Retval tn_sem_create(struct TN_Sem * sem,
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

   TN_CHECK_NON_INT_CONTEXT;

   tn_list_reset(&(sem->wait_queue));

   sem->count     = start_value;
   sem->max_count = max_val;
   sem->id_sem    = TN_ID_SEMAPHORE;

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_sem_delete(struct TN_Sem * sem)
{
   TN_INTSAVE_DATA
   struct TN_ListItem * que;
   struct TN_Task * task;

#if TN_CHECK_PARAM
   if(sem == NULL)
      return TERR_WRONG_PARAM;
   if(sem->id_sem != TN_ID_SEMAPHORE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt(); // v.2.7 - thanks to Eugene Scopal

   while(!tn_is_list_empty(&(sem->wait_queue)))
   {
     //--- delete from the sem wait queue

      que = tn_list_remove_head(&(sem->wait_queue));
      task = get_task_by_tsk_queue(que);
      if(_tn_task_wait_complete(task, (TN_WCOMPL__TO_RUNNABLE)))
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
enum TN_Retval tn_sem_signal(struct TN_Sem * sem)
{
   TN_INTSAVE_DATA
   enum TN_Retval rc; //-- return code
   struct TN_ListItem * que;
   struct TN_Task * task;

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

   if(!(tn_is_list_empty(&(sem->wait_queue))))
   {
      //--- delete from the sem wait queue

      que = tn_list_remove_head(&(sem->wait_queue));
      task = get_task_by_tsk_queue(que);

      if(_tn_task_wait_complete(task, (TN_WCOMPL__TO_RUNNABLE)))
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
enum TN_Retval tn_sem_isignal(struct TN_Sem * sem)
{
   TN_INTSAVE_DATA_INT
   enum TN_Retval rc;
   struct TN_ListItem * que;
   struct TN_Task * task;

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

   if(!(tn_is_list_empty(&(sem->wait_queue))))
   {
      //--- delete from the sem wait queue

      que = tn_list_remove_head(&(sem->wait_queue));
      task = get_task_by_tsk_queue(que);

      if(_tn_task_wait_complete(task, (TN_WCOMPL__TO_RUNNABLE)))
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
enum TN_Retval tn_sem_acquire(struct TN_Sem * sem, unsigned long timeout)
{
   TN_INTSAVE_DATA
   enum TN_Retval rc; //-- return code

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
      _tn_task_curr_to_wait_action(&(sem->wait_queue), TSK_WAIT_REASON_SEM, timeout);
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
enum TN_Retval tn_sem_polling(struct TN_Sem * sem)
{
   TN_INTSAVE_DATA
   enum TN_Retval rc;

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
enum TN_Retval tn_sem_ipolling(struct TN_Sem * sem)
{
   TN_INTSAVE_DATA_INT
   enum TN_Retval rc;

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

