
/*******************************************************************************
 *    Internal TNKernel header
 *
 ******************************************************************************/


#ifndef _TN_INTERNAL_H
#define _TN_INTERNAL_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

struct TN_Task;
struct TN_Mutex;
struct TN_ListItem;


/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

//-- system
/**
 * Note: this function sleeps if there is at least one element in wait_queue.
 */
void _tn_wait_queue_notify_deleted(struct TN_ListItem *wait_queue, TN_INTSAVE_DATA_ARG_DEC);


//-- tn_tasks.c
enum TN_Retval  _tn_task_create(struct TN_Task *task,                 //-- task TCB
      void (*task_func)(void *param),  //-- task function
      int priority,                    //-- task priority
      unsigned int *task_stack_bottom, //-- task stack first addr in memory (bottom)
      int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
      void *param,                     //-- task function parameter
      int option);                     //-- Creation option

enum TN_Retval   _tn_task_wait_complete  (struct TN_Task *task);
void  _tn_task_to_runnable    (struct TN_Task *task);
void  _tn_task_to_non_runnable(struct TN_Task *task);

void _tn_task_curr_to_wait_action(
      struct TN_ListItem *wait_que,
      enum TN_WaitReason wait_reason,
      unsigned long timeout);
enum TN_Retval  _tn_change_running_task_priority(struct TN_Task *task, int new_priority);

#ifdef TN_USE_MUTEXES
void _tn_set_current_priority(struct TN_Task *task, int priority);
#endif


//-- tn_mutex.h

/**
 * Iterate through all the tasks that wait for lock mutex,
 * checking if task's priority is higher than ref_priority.
 *
 * Max priority (i.e. lowest value) is returned.
 */
int  _tn_find_max_blocked_priority(struct TN_Mutex *mutex, int ref_priority);
BOOL _tn_do_unlock_mutex(struct TN_Mutex *mutex);

#endif // _TN_INTERNAL_H


