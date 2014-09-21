/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _TN_SEM_H
#define _TN_SEM_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_Sem {
   struct TN_ListItem  wait_queue;
   int count;
   int max_count;
   enum TN_ObjId id_sem;     //-- ID for verification(is it a semaphore or another object?)
                   // All semaphores have the same id_sem magic number (ver 2.x)
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

enum TN_Retval tn_sem_create(struct TN_Sem * sem, int start_count, int max_count);
enum TN_Retval tn_sem_delete(struct TN_Sem * sem);
enum TN_Retval tn_sem_signal(struct TN_Sem * sem);
enum TN_Retval tn_sem_isignal(struct TN_Sem * sem);
enum TN_Retval tn_sem_acquire(struct TN_Sem * sem, unsigned long timeout);
enum TN_Retval tn_sem_polling(struct TN_Sem * sem);
enum TN_Retval tn_sem_ipolling(struct TN_Sem * sem);


#endif // _TN_SEM_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


