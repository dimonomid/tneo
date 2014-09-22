/**
 * \file
 *
 * A data queue is a FIFO that stores pointer (of type void*) in each cell,
 * called (in μITRON style) a data element. A data queue also has an associated
 * wait queue each for sending (wait_send queue) and for receiving
 * (wait_receive queue).  A task that sends a data element is tried to put the
 * data element into the FIFO. If there is no space left in the FIFO, the task
 * is switched to the WAITING state and placed in the data queue's wait_send
 * queue until space appears (another task gets a data element from the data
 * queue).  A task that receives a data element tries to get a data element
 * from the FIFO. If the FIFO is empty (there is no data in the data queue),
 * the task is switched to the WAITING state and placed in the data queue's
 * wait_receive queue until data element arrive (another task puts some data
 * element into the data queue).  To use a data queue just for the synchronous
 * message passing, set size of the FIFO to 0.  The data element to be sent and
 * received can be interpreted as a pointer or an integer and may have value 0
 * (NULL). 
 *   
 *
 ******************************************************************************/

#ifndef _TN_DQUEUE_H
#define _TN_DQUEUE_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_DQueue {

   /// list of tasks waiting to send data
   struct TN_ListItem  wait_send_list;
   /// list of tasks waiting to receive data
   struct TN_ListItem  wait_receive_list;
   /// array of (void *) to store data queue items. Can be NULL.
   void         **data_fifo;
   /// capacity (total items count). Can be 0.
   int            items_cnt;
   /// count of non-free items in data_fifo
   int            filled_items_cnt;
   /// index of the item which will be written next time
   int            head_idx;
   /// index of the item which will be read next time
   int            tail_idx;
   /// id for object validity verification
   enum TN_ObjId  id_dque;
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
 * otherwise, `TERR_WRONG_PARAM` is returned.
 *
 * @param dque       pointer to already allocated struct TN_DQueue.
 * @param data_fifo  pointer to already allocated array of `void *` to store
 *                   data queue items. Can be NULL.
 * @param items_cnt  capacity of queue
 *                   (count of elements in the `data_fifo` array)
 *                   Can be 0.
 */
enum TN_Retval tn_queue_create(
      struct TN_DQueue *dque,
      void **data_fifo,
      int items_cnt
      );


/**
 * Destruct data queue.
 *
 * All tasks that wait for writing to or reading from the queue become
 * runnable with `TERR_DLT` code returned.
 *
 * @param dque       pointer to data queue to be deleted
 */
enum TN_Retval tn_queue_delete(struct TN_DQueue *dque);


/**
 * Send the data element specified by the `data_ptr` to the data queue
 * specified by the `dque`.
 *
 * If there are tasks in the data queue's `wait_receive` list already, the
 * function releases the task from the head of the `wait_receive` list, makes
 * this task runnable and transfers the parameter `data_ptr` to task's
 * function, that caused it to wait.
 *
 * If there are no tasks in the data queue's `wait_receive` list, parameter
 * `data_ptr` is placed to the tail of data FIFO. If the data FIFO is full,
 * behavior depends on the `timeout` value:
 *
 * * `0`:                  `TERR_TIMEOUT` is returned immediately;
 * * `TN_WAIT_INFINITE`:   task is switched to waiting state until
 *                         there is empty element in the FIFO;
 * * other:                task is switched to waiting state until
 *                         there is empty element in the FIFO or until
 *                         specified timeout expires.
 *
 * @param dque       pointer to data queue to send data to
 * @param data_ptr   value to send
 * @param timeout    timeout after which `TERR_TIMEOUT` is returned if queue
 *                   is full
 *
 * @return  
 * * `TERR_NO_ERR`   if data was successfully sent;
 * * `TERR_TIMEOUT`  if there was a timeout.
 * * `TERR_FORCED`   if task was released from wait forcibly by calling
 *                      `tn_task_release_wait()`
 */
enum TN_Retval tn_queue_send(
      struct TN_DQueue *dque,
      void *data_ptr,
      unsigned long timeout
      );

/**
 * The same as `tn_queue_send()` with zero timeout
 */
enum TN_Retval tn_queue_send_polling(
      struct TN_DQueue *dque,
      void *data_ptr
      );

/**
 * The same as `tn_queue_send()` with zero timeout, but for using in the ISR.
 */
enum TN_Retval tn_queue_isend_polling(
      struct TN_DQueue *dque,
      void *data_ptr
      );

/**
 * Receive the data element from the data queue specified by the `dque` and
 * places it into the address specified by the `data_ptr`.  If the FIFO already
 * has data, function removes an entry from the end of the data queue FIFO and
 * returns it into the `data_ptr` function parameter.
 *
 * If there are task(s) in the data queue's `wait_send` list, they are removed
 * from the head of `wait_send` list, makes this task runnable and puts the
 * data entry, stored in this task, to the tail of data FIFO.  If there are no
 * entries in the data FIFO and there are no tasks in the wait_send list,
 * behavior depends on the `timeout` value:
 *
 * * `0`:                  `TERR_TIMEOUT` is returned immediately;
 * * `TN_WAIT_INFINITE`:   task is switched to waiting state
 *                         until new data comes
 * * other:                task is switched to waiting state until new data 
 *                         comes or until specified timeout expires.
 *
 * @param dque       pointer to data queue to receive data from
 * @param data_ptr   pointer to location to store the value
 * @param timeout    timeout after which `TERR_TIMEOUT` is returned if queue
 *                   is empty
 *
 * @return  
 * * `TERR_NO_ERR`   if data was successfully received;
 * * `TERR_TIMEOUT`  if there was a timeout.
 * * `TERR_FORCED`   if task was released from wait forcibly by calling
 *                   `tn_task_release_wait()`
 */
enum TN_Retval tn_queue_receive(
      struct TN_DQueue *dque,
      void **data_ptr,
      unsigned long timeout
      );

/**
 * The same as `tn_queue_receive()` with zero timeout
 */
enum TN_Retval tn_queue_receive_polling(
      struct TN_DQueue *dque,
      void **data_ptr
      );

/**
 * The same as `tn_queue_receive()` with zero timeout, but for using in the ISR.
 */
enum TN_Retval tn_queue_ireceive(
      struct TN_DQueue *dque,
      void **data_ptr
      );


#endif // _TN_DQUEUE_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


