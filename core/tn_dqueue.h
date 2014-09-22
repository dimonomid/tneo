/*******************************************************************************
 *   Description:   TODO
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
   struct TN_ListItem  wait_send_list;
   struct TN_ListItem  wait_receive_list;

   void         **data_fifo;        //-- array of (void *) to store 
                                    //   items data queue. Can be NULL.
   int            items_cnt;        //-- capacity (total items count). Can be 0.
   int            filled_items_cnt; //-- written items count
   int            head_idx;         //-- head item index in data_fifo
   int            tail_idx;         //-- tail item index in data_fifo
   enum TN_ObjId  id_dque;          //-- id for verification
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
 * Construct data queue. id_dque member should not contain TN_ID_DATAQUEUE,
 * otherwise, TERR_WRONG_PARAM is returned.
 *
 * @param dque       pointer to already allocated struct TN_DQueue.
 * @param data_fifo  pointer to already allocated array of void * to store
 *                   data queue items. Can be NULL.
 * @param items_cnt  capacity of queue
 *                   (count of elements in the data_fifo array)
 *                   Can be 0.
 */
enum TN_Retval tn_queue_create(
      struct TN_DQueue *dque,
      void **data_fifo,
      int items_cnt
      );

enum TN_Retval tn_queue_delete(struct TN_DQueue *dque);

enum TN_Retval tn_queue_send(
      struct TN_DQueue *dque,
      void *data_ptr,
      unsigned long timeout
      );

enum TN_Retval tn_queue_send_polling(
      struct TN_DQueue *dque,
      void *data_ptr
      );

enum TN_Retval tn_queue_isend_polling(
      struct TN_DQueue *dque,
      void *data_ptr
      );

enum TN_Retval tn_queue_receive(
      struct TN_DQueue *dque,
      void **data_ptr,
      unsigned long timeout
      );

enum TN_Retval tn_queue_receive_polling(
      struct TN_DQueue *dque,
      void **data_ptr
      );

enum TN_Retval tn_queue_ireceive(
      struct TN_DQueue *dque,
      void **data_ptr
      );


#endif // _TN_DQUEUE_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


