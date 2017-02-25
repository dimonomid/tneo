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

#include "tn_common.h"
#include "tn_sys.h"

//-- internal tnkernel headers
#include "_tn_eventgrp.h"
#include "_tn_tasks.h"
#include "_tn_list.h"


#include "tn_dqueue.h"
#include "_tn_dqueue.h"

#include "tn_tasks.h"




/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

/**
 * Type of job: send data or receive data. Given to `_dqueue_job_perform()`
 * and `_dqueue_job_iperform()`.
 */
enum _JobType {
   _JOB_TYPE__SEND,
   _JOB_TYPE__RECEIVE,
};



/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
_TN_STATIC_INLINE enum TN_RCode _check_param_generic(
      const struct TN_DQueue *dque
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (dque == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (!_tn_dqueue_is_valid(dque)){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _check_param_create(
      const struct TN_DQueue *dque,
      void **data_fifo,
      int items_cnt
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (dque == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (items_cnt < 0 || _tn_dqueue_is_valid(dque)){
      rc = TN_RC_WPARAM;
   }

   _TN_UNUSED(data_fifo);

   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _check_param_read(
      void **pp_data
      )
{
   return (pp_data == TN_NULL) ? TN_RC_WPARAM : TN_RC_OK;
}

#else
#  define _check_param_generic(dque)                        (TN_RC_OK)
#  define _check_param_create(dque, data_fifo, items_cnt)   (TN_RC_OK)
#  define _check_param_read(pp_data)                        (TN_RC_OK)
#endif
// }}}

//-- Data queue storage FIFO processing {{{

/**
 * Try to put data to the FIFO.
 *
 * If there is a room in the FIFO, data is written, and `#TN_RC_OK` is
 * returned; otherwise, `#TN_RC_TIMEOUT` is returned, and this case can 
 * be handled by the caller.
 *
 * @param dque
 *    Data queue in which data should be written
 * @param p_data
 *    Data to write (just a pointer itself is written to the FIFO, not the data
 *    which is pointed to by `p_data`)
 */
static enum TN_RCode _fifo_write(struct TN_DQueue *dque, void *p_data)
{
   enum TN_RCode rc = TN_RC_OK;

   if (dque->filled_items_cnt >= dque->items_cnt){
      //-- no space for new data
      rc = TN_RC_TIMEOUT;
   } else {

      //-- write data
      dque->data_fifo[dque->head_idx] = p_data;
      dque->filled_items_cnt++;
      dque->head_idx++;
      if (dque->head_idx >= dque->items_cnt){
         dque->head_idx = 0;
      }

      //-- set flag in the connected event group (if any),
      //   indicating that there are messages in the queue
      _tn_eventgrp_link_manage(&dque->eventgrp_link, TN_TRUE);
   }

   return rc;
}


/**
 * Try to read data from the FIFO.
 *
 * If there is some data in the FIFO, data is read, and `#TN_RC_OK` is
 * returned; otherwise, `#TN_RC_TIMEOUT` is returned, and this case can 
 * be handled by the caller.
 *
 * @param dque
 *    Data queue from which data should be read
 * @param p_data
 *    Pointer to the place at which new data should be read.
 */
static enum TN_RCode _fifo_read(struct TN_DQueue *dque, void **pp_data)
{
   enum TN_RCode rc = _check_param_read(pp_data);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (dque->filled_items_cnt == 0){
      //-- nothing to read
      rc = TN_RC_TIMEOUT;
   } else {

      //-- read data
      *pp_data = dque->data_fifo[dque->tail_idx];
      dque->filled_items_cnt--;
      dque->tail_idx++;
      if (dque->tail_idx >= dque->items_cnt){
         dque->tail_idx = 0;
      }

      if (dque->filled_items_cnt == 0){
         //-- clear flag in the connected event group (if any),
         //   indicating that there are no messages in the queue
         _tn_eventgrp_link_manage(&dque->eventgrp_link, TN_FALSE);
      }
   }

   return rc;
}
// }}}

/**
 * Callback function that is given to `_tn_task_first_wait_complete()`
 * when task finishes waiting for new messages in the queue.
 *
 * See `#_TN_CBBeforeTaskWaitComplete` for details on function signature.
 */
static void _cb_before_task_wait_complete__send(
      struct TN_Task   *task,
      void             *user_data_1,
      void             *user_data_2
      )
{
   //-- before task is woken up, set data that it is waiting for
   task->subsys_wait.dqueue.data_elem = user_data_1;
   _TN_UNUSED(user_data_2);
}

/**
 * Callback function that is given to `_tn_task_first_wait_complete()`
 * when task finishes waiting for free item in the queue.
 *
 * See `#_TN_CBBeforeTaskWaitComplete` for details on function signature.
 */
static void _cb_before_task_wait_complete__receive_ok(
      struct TN_Task   *task,
      void             *user_data_1,
      void             *user_data_2
      )
{
   struct TN_DQueue *dque = (struct TN_DQueue *)user_data_1;

   //-- put to data FIFO
   enum TN_RCode rc = _fifo_write(dque, task->subsys_wait.dqueue.data_elem); 
   if (rc != TN_RC_OK){
      _TN_FATAL_ERROR("rc should always be TN_RC_OK here");
   }
   _TN_UNUSED(user_data_2);
}

/**
 * Callback function that is given to `_tn_task_first_wait_complete()`
 * when `items_cnt` is 0.
 *
 * See `#_TN_CBBeforeTaskWaitComplete` for details on function signature.
 */
static void _cb_before_task_wait_complete__receive_timeout(
      struct TN_Task   *task,
      void             *user_data_1,
      void             *user_data_2
      )
{
   // (that might happen if only dque->items_cnt is 0)

   void **pp_data = (void **)user_data_1;

   *pp_data = task->subsys_wait.dqueue.data_elem; //-- Return to caller
   _TN_UNUSED(user_data_2);
}


/**
 * Actual worker function that sends new data through the queue. Eventually
 * called when user calls one of these functions:
 *
 * - `tn_queue_send()`
 * - `tn_queue_send_polling()`
 * - `tn_queue_isend_polling()`
 *
 *
 * First of all, it checks whether there are tasks that wait for new data. If
 * so, the data is given to that task, and task is woken up. FIFO stays
 * untouched.
 *
 * Otherwise, it calls `_fifo_write()` which tries to put data to the FIFO.
 * If there is a room in the FIFO, data is written, and `#TN_RC_OK` is
 * returned; otherwise, `#TN_RC_TIMEOUT` is returned, and this case is 
 * probably handled by the caller (`_dqueue_job_perform()` or
 * `_dqueue_job_iperform()`) depending on requested `timeout` value.
 *
 * @param dque
 *    Data queue in which data should be written
 * @param p_data
 *    Data to write (just a pointer itself is written to the FIFO, not the data
 *    which is pointed to by `p_data`)
 */
static enum TN_RCode _queue_send(
      struct TN_DQueue *dque,
      void *p_data
      )
{
   enum TN_RCode rc = TN_RC_OK;

   //-- first of all, we check whether there are task(s) that
   //   waits for receive message from the queue.
   //
   //   If yes, we just pass new message to the first task
   //   from the waiting tasks list, and don't modify messages
   //   fifo at all.
   //
   //   Otherwise (no waiting tasks), we add new message to the fifo.

   if (  !_tn_task_first_wait_complete(
            &dque->wait_receive_list, TN_RC_OK,
            _cb_before_task_wait_complete__send, p_data, TN_NULL
            )
      )
   {
      //-- the data queue's wait_receive list is empty
      rc = _fifo_write(dque, p_data);
   }

   return rc;
}

/**
 * Actual worker function that receives data from the queue. 
 * Eventually called when user calls one of these functions:
 *
 * - `tn_queue_receive()`
 * - `tn_queue_receive_polling()`
 * - `tn_queue_ireceive_polling()`
 *
 * First of all, it tries to read data from the queue by calling
 * `_fifo_read()`. In case of success, it checks whether there are tasks that
 * wait for the free space in the queue, and wakes up the first task, if any.
 *
 * Otherwise (queue is empty, so, read is failed), it checks for the rare case
 * if there are tasks that wait to write to the queue. It may happen if only
 * `items_cnt` is 0. If there are such tasks, data is received from the first
 * task from the queue. Otherwise, `#TN_RC_TIMEOUT` is returned, and this can
 * be handled by the caller (`_dqueue_job_perform()` or
 * `_dqueue_job_iperform()`) depending on requested `timeout` value.
 *
 * @param dque
 *    Data queue from which data should be read
 * @param p_data
 *    Pointer to the place at which new data should be read.
 */
static enum TN_RCode _queue_receive(
      struct TN_DQueue *dque,
      void **pp_data
      )
{
   enum TN_RCode rc = TN_RC_OK;

   //-- try to read data from the queue
   rc = _fifo_read(dque, pp_data);

   switch (rc){
      case TN_RC_OK:
         //-- successfully read item from the queue.
         //   if there are tasks that wait to send data to the queue,
         //   wake the first one up, since there is room now.
         _tn_task_first_wait_complete(
               &dque->wait_send_list, TN_RC_OK,
               _cb_before_task_wait_complete__receive_ok, dque, TN_NULL
               );
         break;

      case TN_RC_TIMEOUT:
         //-- nothing to read from the queue.
         //   Let's check whether some task wants to send data
         //   (that might happen if only dque->items_cnt is 0)
         if (  _tn_task_first_wait_complete(
                  &dque->wait_send_list, TN_RC_OK,
                  _cb_before_task_wait_complete__receive_timeout, pp_data, TN_NULL
                  )
            )
         {
            //-- that might happen if only dque->items_cnt is 0:
            //   data was read to `pp_data` in the 
            //   `_cb_before_task_wait_complete__receive_timeout()`
            rc = TN_RC_OK;
         }
         break;

      case TN_RC_WPARAM:
         //-- do nothing, just return this error
         break;

      default:
         _TN_FATAL_ERROR(
               "rc should be TN_RC_OK, TN_RC_TIMEOUT or TN_RC_WPARAM here"
               );
         break;
   }

   return rc;
}


/**
 * Intermediary function that is called by queue-related services
 * (`tn_queue_send()`, `tn_queue_receive()`, etc), which performs all necessary
 * housekeeping and eventually calls actual worker function depending on given
 * `job_type`.
 *
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param dque
 *    Data queue on which job should be performed.
 * @param job_type
 *    Type of job to perform, depending on it, appropriate worker function
 *    will be called (`_queue_send()` or `_queue_receive()`).
 * @param p_data
 *    Depends on given job_type:
 *
 *    - `_JOB_TYPE__SEND`: data to send;
 *    - `_JOB_TYPE__RECEIVE`: pointer at which data should be received.
 * @param timeout
 *    Refer to `#TN_TickCnt`.
 */
static enum TN_RCode _dqueue_job_perform(
      struct TN_DQueue *dque,
      enum _JobType job_type,
      void *p_data,
      TN_TickCnt timeout
      )
{
   TN_BOOL waited = TN_FALSE;
   void **pp_data = (void **)p_data;
   enum TN_RCode rc = _check_param_generic(dque);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      switch (job_type){

         case _JOB_TYPE__SEND:
            //-- try to put new item to the queue
            rc = _queue_send(dque, p_data);

            if (rc == TN_RC_TIMEOUT && timeout != 0){
               //-- We can't put new item to the queue right now (queue is
               //   full), and user asked to wait if that happens.
               //
               //   Save user-provided data in the `dqueue.data_elem` task
               //   field, and put current task to wait until there's room in
               //   the queue.
               _tn_curr_run_task->subsys_wait.dqueue.data_elem = p_data;
               _tn_task_curr_to_wait_action(
                     &(dque->wait_send_list),
                     TN_WAIT_REASON_DQUE_WSEND,
                     timeout
                     );

               waited = TN_TRUE;
            }
            break;

         case _JOB_TYPE__RECEIVE:
            //-- try to get the item from the queue
            rc = _queue_receive(dque, pp_data);

            if (rc == TN_RC_TIMEOUT && timeout != 0){
               //-- Queue is empty right now, and user asked to wait if that
               //   happens.
               //
               //   Put current task to wait until new data comes.
               _tn_task_curr_to_wait_action(
                     &(dque->wait_receive_list),
                     TN_WAIT_REASON_DQUE_WRECEIVE,
                     timeout
                     );

               waited = TN_TRUE;
            }
            break;
      }

#if TN_DEBUG
      if (!_tn_need_context_switch() && waited){
         _TN_FATAL_ERROR("");
      }
#endif

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();
      if (waited){

         //-- get wait result
         rc = _tn_curr_run_task->task_wait_rc;

         switch (job_type){
            case _JOB_TYPE__SEND:
               //-- do nothing special
               break;
            case _JOB_TYPE__RECEIVE:
               //-- if wait result is TN_RC_OK, copy received pointer to the
               //   user's location
               if (rc == TN_RC_OK){
                  //-- dqueue.data_elem should contain valid value now,
                  //   return it to caller
                  *pp_data = _tn_curr_run_task->subsys_wait.dqueue.data_elem;
               }
               break;
         }
      }

   }
   return rc;
}

/**
 * The same as `_dqueue_job_perform()` with zero timeout, but for using in the
 * ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
static enum TN_RCode _dqueue_job_iperform(
      struct TN_DQueue *dque,
      enum _JobType job_type,
      void *p_data      //-- used for _JOB_TYPE__SEND
      )
{
   void **pp_data = (void **)p_data;
   enum TN_RCode rc = _check_param_generic(dque);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_isr_context()){
      //-- wrong context
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA_INT;

      TN_INT_IDIS_SAVE();

      //-- depending on the job type, call appropriate function
      switch (job_type){

         case _JOB_TYPE__SEND:
            //-- Try to put new item to the queue. We don't handle returned
            //   value here, since we can't wait in interrupt, so, just return
            //   the value to the caller.
            rc = _queue_send(dque, p_data);
            break;

         case _JOB_TYPE__RECEIVE:
            //-- try to get the item from the queue. We don't handle returned
            //   value here, since we can't wait in interrupt, so, just return
            //   the value to the caller.
            rc = _queue_receive(dque, pp_data);
            break;
      }

      TN_INT_IRESTORE();
      _TN_CONTEXT_SWITCH_IPEND_IF_NEEDED();
   }

   return rc;
}





/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_create(
      struct TN_DQueue *dque,
      void **data_fifo,
      int items_cnt
      )
{
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_create(dque, data_fifo, items_cnt);
   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      _tn_list_reset(&(dque->wait_send_list));
      _tn_list_reset(&(dque->wait_receive_list));

      dque->data_fifo         = data_fifo;
      dque->items_cnt         = items_cnt;

      _tn_eventgrp_link_reset(&dque->eventgrp_link);

      if (dque->data_fifo == TN_NULL){
         dque->items_cnt = 0;
      }

      dque->filled_items_cnt  = 0;
      dque->tail_idx          = 0;
      dque->head_idx          = 0;

      dque->id_dque = TN_ID_DATAQUEUE;
   }

   return rc;
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_delete(struct TN_DQueue * dque)
{
   enum TN_RCode rc = TN_RC_OK;

   rc = _check_param_generic(dque);
   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      //-- notify waiting tasks that the object is deleted
      //   (TN_RC_DELETED is returned)
      _tn_wait_queue_notify_deleted(&(dque->wait_send_list));
      _tn_wait_queue_notify_deleted(&(dque->wait_receive_list));

      dque->id_dque = TN_ID_NONE; //-- data queue does not exist now

      TN_INT_RESTORE();

      //-- we might need to switch context if _tn_wait_queue_notify_deleted()
      //   has woken up some high-priority task
      _tn_context_switch_pend_if_needed();

   }

   return rc;

}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_send(
      struct TN_DQueue *dque,
      void *p_data,
      TN_TickCnt timeout
      )
{
   return _dqueue_job_perform(dque, _JOB_TYPE__SEND, p_data, timeout);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_send_polling(struct TN_DQueue *dque, void *p_data)
{
   return _dqueue_job_perform(dque, _JOB_TYPE__SEND, p_data, 0);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_isend_polling(struct TN_DQueue *dque, void *p_data)
{
   return _dqueue_job_iperform(dque, _JOB_TYPE__SEND, p_data);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_receive(
      struct TN_DQueue *dque,
      void **pp_data,
      TN_TickCnt timeout
      )
{
   return _dqueue_job_perform(dque, _JOB_TYPE__RECEIVE, pp_data, timeout);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_receive_polling(struct TN_DQueue *dque, void **pp_data)
{
   return _dqueue_job_perform(dque, _JOB_TYPE__RECEIVE, pp_data, 0);
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_ireceive_polling(struct TN_DQueue *dque, void **pp_data)
{
   return _dqueue_job_iperform(dque, _JOB_TYPE__RECEIVE, pp_data);
}

/*
 * See comments in the header file (tn_dqueue.h)
 */
int tn_queue_free_items_cnt_get(
      struct TN_DQueue    *dque
      )
{
   int ret = -1;
   enum TN_RCode rc = _check_param_generic(dque);

   if (rc == TN_RC_OK){
      //-- It's not needed to disable interrupts here, since `filled_items_cnt`
      //   is read by just one assembler instruction, and `items_cnt` never
      //   changes.
      ret = dque->items_cnt - dque->filled_items_cnt;
   }

   return ret;
}

/*
 * See comments in the header file (tn_dqueue.h)
 */
int tn_queue_used_items_cnt_get(
      struct TN_DQueue    *dque
      )
{
   int ret = -1;
   enum TN_RCode rc = _check_param_generic(dque);

   if (rc == TN_RC_OK){
      //-- It's not needed to disable interrupts here, since `filled_items_cnt`
      //   is read by just one assembler instruction.
      ret = dque->filled_items_cnt;
   }

   return ret;
}

/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_eventgrp_connect(
      struct TN_DQueue    *dque,
      struct TN_EventGrp  *eventgrp,
      TN_UWord             pattern
      )
{
   int sr_saved;
   enum TN_RCode rc = _check_param_generic(dque);

   if (rc == TN_RC_OK){
      sr_saved = tn_arch_sr_save_int_dis();
      rc = _tn_eventgrp_link_set(&dque->eventgrp_link, eventgrp, pattern);
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_queue_eventgrp_disconnect(
      struct TN_DQueue    *dque
      )
{
   int sr_saved;
   enum TN_RCode rc = _check_param_generic(dque);

   if (rc == TN_RC_OK){
      sr_saved = tn_arch_sr_save_int_dis();
      rc = _tn_eventgrp_link_reset(&dque->eventgrp_link);
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}


