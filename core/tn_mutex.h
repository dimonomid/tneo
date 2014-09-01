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
#include "tn_common.h"


/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_Mutex {
   struct TN_ListItem wait_queue;        //-- List of tasks that wait a mutex
   struct TN_ListItem mutex_queue;       //-- To include in task's locked mutexes list (if any)
   struct TN_ListItem lock_mutex_queue;  //-- To include in system's locked mutexes list
   int attr;                     //-- Mutex creation attr - CEILING or INHERIT

   struct TN_Task *holder;       //-- Current mutex owner(task that locked mutex)
   int ceil_priority;            //-- When mutex created with CEILING attr
   int cnt;                      //-- Lock count
   enum TN_ObjId id_mutex;                 //-- ID for verification(is it a mutex or another object?)
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
   (que ? CONTAINING_RECORD(que, struct TN_Mutex, mutex_queue) : 0)

#define get_mutex_by_wait_queque(que)               \
   (que ? CONTAINING_RECORD(que, struct TN_Mutex, wait_queue) : 0)

#define get_mutex_by_lock_mutex_queque(que) \
   (que ? CONTAINING_RECORD(que, struct TN_Mutex, mutex_queue) : 0)




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

enum TN_Retval tn_mutex_create(struct TN_Mutex * mutex,
                    int attribute,
                    int ceil_priority);
enum TN_Retval tn_mutex_delete(struct TN_Mutex * mutex);
enum TN_Retval tn_mutex_lock(struct TN_Mutex * mutex, unsigned long timeout);
enum TN_Retval tn_mutex_lock_polling(struct TN_Mutex * mutex);
enum TN_Retval tn_mutex_unlock(struct TN_Mutex * mutex);


#endif // _TN_MUTEX_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


