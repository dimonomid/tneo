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

typedef struct _TN_MUTEX {
   struct TNQueueHead wait_queue;        //-- List of tasks that wait a mutex
   struct TNQueueHead mutex_queue;       //-- To include in task's locked mutexes list (if any)
   struct TNQueueHead lock_mutex_queue;  //-- To include in system's locked mutexes list
   int attr;                     //-- Mutex creation attr - CEILING or INHERIT

   struct _TN_TCB *holder;       //-- Current mutex owner(task that locked mutex)
   int ceil_priority;            //-- When mutex created with CEILING attr
   int cnt;                      //-- Lock count
   int id_mutex;                 //-- ID for verification(is it a mutex or another object?)
                     // All mutexes have the same id_mutex magic number (ver 2.x)
} TN_MUTEX;

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define  TN_MUTEX_ATTR_CEILING           1
#define  TN_MUTEX_ATTR_INHERIT           2


#define get_mutex_by_mutex_queque(que)              \
   (que ? CONTAINING_RECORD(que, TN_MUTEX, mutex_queue) : 0)

#define get_mutex_by_wait_queque(que)               \
   (que ? CONTAINING_RECORD(que, TN_MUTEX, wait_queue) : 0)

#define get_mutex_by_lock_mutex_queque(que) \
   (que ? CONTAINING_RECORD(que, TN_MUTEX, mutex_queue) : 0)




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

int tn_mutex_create(TN_MUTEX * mutex,
                    int attribute,
                    int ceil_priority);
int tn_mutex_delete(TN_MUTEX * mutex);
int tn_mutex_lock(TN_MUTEX * mutex, unsigned long timeout);
int tn_mutex_lock_polling(TN_MUTEX * mutex);
int tn_mutex_unlock(TN_MUTEX * mutex);


#endif // _TN_MUTEX_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


