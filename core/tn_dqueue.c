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

//-------------------------------------------------------------------------
// Structure's field dque->id_dque have to be set to 0
//-------------------------------------------------------------------------
int tn_queue_create(TN_DQUE * dque,     //-- Ptr to already existing TN_DQUE
                    void ** data_fifo,  //-- Ptr to already existing array of void * to store data queue entries.Can be NULL
                    int num_entries)    //-- Capacity of data queue(num entries).Can be 0

{

#if TN_CHECK_PARAM
   if(dque == NULL)
      return TERR_WRONG_PARAM;
   if(num_entries < 0 || dque->id_dque == TN_ID_DATAQUEUE)
      return TERR_WRONG_PARAM;
#endif

   queue_reset(&(dque->wait_send_list));
   queue_reset(&(dque->wait_receive_list));

   dque->data_fifo = data_fifo;
   dque->num_entries = num_entries;
   if(dque->data_fifo == NULL)
      dque->num_entries = 0;

   dque->tail_cnt   = 0;
   dque->header_cnt = 0;

   dque->id_dque = TN_ID_DATAQUEUE;

   return  TERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_queue_delete(TN_DQUE * dque)
{
   TN_INTSAVE_DATA
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(dque == NULL)
      return TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt(); // v.2.7 - thanks to Eugene Scopal

   while(!is_queue_empty(&(dque->wait_send_list)))
   {

     //--- delete from sem wait queue

      que = queue_remove_head(&(dque->wait_send_list));
      task = get_task_by_tsk_queue(que);
      if(task_wait_complete(task))
      {
         task->task_wait_rc = TERR_DLT;
         tn_enable_interrupt();
         tn_switch_context();
         tn_disable_interrupt(); // v.2.7
      }
   }

   while(!is_queue_empty(&(dque->wait_receive_list)))
   {
     //--- delete from sem wait queue

      que = queue_remove_head(&(dque->wait_receive_list));
      task = get_task_by_tsk_queue(que);
      if(task_wait_complete(task))
      {
         task->task_wait_rc = TERR_DLT;
         tn_enable_interrupt();
         tn_switch_context();
         tn_disable_interrupt(); // v.2.7
      }
   }
      
   dque->id_dque = 0; // Data queue not exists now

   tn_enable_interrupt();

   return TERR_NO_ERR;

}

//----------------------------------------------------------------------------
int tn_queue_send(TN_DQUE * dque, void * data_ptr, unsigned long timeout)
{
   TN_INTSAVE_DATA
   int rc;
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(dque == NULL || timeout == 0)
      return  TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

  //-- there are task(s) in the data queue's wait_receive list

   if(!is_queue_empty(&(dque->wait_receive_list)))
   {
      que  = queue_remove_head(&(dque->wait_receive_list));
      task = get_task_by_tsk_queue(que);

      task->data_elem = data_ptr;

      if(task_wait_complete(task))
      {
         tn_enable_interrupt();
         tn_switch_context();
         return  TERR_NO_ERR;
      }
      rc = TERR_NO_ERR;
   }
   else  //-- the data queue's  wait_receive list is empty
   {
      rc = dque_fifo_write(dque,data_ptr);
      if(rc != TERR_NO_ERR)  //-- No free entries in the data queue
      {
         tn_curr_run_task->data_elem = data_ptr;  //-- Store data_ptr
         task_curr_to_wait_action(&(dque->wait_send_list),
                                         TSK_WAIT_REASON_DQUE_WSEND, timeout);
         tn_enable_interrupt();
         tn_switch_context();
         return tn_curr_run_task->task_wait_rc;
      }
   }

   tn_enable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
int tn_queue_send_polling(TN_DQUE * dque, void * data_ptr)
{
   TN_INTSAVE_DATA
   int rc;
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(dque == NULL)
      return  TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

  //-- there are task(s) in the data queue's  wait_receive list

   if(!is_queue_empty(&(dque->wait_receive_list)))
   {
      que  = queue_remove_head(&(dque->wait_receive_list));
      task = get_task_by_tsk_queue(que);

      task->data_elem = data_ptr;

      if(task_wait_complete(task))
      {
         tn_enable_interrupt();
         tn_switch_context();
         return  TERR_NO_ERR;
      }
      rc = TERR_NO_ERR;
   }
   else //-- the data queue's  wait_receive list is empty
   {
      rc = dque_fifo_write(dque,data_ptr);
      if(rc != TERR_NO_ERR)  //-- No free entries in data queue
         rc = TERR_TIMEOUT;  //-- Just convert errorcode
   }
   tn_enable_interrupt();
   return rc;
}

//----------------------------------------------------------------------------
int tn_queue_isend_polling(TN_DQUE * dque, void * data_ptr)
{
   TN_INTSAVE_DATA_INT
   int rc;
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(dque == NULL)
      return  TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

  //-- there are task(s) in the data queue's  wait_receive list

   if(!is_queue_empty(&(dque->wait_receive_list)))
   {
      que  = queue_remove_head(&(dque->wait_receive_list));
      task = get_task_by_tsk_queue(que);

      task->data_elem = data_ptr;

      if(task_wait_complete(task))
      {
         tn_ienable_interrupt();
         return TERR_NO_ERR;
      }
      rc = TERR_NO_ERR;
   }
   else  //-- the data queue's wait_receive list is empty
   {
      rc = dque_fifo_write(dque,data_ptr);

      if(rc != TERR_NO_ERR)  //-- No free entries in data queue
         rc = TERR_TIMEOUT;  //-- Just convert errorcode
   }

   tn_ienable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_queue_receive(TN_DQUE * dque,void ** data_ptr,unsigned long timeout)
{
   TN_INTSAVE_DATA
   int rc; //-- return code
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(dque == NULL || timeout == 0 || data_ptr == NULL)
      return  TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   rc = dque_fifo_read(dque,data_ptr);
   if(rc == TERR_NO_ERR)  //-- There was entry(s) in data queue
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         dque_fifo_write(dque,task->data_elem); //-- Put to data FIFO

         if(task_wait_complete(task))
         {
            tn_enable_interrupt();
            tn_switch_context();
            return TERR_NO_ERR;
         }
      }
   }
   else //-- data FIFO is empty
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         *data_ptr = task->data_elem; //-- Return to caller

         if(task_wait_complete(task))
         {
            tn_enable_interrupt();
            tn_switch_context();
            return TERR_NO_ERR;
         }
         rc = TERR_NO_ERR;
      }
      else //-- wait_send_list is empty
      {
         task_curr_to_wait_action(&(dque->wait_receive_list),
                                     TSK_WAIT_REASON_DQUE_WRECEIVE,timeout);
         tn_enable_interrupt();
         tn_switch_context();

         //-- When returns to this point, in the data_elem have to be valid value

         *data_ptr = tn_curr_run_task->data_elem; //-- Return to caller

         return tn_curr_run_task->task_wait_rc;
      }
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_queue_receive_polling(TN_DQUE * dque,void ** data_ptr)
{
   TN_INTSAVE_DATA
   int rc;
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(dque == NULL || data_ptr == NULL)
      return  TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT

   tn_disable_interrupt();

   rc = dque_fifo_read(dque,data_ptr);
   if(rc == TERR_NO_ERR)  //-- There was entry(s) in data queue
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         dque_fifo_write(dque,task->data_elem); //-- Put to data FIFO

         if(task_wait_complete(task))
         {
            tn_enable_interrupt();
            tn_switch_context();
            return TERR_NO_ERR;
         }
      }
   }
   else //-- data FIFO is empty
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         *data_ptr = task->data_elem; //-- Return to caller

         if(task_wait_complete(task))
         {
            tn_enable_interrupt();
            tn_switch_context();
            return TERR_NO_ERR;
         }
         rc = TERR_NO_ERR;
      }
      else //--   wait_send_list is empty
         rc = TERR_TIMEOUT;
   }

   tn_enable_interrupt();

   return rc;
}

