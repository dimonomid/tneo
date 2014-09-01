/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _TN_MUTEX_H
#define _TN_MUTEX_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_utils.h"


/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct tn_mutex {
   struct tn_que_head wait_queue;        //-- List of tasks that wait a mutex
   struct tn_que_head mutex_queue;       //-- To include in task's locked mutexes list (if any)
   struct tn_que_head lock_mutex_queue;  //-- To include in system's locked mutexes list
   int attr;                     //-- Mutex creation attr - CEILING or INHERIT

   struct tn_task *holder;       //-- Current mutex owner(task that locked mutex)
   int ceil_priority;            //-- When mutex created with CEILING attr
   int cnt;                      //-- Lock count
   int id_mutex;                 //-- ID for verification(is it a mutex or another object?)
                     // All mutexes have the same id_mutex magic number (ver 2.x)
};

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define  TN_MUTEX_ATTR_CEILING           1
#define  TN_MUTEX_ATTR_INHERIT           2


#define get_mutex_by_mutex_queque(que)              \
   (que ? CONTAINING_RECORD(que, struct tn_mutex, mutex_queue) : 0)

#define get_mutex_by_wait_queque(que)               \
   (que ? CONTAINING_RECORD(que, struct tn_mutex, wait_queue) : 0)

#define get_mutex_by_lock_mutex_queque(que) \
   (que ? CONTAINING_RECORD(que, struct tn_mutex, mutex_queue) : 0)




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

int tn_mutex_create(struct tn_mutex * mutex,
                    int attribute,
                    int ceil_priority);
int tn_mutex_delete(struct tn_mutex * mutex);
int tn_mutex_lock(struct tn_mutex * mutex, unsigned long timeout);
int tn_mutex_lock_polling(struct tn_mutex * mutex);
int tn_mutex_unlock(struct tn_mutex * mutex);


#endif // _TN_MUTEX_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


