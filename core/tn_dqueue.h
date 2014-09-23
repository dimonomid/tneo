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

/**
 * \file
 *
 * A data queue is a FIFO that stores pointer (of type `void *`) in each cell,
 * called (in μITRON style) a data element. A data queue also has an associated
 * wait queue each for sending (`wait_send` queue) and for receiving
 * (`wait_receive` queue).  A task that sends a data element tries to put the
 * data element into the FIFO. If there is no space left in the FIFO, the task
 * is switched to the waiting state and placed in the data queue's `wait_send`
 * queue until space appears (another task gets a data element from the data
 * queue).  A task that receives a data element tries to get a data element
 * from the FIFO. If the FIFO is empty (there is no data in the data queue),
 * the task is switched to the waiting state and placed in the data queue's
 * `wait_receive` queue until data element arrive (another task puts some data
 * element into the data queue).  To use a data queue just for the synchronous
 * message passing, set size of the FIFO to 0.  The data element to be sent and
 * received can be interpreted as a pointer or an integer and may have value 0
 * (`NULL`). 
 *   
 */

#ifndef _TN_DQUEUE_H
#define _TN_DQUEUE_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"





#ifdef __cplusplus
extern "C"  {  /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Structure representing data queue object
 */
struct TN_DQueue {
   ///
   /// list of tasks waiting to send data
   struct TN_ListItem  wait_send_list;
   ///
   /// list of tasks waiting to receive data
   struct TN_ListItem  wait_receive_list;

   ///
   /// array of `void *` to store data queue items. Can be `NULL`.
   void         **data_fifo;
   ///
   /// capacity (total items count). Can be 0.
   int            items_cnt;
   ///
   /// count of non-free items in `data_fifo`
   int            filled_items_cnt;
   ///
   /// index of the item which will be written next time
   int            head_idx;
   ///
   /// index of the item which will be read next time
   int            tail_idx;
   ///
   /// id for object validity verification
   enum TN_ObjId  id_dque;
};

/**
 * DQueue-specific fields for struct TN_Task.
 */
struct TN_DQueueTaskWait {
   /// if task tries to send the data to the data queue,
   /// and there's no space in the queue, value to put to queue is stored
   /// in this field
   void *data_elem;
};


/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Construct data queue. `id_dque` member should not contain `TN_ID_DATAQUEUE`,
 * otherwise, `TN_RC_WPARAM` is returned.
 *
 * @param dque       pointer to already allocated struct TN_DQueue.
 * @param data_fifo  pointer to already allocated array of `void *` to store
 *                   data queue items. Can be NULL.
 * @param items_cnt  capacity of queue
 *                   (count of elements in the `data_fifo` array)
 *                   Can be 0.
 */
enum TN_RCode tn_queue_create(
      struct TN_DQueue *dque,
      void **data_fifo,
      int items_cnt
      );


/**
 * Destruct data queue.
 *
 * All tasks that wait for writing to or reading from the queue become
 * runnable with `TN_RC_DELETED` code returned.
 *
 * @param dque       pointer to data queue to be deleted
 */
enum TN_RCode tn_queue_delete(struct TN_DQueue *dque);


/**
 * Send the data element specified by the `p_data` to the data queue
 * specified by the `dque`.
 *
 * If there are tasks in the data queue's `wait_receive` list already, the
 * function releases the task from the head of the `wait_receive` list, makes
 * this task runnable and transfers the parameter `p_data` to task's
 * function, that caused it to wait.
 *
 * If there are no tasks in the data queue's `wait_receive` list, parameter
 * `p_data` is placed to the tail of data FIFO. If the data FIFO is full,
 * behavior depends on the `timeout` value:
 *
 * * `0`:                  `TN_RC_TIMEOUT` is returned immediately;
 * * `TN_WAIT_INFINITE`:   task is switched to waiting state until
 *                         there is empty element in the FIFO;
 * * other:                task is switched to waiting state until
 *                         there is empty element in the FIFO or until
 *                         specified timeout expires.
 *
 * @param dque       pointer to data queue to send data to
 * @param p_data     value to send
 * @param timeout    timeout after which `TN_RC_TIMEOUT` is returned if queue
 *                   is full
 *
 * @return  
 * * `TN_RC_OK`   if data was successfully sent;
 * * `TN_RC_TIMEOUT`  if there was a timeout.
 * * `TN_RC_FORCED`   if task was released from wait forcibly by calling
 *                      `tn_task_release_wait()`
 */
enum TN_RCode tn_queue_send(
      struct TN_DQueue *dque,
      void *p_data,
      unsigned long timeout
      );

/**
 * The same as `tn_queue_send()` with zero timeout
 */
enum TN_RCode tn_queue_send_polling(
      struct TN_DQueue *dque,
      void *p_data
      );

/**
 * The same as `tn_queue_send()` with zero timeout, but for using in the ISR.
 */
enum TN_RCode tn_queue_isend_polling(
      struct TN_DQueue *dque,
      void *p_data
      );

/**
 * Receive the data element from the data queue specified by the `dque` and
 * place it into the address specified by the `pp_data`.  If the FIFO already
 * has data, function removes an entry from the end of the data queue FIFO and
 * returns it into the `pp_data` function parameter.
 *
 * If there are task(s) in the data queue's `wait_send` list, first one gets
 * removed from the head of `wait_send` list, becomes runnable and puts the
 * data entry, stored in this task, to the tail of data FIFO.  If there are no
 * entries in the data FIFO and there are no tasks in the wait_send list,
 * behavior depends on the `timeout` value:
 *
 * * `0`:                  `TN_RC_TIMEOUT` is returned immediately;
 * * `TN_WAIT_INFINITE`:   task is switched to waiting state
 *                         until new data comes
 * * other:                task is switched to waiting state until new data 
 *                         comes or until specified timeout expires.
 *
 * @param dque       pointer to data queue to receive data from
 * @param pp_data    pointer to location to store the value
 * @param timeout    timeout after which `TN_RC_TIMEOUT` is returned if queue
 *                   is empty
 *
 * @return  
 * * `TN_RC_OK`   if data was successfully received;
 * * `TN_RC_TIMEOUT`  if there was a timeout.
 * * `TN_RC_FORCED`   if task was released from wait forcibly by calling
 *                   `tn_task_release_wait()`
 */
enum TN_RCode tn_queue_receive(
      struct TN_DQueue *dque,
      void **pp_data,
      unsigned long timeout
      );

/**
 * The same as `tn_queue_receive()` with zero timeout
 */
enum TN_RCode tn_queue_receive_polling(
      struct TN_DQueue *dque,
      void **pp_data
      );

/**
 * The same as `tn_queue_receive()` with zero timeout, but for using in the ISR.
 */
enum TN_RCode tn_queue_ireceive(
      struct TN_DQueue *dque,
      void **pp_data
      );




#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_DQUEUE_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