//----------------------------------------------------------------------------
int tn_queue_ireceive(TN_DQUE * dque,void ** data_ptr)
{
   TN_INTSAVE_DATA_INT
   int rc;
   CDLL_QUEUE * que;
   TN_TCB * task;

#if TN_CHECK_PARAM
   if(dque == NULL || data_ptr == NULL)
      return  TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT

   tn_idisable_interrupt();

   rc = dque_fifo_read(dque,data_ptr);
   if(rc == TERR_NO_ERR)  //-- There was entry(s) in data queue
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task = get_task_by_tsk_queue(que);

         dque_fifo_write(dque,task->data_elem); //-- Put to data FIFO

         if(task_wait_complete(task))
         {
            tn_ienable_interrupt();
            return TERR_NO_ERR;
         }
      }
   }
   else //-- data FIFO is empty
   {
      if(!is_queue_empty(&(dque->wait_send_list)))
      {
         que  = queue_remove_head(&(dque->wait_send_list));
         task =  get_task_by_tsk_queue(que);

        *data_ptr = task->data_elem; //-- Return to caller

         if(task_wait_complete(task))
         {
            tn_ienable_interrupt();
            return TERR_NO_ERR;
         }
         rc = TERR_NO_ERR;
      }
      else
      {
         rc = TERR_TIMEOUT;
      }
   }

   tn_ienable_interrupt();

   return rc;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


