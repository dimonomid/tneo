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

#include "tn_common.h"
#include "tn_sys.h"
#include "tn_internal.h"

#include "tn_dqueue.h"

#include "tn_tasks.h"




/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

enum _JobType {
   _JOB_TYPE__SEND,
   _JOB_TYPE__RECEIVE,
};

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Data queue storage FIFO processing {{{

//---------------------------------------------------------------------------
static enum TN_Retval  dque_fifo_write(struct TN_DQueue *dque, void *p_data)
{
   register int flag;

#if TN_CHECK_PARAM
   if(dque == NULL)
      return TERR_WRONG_PARAM;
#endif

   //-- v.2.7

   if(dque->items_cnt <= 0)
      return TERR_OUT_OF_MEM;

   flag = ((dque->tail_idx == 0 && dque->head_idx == dque->items_cnt - 1)
         || dque->head_idx == dque->tail_idx-1);
   if(flag)
      return  TERR_OVERFLOW;  //--  full

   //-- wr  data

   dque->data_fifo[dque->head_idx] = p_data;
   dque->head_idx++;
   if(dque->head_idx >= dque->items_cnt)
      dque->head_idx = 0;
   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
static enum TN_Retval  dque_fifo_read(struct TN_DQueue * dque, void **pp_data)
{

#if TN_CHECK_PARAM
   if(dque == NULL || pp_data == NULL)
      return TERR_WRONG_PARAM;
#endif

   //-- v.2.7  Thanks to kosyak© from electronix.ru

   if(dque->items_cnt <= 0)
      return TERR_OUT_OF_MEM;

   if(dque->tail_idx == dque->head_idx)
      return TERR_UNDERFLOW; //-- empty

   //-- rd data

   *pp_data = dque->data_fifo[dque->tail_idx];
   dque->tail_idx++;
   if(dque->tail_idx >= dque->items_cnt)
      dque->tail_idx = 0;

   return TERR_NO_ERR;
}
// }}}


static inline enum TN_Retval _queue_send(struct TN_DQueue *dque, void *p_data)
{
   enum TN_Retval rc = TERR_NO_ERR;

   if (!tn_is_list_empty(&(dque->wait_receive_list))){
      struct TN_Task *task;
      //-- there are tasks waiting for message,
      //   so, wake up first one

      //-- get first task from wait_receive_list
      task = tn_list_first_entry(&(dque->wait_receive_list), typeof(*task), task_queue);

      task->data_elem = p_data;

      _tn_task_wait_complete(task, TERR_NO_ERR);
   } else {
      //-- the data queue's  wait_receive list is empty
      rc = dque_fifo_write(dque, p_data);
      if (rc == TERR_OUT_OF_MEM || rc == TERR_OVERFLOW){
         //-- No free items in data queue: just convert errorcode
         rc = TERR_TIMEOUT;
      } else {
         //-- data is either successfully written or failed with some
         //   unexpected error. In either case, leave return code as is.
      }
   }

   return rc;
}

static inline enum TN_Retval _queue_receive(struct TN_DQueue *dque, void **pp_data)
{
   enum TN_Retval rc = TERR_NO_ERR;

   rc = dque_fifo_read(dque, pp_data);

   switch (rc){
      case TERR_NO_ERR:
         //-- data is successfully read from the queue.
         //   Let's check whether there is some task that
         //   wants to write more data, and waits for room
         if (!tn_is_list_empty(&(dque->wait_send_list))){
            struct TN_Task *task;
            //-- there are tasks that want to write data

            task = tn_list_first_entry(&(dque->wait_send_list), typeof(*task), task_queue);

            rc = dque_fifo_write(dque, task->data_elem); //-- Put to data FIFO
            if (rc != TERR_NO_ERR){
               TN_FATAL_ERROR("rc should always be TERR_NO_ERR here");
            }

            _tn_task_wait_complete(task, TERR_NO_ERR);
         }
         break;
      case TERR_OUT_OF_MEM:
      case TERR_UNDERFLOW:
         //-- data FIFO is empty, there's nothing to read.
         //   let's check if some task waits to write
         //   (that might happen if only dque->items_cnt is 0)
         if (!tn_is_list_empty(&(dque->wait_send_list))){
            struct TN_Task *task;
            //-- there are tasks that want to write data
            //   (that might happen if only dque->items_cnt is 0)

            task = tn_list_first_entry(&(dque->wait_send_list), typeof(*task), task_queue);

            *pp_data = task->data_elem; //-- Return to caller
            _tn_task_wait_complete(task, TERR_NO_ERR);

            rc = TERR_NO_ERR;
         } else {
            //-- wait_send_list is empty. return TERR_TIMEOUT
            rc = TERR_TIMEOUT;
         }
         break;
      default:
         //-- there's some abnormal error, we should leave return code as is
         break;

   }

   return rc;
}


static inline enum TN_Retval _dqueue_job_perform(
      struct TN_DQueue *dque,
      enum _JobType job_type,
      void *p_data,
      unsigned long timeout
      )
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc = TERR_NO_ERR;
   BOOL waited = FALSE;
   void **pp_data = (void **)p_data;

#if TN_CHECK_PARAM
   if(dque == NULL)
      return  TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   switch (job_type){
      case _JOB_TYPE__SEND:
         rc = _queue_send(dque, p_data);

         if (rc == TERR_TIMEOUT && timeout != 0){
            //-- put current task to wait until there's room in the queue.
            tn_curr_run_task->data_elem = p_data;  //-- Store p_data
            _tn_task_curr_to_wait_action(
                  &(dque->wait_send_list),
                  TSK_WAIT_REASON_DQUE_WSEND,
                  timeout
                  );

            waited = TRUE;
         }

         break;
      case _JOB_TYPE__RECEIVE:
         rc = _queue_receive(dque, pp_data);

         if (rc == TERR_TIMEOUT && timeout != 0){
            //-- put current task to wait until new data comes.
            _tn_task_curr_to_wait_action(
                  &(dque->wait_receive_list),
                  TSK_WAIT_REASON_DQUE_WRECEIVE,
                  timeout
                  );

            waited = TRUE;
         }
         break;
   }

#if TN_DEBUG
   if (!_tn_need_context_switch() && waited){
      TN_FATAL_ERROR("");
   }
#endif

   tn_enable_interrupt();
   _tn_switch_context_if_needed();
   if (waited){
      switch (job_type){
         case _JOB_TYPE__SEND:
            rc = tn_curr_run_task->task_wait_rc;
            break;
         case _JOB_TYPE__RECEIVE:
            //-- When returns to this point, in the data_elem have to be valid value

            *pp_data = tn_curr_run_task->data_elem; //-- Return to caller
            rc = tn_curr_run_task->task_wait_rc;
            break;
      }
   }

   return rc;
}

static inline enum TN_Retval _dqueue_job_iperform(
      struct TN_DQueue *dque,
      enum _JobType job_type,
      void *p_data      //-- used for _JOB_TYPE__SEND
      )
{
   TN_INTSAVE_DATA_INT;
   enum TN_Retval rc = TERR_NO_ERR;
   void **pp_data = (void **)p_data;

#if TN_CHECK_PARAM
   if(dque == NULL)
      return  TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_INT_CONTEXT;

   tn_idisable_interrupt();

   switch (job_type){
      case _JOB_TYPE__SEND:
         rc = _queue_send(dque, p_data);
         break;
      case _JOB_TYPE__RECEIVE:
         rc = _queue_receive(dque, pp_data);
         break;
   }

   tn_ienable_interrupt();

   return rc;
}





/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

//-------------------------------------------------------------------------
// Structure's field dque->id_dque have to be set to 0
//-------------------------------------------------------------------------
enum TN_Retval tn_queue_create(
      struct TN_DQueue *dque,      //-- Ptr to already existing struct TN_DQueue
      void **data_fifo,   //-- Ptr to already existing array of void * to store items data queue.
                          //   Can be NULL.
      int items_cnt     //-- capacity (total items count). Can be 0.
      )
{
#if TN_CHECK_PARAM
   if(dque == NULL)
      return TERR_WRONG_PARAM;
   if(items_cnt < 0 || dque->id_dque == TN_ID_DATAQUEUE)
      return TERR_WRONG_PARAM;
#endif

   tn_list_reset(&(dque->wait_send_list));
   tn_list_reset(&(dque->wait_receive_list));

   dque->data_fifo      = data_fifo;
   dque->items_cnt    = items_cnt;
   if (dque->data_fifo == NULL){
      dque->items_cnt = 0;
   }

   dque->tail_idx   = 0;
   dque->head_idx = 0;

   dque->id_dque = TN_ID_DATAQUEUE;

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_queue_delete(struct TN_DQueue * dque)
{
   TN_INTSAVE_DATA;

#if TN_CHECK_PARAM
   if(dque == NULL)
      return TERR_WRONG_PARAM;
   if(dque->id_dque != TN_ID_DATAQUEUE)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt(); // v.2.7 - thanks to Eugene Scopal

   _tn_wait_queue_notify_deleted(&(dque->wait_send_list),    TN_INTSAVE_DATA_ARG_GIVE);
   _tn_wait_queue_notify_deleted(&(dque->wait_receive_list), TN_INTSAVE_DATA_ARG_GIVE);

   dque->id_dque = 0; //-- data queue does not exist now

   tn_enable_interrupt();

   return TERR_NO_ERR;

}

//----------------------------------------------------------------------------
enum TN_Retval tn_queue_send(struct TN_DQueue *dque, void *p_data, unsigned long timeout)
{
   return _dqueue_job_perform(dque, _JOB_TYPE__SEND, p_data, timeout);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_queue_send_polling(struct TN_DQueue *dque, void *p_data)
{
   return _dqueue_job_perform(dque, _JOB_TYPE__SEND, p_data, 0);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_queue_isend_polling(struct TN_DQueue *dque, void *p_data)
{
   return _dqueue_job_iperform(dque, _JOB_TYPE__SEND, p_data);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_queue_receive(struct TN_DQueue *dque, void **pp_data, unsigned long timeout)
{
   return _dqueue_job_perform(dque, _JOB_TYPE__RECEIVE, pp_data, timeout);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_queue_receive_polling(struct TN_DQueue *dque, void **pp_data)
{
   return _dqueue_job_perform(dque, _JOB_TYPE__RECEIVE, pp_data, 0);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_queue_ireceive(struct TN_DQueue *dque, void **pp_data)
{
   return _dqueue_job_iperform(dque, _JOB_TYPE__RECEIVE, pp_data);
}





//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


