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

static enum TN_Retval _fifo_write(struct TN_DQueue *dque, void *p_data)
{
   enum TN_Retval rc = TERR_NO_ERR;

#if TN_CHECK_PARAM
   if (dque == NULL){
      rc = TERR_WRONG_PARAM;
      goto out;
   }
#endif

   if (dque->filled_items_cnt >= dque->items_cnt){
      //-- no space for new data
      rc = TERR_TIMEOUT;
      goto out;
   }

   //-- write data
   dque->data_fifo[dque->head_idx] = p_data;
   dque->filled_items_cnt++;
   dque->head_idx++;
   if (dque->head_idx >= dque->items_cnt){
      dque->head_idx = 0;
   }

out:
   return rc;
}


static enum TN_Retval _fifo_read(struct TN_DQueue *dque, void **pp_data)
{
   enum TN_Retval rc = TERR_NO_ERR;
#if TN_CHECK_PARAM
   if (dque == NULL || pp_data == NULL){
      rc = TERR_WRONG_PARAM;
      goto out;
   }
#endif

   if (dque->filled_items_cnt == 0){
      //-- nothing to read
      rc = TERR_TIMEOUT;
      goto out;
   }

   //-- read data
   *pp_data = dque->data_fifo[dque->tail_idx];
   dque->filled_items_cnt--;
   dque->tail_idx++;
   if (dque->tail_idx >= dque->items_cnt){
      dque->tail_idx = 0;
   }

out:
   return rc;
}
// }}}

static enum TN_Retval _queue_send(
      struct TN_DQueue *dque,
      void *p_data
      )
{
   enum TN_Retval rc = TERR_NO_ERR;

   if (!tn_is_list_empty(&(dque->wait_receive_list))){
      struct TN_Task *task;
      //-- there are tasks waiting for message,
      //   so, wake up first one

      //-- get first task from wait_receive_list
      task = tn_list_first_entry(
            &(dque->wait_receive_list), typeof(*task), task_queue
            );

      task->subsys_wait.dqueue.data_elem = p_data;

      _tn_task_wait_complete(task, TERR_NO_ERR);
   } else {
      //-- the data queue's  wait_receive list is empty
      rc = _fifo_write(dque, p_data);
   }

   return rc;
}

static enum TN_Retval _queue_receive(
      struct TN_DQueue *dque,
      void **pp_data
      )
{
   enum TN_Retval rc = TERR_NO_ERR;

   rc = _fifo_read(dque, pp_data);

   switch (rc){
      case TERR_NO_ERR:
         //-- data is successfully read from the queue.
         //   Let's check whether there is some task that
         //   wants to write more data, and waits for room
         if (!tn_is_list_empty(&(dque->wait_send_list))){
            struct TN_Task *task;
            //-- there are tasks that want to write data

            task = tn_list_first_entry(
                  &(dque->wait_send_list),
                  typeof(*task),
                  task_queue
                  );

            rc = _fifo_write(dque, task->subsys_wait.dqueue.data_elem); //-- Put to data FIFO
            if (rc != TERR_NO_ERR){
               TN_FATAL_ERROR("rc should always be TERR_NO_ERR here");
            }

            _tn_task_wait_complete(task, TERR_NO_ERR);
         }
         break;
      case TERR_TIMEOUT:
         //-- data FIFO is empty, there's nothing to read.
         //   let's check if some task waits to write
         //   (that might happen if only dque->items_cnt is 0)
         if (!tn_is_list_empty(&(dque->wait_send_list))){
            struct TN_Task *task;
            //-- there are tasks that want to write data
            //   (that might happen if only dque->items_cnt is 0)

            task = tn_list_first_entry(
                  &(dque->wait_send_list),
                  typeof(*task),
                  task_queue
                  );

            *pp_data = task->subsys_wait.dqueue.data_elem; //-- Return to caller
            _tn_task_wait_complete(task, TERR_NO_ERR);

            rc = TERR_NO_ERR;
         } else {
            //-- wait_send_list is empty.
         }
         break;
      default:
         //-- there's some abnormal error, we should leave return code as is
         break;

   }

   return rc;
}


