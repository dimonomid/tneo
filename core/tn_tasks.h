/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _TN_TASKS_H
#define _TN_TASKS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_utils.h"
#include "tn_common.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_Task {
   unsigned int * task_stk;   //-- Pointer to task's top of stack
   struct TN_QueHead task_queue;     //-- Queue is used to include task in ready/wait lists
   struct TN_QueHead timer_queue;    //-- Queue is used to include task in timer(timeout,etc.) list
   struct TN_QueHead * pwait_queue;  //-- Ptr to object's(semaphor,event,etc.) wait list,
                                      // that task has been included for waiting (ver 2.x)
   struct TN_QueHead create_queue;   //-- Queue is used to include task in create list only

#ifdef TN_USE_MUTEXES

   struct TN_QueHead mutex_queue;    //-- List of all mutexes that tack locked  (ver 2.x)

#endif

   unsigned int * stk_start;  //-- Base address of task's stack space
   int   stk_size;            //-- Task's stack size (in sizeof(void*),not bytes)
   void * task_func_addr;     //-- filled on creation  (ver 2.x)
   void * task_func_param;    //-- filled on creation  (ver 2.x)

   int  base_priority;        //-- Task base priority  (ver 2.x)
   int  priority;             //-- Task current priority
   enum TN_ObjId  id_task;   //-- ID for verification(is it a task or another object?)
                              // All tasks have the same id_task magic number (ver 2.x)

   int  task_state;           //-- Task state
   int  task_wait_reason;     //-- Reason for waiting
   int  task_wait_rc;         //-- Waiting return code(reason why waiting  finished)
   unsigned long tick_count;  //-- Remaining time until timeout
   int  tslice_count;         //-- Time slice counter

#ifdef  TN_USE_EVENTS

   int  ewait_pattern;        //-- Event wait pattern
   int  ewait_mode;           //-- Event wait mode:  _AND or _OR

#endif

   void * data_elem;          //-- Store data queue entry,if data queue is full

   int  activate_count;       //-- Activation request count - for statistic
   int  wakeup_count;         //-- Wakeup request count - for statistic
   int  suspend_count;        //-- Suspension count - for statistic

// Other implementation specific fields may be added below

};

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

//-- options for tn_task_create()
#define  TN_TASK_START_ON_CREATION        1
#define  TN_TASK_TIMER                 0x80
#define  TN_TASK_IDLE                  0x40


//-- task states
#define  TSK_STATE_RUNNABLE            0x01
#define  TSK_STATE_WAIT                0x04
#define  TSK_STATE_SUSPEND             0x08
#define  TSK_STATE_WAITSUSP            (TSK_STATE_SUSPEND | TSK_STATE_WAIT)
#define  TSK_STATE_DORMANT             0x10


//-- attr for tn_task_exit()
#define  TN_EXIT_AND_DELETE_TASK          1


#define get_task_by_tsk_queue(que)                                   \
   (que ? CONTAINING_RECORD(que, struct TN_Task, task_queue) : 0)

#define get_task_by_timer_queque(que)                                \
   (que ? CONTAINING_RECORD(que, struct TN_Task, timer_queue) : 0)

#define get_task_by_block_queque(que)                                \
   (que ? CONTAINING_RECORD(que, struct TN_Task, block_queue) : 0)




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Create task.
 *
 * This function creates a task. A field id_task of the structure task must be set to 0 before invoking this
 * function. A memory for the task TCB and a task stack must be allocated before the function call. An
 * allocation may be static (global variables of the struct TN_Task type for the task and
 * unsigned int [task_stack_size] for the task stack) or dynamic, if the user application supports
 * malloc/alloc (TNKernel itself does not use dynamic memory allocation).
 * The task_stack_size value must to be chosen big enough to fit the task_func local variables and its switch
 * context (processor registers, stack and instruction pointers, etc.).
 * The task stack must to be created as an array of unsigned int. Actually, the size of stack array element
 * must be identical to the processor register size (for most 32-bits and 16-bits processors a register size
 * equals sizeof(int)).
 * A parameter task_stack_start must point to the stack bottom. For instance, if the processor stack grows
 * from the high to the low memory addresses and the task stack array is defined as
 * unsigned int xxx_xxx[task_stack_size] (in C-language notation),
 * then the task_stack_start parameter has to be &xxx_xxx[task_stack_size - 1].
 *
 * @param task       Ready-allocated struct TN_Task structure. A field id_task of it must be 
 *                   set to 0 before invocation of tn_task_create().
 * @param task_func  Task body function.
 * @param priority   Priority for new task. NOTE: the lower value, the higher priority.
 *                   Must be > 0 and < (TN_NUM_PRIORITY - 1).
 * @param task_stack_start    Task start pointer.
 *                            A task must be created as an array of int. Actually, the size
 *                            of stack array element must be identical to the processor 
 *                            register size (for most 32-bits and 16-bits processors
 *                            a register size equals sizeof(int)).
 *                            NOTE: See option TN_API_TASK_CREATE for further details.
 * @param task_stack_size     Size of task stack
 *                            (actually, size of array that is used for task_stack_start).
 * @param            Parameter that is passed to task_func.
 * @option           (0): task is created in DORMANT state,
 *                        you need to call tn_task_activate() after task creation.
 *                   (TN_TASK_START_ON_CREATION): task is created and activated.
 *                   
 */
