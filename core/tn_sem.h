/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _TN_SEM_H
#define _TN_SEM_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_utils.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct tn_sem {
   struct tn_que_head  wait_queue;
   int count;
   int max_count;
   int id_sem;     //-- ID for verification(is it a semaphore or another object?)
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

int tn_sem_create(struct tn_sem * sem, int start_value, int max_val);
int tn_sem_delete(struct tn_sem * sem);
int tn_sem_signal(struct tn_sem * sem);
int tn_sem_isignal(struct tn_sem * sem);
int tn_sem_acquire(struct tn_sem * sem, unsigned long timeout);
int tn_sem_polling(struct tn_sem * sem);
int tn_sem_ipolling(struct tn_sem * sem);


#endif // _TN_SEM_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