static enum TN_Retval _dqueue_job_perform(
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
            //-- save user-provided data in the dqueue.data_elem task field,
            //   and put current task to wait until there's room in the queue.
            tn_curr_run_task->subsys_wait.dqueue.data_elem = p_data;
            _tn_task_curr_to_wait_action(
                  &(dque->wait_send_list),
                  TN_WAIT_REASON_DQUE_WSEND,
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
                  TN_WAIT_REASON_DQUE_WRECEIVE,
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
            //-- dqueue.data_elem should contain valid value now,
            //   return it to caller
            *pp_data = tn_curr_run_task->subsys_wait.dqueue.data_elem;
            rc = tn_curr_run_task->task_wait_rc;
            break;
      }
   }

   return rc;
}

static enum TN_Retval _dqueue_job_iperform(
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

/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_Retval tn_queue_create(
      struct TN_DQueue *dque,
      void **data_fifo,
      int items_cnt
      )
{
#if TN_CHECK_PARAM
   if (dque == NULL){
      return TERR_WRONG_PARAM;
   }
   if (items_cnt < 0 || dque->id_dque == TN_ID_DATAQUEUE){
      return TERR_WRONG_PARAM;
   }
#endif

   tn_list_reset(&(dque->wait_send_list));
   tn_list_reset(&(dque->wait_receive_list));

   dque->data_fifo         = data_fifo;
   dque->items_cnt         = items_cnt;
   if (dque->data_fifo == NULL){
      dque->items_cnt = 0;
   }

   dque->filled_items_cnt  = 0;
   dque->tail_idx          = 0;
   dque->head_idx          = 0;

   dque->id_dque = TN_ID_DATAQUEUE;

   return TERR_NO_ERR;
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_Retval tn_queue_delete(struct TN_DQueue * dque)
{
   TN_INTSAVE_DATA;

#if TN_CHECK_PARAM
   if (dque == NULL){
      return TERR_WRONG_PARAM;
   }
   if (dque->id_dque != TN_ID_DATAQUEUE){
      return TERR_NOEXS;
   }
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   //-- notify waiting tasks that the object is deleted
   //   (TERR_DLT is returned)
   _tn_wait_queue_notify_deleted(&(dque->wait_send_list));
   _tn_wait_queue_notify_deleted(&(dque->wait_receive_list));

   dque->id_dque = 0; //-- data queue does not exist now

   tn_enable_interrupt();

   //-- we might need to switch context if _tn_wait_queue_notify_deleted()
   //   has woken up some high-priority task
   _tn_switch_context_if_needed();

   return TERR_NO_ERR;

}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_Retval tn_queue_send(
      struct TN_DQueue *dque,
      void *p_data,
      unsigned long timeout
      )
{
   return _dqueue_job_perform(dque, _JOB_TYPE__SEND, p_data, timeout);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_Retval tn_queue_send_polling(struct TN_DQueue *dque, void *p_data)
{
   return _dqueue_job_perform(dque, _JOB_TYPE__SEND, p_data, 0);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_Retval tn_queue_isend_polling(struct TN_DQueue *dque, void *p_data)
{
   return _dqueue_job_iperform(dque, _JOB_TYPE__SEND, p_data);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_Retval tn_queue_receive(
      struct TN_DQueue *dque,
      void **pp_data,
      unsigned long timeout
      )
{
   return _dqueue_job_perform(dque, _JOB_TYPE__RECEIVE, pp_data, timeout);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_Retval tn_queue_receive_polling(struct TN_DQueue *dque, void **pp_data)
{
   return _dqueue_job_perform(dque, _JOB_TYPE__RECEIVE, pp_data, 0);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_Retval tn_queue_ireceive(struct TN_DQueue *dque, void **pp_data)
{
   return _dqueue_job_iperform(dque, _JOB_TYPE__RECEIVE, pp_data);
}