int tn_task_create(struct TN_Task *task,                  //-- task TCB
                 void (*task_func)(void *param),  //-- task function
                 int priority,                    //-- task priority
                 unsigned int *task_stack_start,  //-- task stack first addr in memory (see option TN_API_TASK_CREATE)
                 int task_stack_size,             //-- task stack size (in sizeof(void*),not bytes)
                 void *param,                     //-- task function parameter
                 int option);                     //-- creation option


/**
 * If the task is runnable, it is moved to the SUSPENDED state. If the task
 * is in the WAITING state, it is moved to the WAITING­SUSPENDED state.
 */
int tn_task_suspend(struct TN_Task *task);

/**
 * Release task from SUSPENDED state. If the given task is in the SUSPENDED state,
 * it is moved to READY state; afterwards it has the lowest precedence amoung
 * runnable tasks with the same priority. If the task is in WAITING_SUSPENDED state,
 * it is moved to WAITING state.
 */
int tn_task_resume(struct TN_Task *task);

/**
 * Put current task to sleep for at most timeout ticks. When the timeout
 * expires and the task was not suspended during the sleep, it is switched
 * to runnable state. If the timeout value is TN_WAIT_INFINITE and the task
 * was not suspended during the sleep, the task will sleep until another
 * function call (like tn_task_wakeup() or similar) will make it runnable.
 *
 * Each task has a wakeup request counter. If its value for currently
 * running task is greater then 0, the counter is decremented by 1 and the
 * currently running task is not switched to the sleeping mode and
 * continues execution.
 */
int tn_task_sleep(unsigned long timeout);

/**
 * Wake up task from sleep.
 *
 * These functions wakes up the task specified by the task from sleep mode.
 * The function placing the task into the sleep mode will return to the task
 * without errors. If the task is not in the sleep mode, the wakeup request
 * for the task is queued and the wakeup_count is incremented by 1.
 *
 * TODO: is it actually good idea about wakeup_count?
 *       it seems just like dirty hack to prevent race conditions.
 *       It makes the programmer able to not create proper syncronization.
 */
int tn_task_wakeup(struct TN_Task *task);
int tn_task_iwakeup(struct TN_Task *task);

/**
 * Activate task that was created by tn_task_create() without TN_TASK_START_ON_CREATION
 * option.
 *
 * Task is moved from DORMANT state to the READY state.
 * If task isn't in DORMANT state, activate_count is incremented.
 * If activate_count is already non-zero, TERR_OVERFLOW is returned.
 * TODO: is it actually good idea about activate_count?
 *       it seems just like dirty hack to prevent race conditions.
 *       It makes the programmer able to not create proper syncronization.
 */
int tn_task_activate(struct TN_Task *task);
int tn_task_iactivate(struct TN_Task *task);

/**
 * Release task from WAIT state.
 *
 * These functions forcibly release task from any waiting state.
 * If task is in WAITING state, it is moved to READY state.
 * If task is in WAITING_SUSPENDED state, it is moved to SUSPENDED state.
 */
int tn_task_release_wait(struct TN_Task *task);
int tn_task_irelease_wait(struct TN_Task *task);

/**
 * This function terminates the currently running task. The task is moved to the DORMANT state.
 * If activate requests exist (activation request count is 1) the count 
 * is decremented and the task is moved to the READY state.
 * In this case the task starts execution from the beginning (as after creation and activation),
 * all mutexes locked by the task are unlocked etc.
 * The task will have the lowest precedence among all tasks with the same priority in the READY state.
 *
 * After exiting, the task may be reactivated by the tn_task_iactivate()
 * function or the tn_task_activate() function call.
 * In this case task starts execution from beginning (as after creation/activation).
 * The task will have the lowest precedence among all tasks with the same
 * priority in the READY state.
 *
 * If this function is invoked with TN_EXIT_AND_DELETE_TASK parameter value, the task will be deleted
 * after termination and cannot be reactivated (needs recreation).
 * 
 * This function cannot be invoked from interrupts.
 */
void tn_task_exit(int attr);


/**
 * This function terminates the task specified by the task. The task is moved to the DORMANT state.
 * When the task is waiting in a wait queue, the task is removed from the queue.
 * If activate requests exist (activation request count is 1) the count 
 * is decremented and the task is moved to the READY state.
 * In this case the task starts execution from beginning (as after creation and activation), all
 * mutexes locked by the task are unlocked etc.
 * The task will have the lowest precedence among all tasks with the same priority in the READY state.
 *
 * After termination, the task may be reactivated by the tn_task_iactivate()
 * function or the tn_task_activate() function call.
 * In this case the task starts execution from the beginning (as after creation/activation).
 * The task will have the lowest precedence among all tasks with the same
 * priority in the READY state.
 *
 * A task must not terminate itself by this function (use the tn_task_exit() function instead).
 * This function cannot be used in interrupts.
 */
int tn_task_terminate(struct TN_Task *task);

/**
 * This function deletes the task specified by the task. The task must be in the DORMANT state,
 * otherwise TERR_WCONTEXT will be returned.
 *
 * This function resets the id_task field in the task structure to 0
 * and removes the task from the system tasks list.
 * The task can not be reactivated after this function call (the task must be recreated).
 *
 * This function cannot be invoked from interrupts.
 */
int tn_task_delete(struct TN_Task *task);

/**
 * Set new priority for task.
 * If priority is 0, then task's base_priority is set.
 */
int tn_task_change_priority(struct TN_Task *task, int new_priority);

#endif // _TN_TASKS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


