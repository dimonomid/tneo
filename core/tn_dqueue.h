/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _TN_DQUEUE_H
#define _TN_DQUEUE_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_utils.h"
#include "tn_common.h"

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_DQueue {
   struct TN_ListItem  wait_send_list;
   struct TN_ListItem  wait_receive_list;

   void ** data_fifo;   //-- Array of void* to store data queue entries
   int  num_entries;    //-- Capacity of data_fifo(num entries)
   int  tail_cnt;       //-- Counter to processing data queue's Array of void*
   int  header_cnt;     //-- Counter to processing data queue's Array of void*
   enum TN_ObjId  id_dque;        //-- ID for verification(is it a data queue or another object?)
   // All data queues have the same id_dque magic number (ver 2.x)
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

enum TN_Retval tn_queue_create(struct TN_DQueue * dque,
                               void ** data_fifo,
                               int num_entries);
enum TN_Retval tn_queue_delete(struct TN_DQueue * dque);
enum TN_Retval tn_queue_send(struct TN_DQueue * dque, void * data_ptr, unsigned long timeout);
enum TN_Retval tn_queue_send_polling(struct TN_DQueue * dque, void * data_ptr);
enum TN_Retval tn_queue_isend_polling(struct TN_DQueue * dque, void * data_ptr);
enum TN_Retval tn_queue_receive(struct TN_DQueue * dque, void ** data_ptr, unsigned long timeout);
enum TN_Retval tn_queue_receive_polling(struct TN_DQueue * dque, void ** data_ptr);
enum TN_Retval tn_queue_ireceive(struct TN_DQueue * dque, void ** data_ptr);


#endif // _TN_DQUEUE_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


