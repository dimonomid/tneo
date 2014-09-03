/*

  TNKernel real-time kernel

  Copyright © 2004, 2013 Yuri Tiomkin
  All rights reserved.

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

  /* ver 2.7  */

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- common tnkernel headers
#include "tn_common.h"
#include "tn_sys.h"
#include "tn_internal.h"

//-- header of current module
#include "tn_mutex.h"

//-- header of other needed modules
#include "tn_tasks.h"


#if TN_USE_MUTEXES

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/



#if TN_MUTEX_REC
//-- recursive locking enabled
#  define __mutex_lock_cnt_change(mutex, value)  { (mutex)->cnt += (value); }
#  define __MUTEX_REC_LOCK_RETVAL   TERR_NO_ERR
#else
//-- recursive locking disabled
#  define __mutex_lock_cnt_change(mutex, value)
#  define __MUTEX_REC_LOCK_RETVAL   TERR_ILUSE
#endif

/*
     The ceiling protocol in ver 2.6 and latest is more "lightweight" in comparison
   to previous versions.
     The code of ceiling protocol is derived from Vyacheslav Ovsiyenko version
*/


// L. Sha, R. Rajkumar, J. Lehoczky, Priority Inheritance Protocols: An Approach
// to Real-Time Synchronization, IEEE Transactions on Computers, Vol.39, No.9, 1990




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Iterate through all the tasks that wait for lock mutex,
 * checking if task's priority is higher than ref_priority.
 *
 * Max priority (i.e. lowest value) is returned.
 */
static int _find_max_blocked_priority(struct TN_Mutex *mutex, int ref_priority)
{
   int               priority;
   struct TN_Task   *task;

   priority = ref_priority;

   //-- Iterate through all the tasks that wait for lock mutex.
   //   Highest priority (i.e. lowest number) will be returned eventually.
   tn_list_for_each_entry(task, &(mutex->wait_queue), task_queue){
      if(task->priority < priority){
         //--  task priority is higher, remember it
         priority = task->priority;
      }
   }

   return priority;
}

static inline void _task_set_priority(struct TN_Task *task, int priority)
{
   //-- transitive priority changing

   // if we have a task A that is blocked by the task B and we changed priority
   // of task A,priority of task B also have to be changed. I.e, if we have
   // the task A that is waiting for the mutex M1 and we changed priority
   // of this task, a task B that holds a mutex M1 now, also needs priority's
   // changing.  Then, if a task B now is waiting for the mutex M2, the same
   // procedure have to be applied to the task C that hold a mutex M2 now
   // and so on.


   if (task->priority <= priority){
      //-- Task's priority is alreasy higher than given one, so,
      //   don't do anything
   } else if (task->task_state == TSK_STATE_RUNNABLE){
      //-- Task is runnable, so, set new priority to it
      //   (for runnable tasks, we should use special function for that)
      _tn_change_running_task_priority(task, priority);
   } else {
      //-- Task is not runnable, so, just set new priority to it
      task->priority = priority;

      //-- and check if the task is waiting for mutex
      if (     (task->task_state & TSK_STATE_WAIT)
            && (task->task_wait_reason == TSK_WAIT_REASON_MUTEX_I)
         )
      {
         //-- Task is waiting for another mutex. In this case, 
         //   call this function again, recursively,
         //   for mutex's holder
         _task_set_priority(
               get_mutex_by_wait_queque(task->pwait_queue)->holder,
               priority
               );
      }
   }

}

static inline void _mutex_do_lock(struct TN_Mutex *mutex)
{
   mutex->holder = tn_curr_run_task;
   __mutex_lock_cnt_change(mutex, 1);

   //-- Add mutex to task's locked mutexes queue
   tn_list_add_tail(&(tn_curr_run_task->mutex_queue), &(mutex->mutex_queue));

   if (mutex->attr == TN_MUTEX_ATTR_CEILING){
      //-- Ceiling protocol

      if(tn_curr_run_task->priority > mutex->ceil_priority){
         _tn_change_running_task_priority(tn_curr_run_task, mutex->ceil_priority);
      }
   }
}

/**
 * Should be called whenever task wanted to lock mutex, but it is already 
 * locked, so, task was just added to mutex's wait_queue.
 *
 * Checks for deadlock; if it is detected, flag TN_STATE_FLAG__DEADLOCK is set.
 */
static void _check_deadlock(struct TN_Mutex *mutex, struct TN_Task *task)
{
   struct TN_Task *holder = mutex->holder;
   if (     (holder->task_state & TSK_STATE_WAIT)
         && (
               (holder->task_wait_reason == TSK_WAIT_REASON_MUTEX_I)
            || (holder->task_wait_reason == TSK_WAIT_REASON_MUTEX_C)
            )
      )
   {
      //-- holder is waiting for another mutex (mutex2). In this case, 
      //   check against task's locked mutexes list; if mutex2 is
      //   there - deadlock.
      //
      //   Otherwise, get holder of mutex2 and call this function
      //   again, recursively.

      struct TN_Mutex *mutex2 = get_mutex_by_wait_queque(holder->pwait_queue);
      if (_tn_is_mutex_locked_by_task(task, mutex2)){
         //-- deadlock: set flag
         //   (user will be notified if he has set tn_event_callback)
         _tn_sys_state_flags_set(TN_STATE_FLAG__DEADLOCK);
      } else {
         //-- call this function again, recursively
         _check_deadlock(mutex2, task);
      }

   } else {
      //-- no deadlock: holder of given mutex isn't waiting for another mutex
   }
}

static inline void _add_curr_task_to_mutex_wait_queue(struct TN_Mutex *mutex, unsigned long timeout)
{
   enum TN_WaitReason wait_reason;

   if (mutex->attr == TN_MUTEX_ATTR_INHERIT){
      //-- Priority inheritance protocol

      //-- if run_task curr priority higher holder's curr priority
      if (tn_curr_run_task->priority < mutex->holder->priority){
         _task_set_priority(mutex->holder, tn_curr_run_task->priority);
      }

      wait_reason = TSK_WAIT_REASON_MUTEX_I;
   } else {
      //-- Priority ceiling protocol
      wait_reason = TSK_WAIT_REASON_MUTEX_C;
   }

   _tn_task_curr_to_wait_action(&(mutex->wait_queue), wait_reason, timeout);

   //-- check if there is deadlock
   _check_deadlock(mutex, tn_curr_run_task);
}




/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

//----------------------------------------------------------------------------
//  Structure's Field mutex->id_mutex should be set to 0
//----------------------------------------------------------------------------
enum TN_Retval tn_mutex_create(struct TN_Mutex * mutex,
                    int attribute,
                    int ceil_priority)
{

#if TN_CHECK_PARAM
   if(mutex == NULL)
      return TERR_WRONG_PARAM;
   if(mutex->id_mutex != 0) //-- no recreation
      return TERR_WRONG_PARAM;
   if(attribute != TN_MUTEX_ATTR_CEILING && attribute != TN_MUTEX_ATTR_INHERIT)
      return TERR_WRONG_PARAM;
   if(attribute == TN_MUTEX_ATTR_CEILING &&
         (ceil_priority < 1 || ceil_priority > TN_NUM_PRIORITY - 2))
      return TERR_WRONG_PARAM;
#endif

   tn_list_reset(&(mutex->wait_queue));
   tn_list_reset(&(mutex->mutex_queue));
   tn_list_reset(&(mutex->lock_mutex_queue));

   mutex->attr          = attribute;
   mutex->holder        = NULL;
   mutex->ceil_priority = ceil_priority;
   mutex->cnt           = 0;
   mutex->id_mutex      = TN_ID_MUTEX;

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_mutex_delete(struct TN_Mutex *mutex)
{
   TN_INTSAVE_DATA;

#if TN_CHECK_PARAM
   if(mutex == NULL)
      return TERR_WRONG_PARAM;
   if(mutex->id_mutex != TN_ID_MUTEX)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   //-- mutex can be deleted if only it isn't held 
   if (mutex->holder != NULL && mutex->holder != tn_curr_run_task){
      return TERR_ILUSE;
   }

   //-- Remove all tasks (if any) from mutex's wait queue
   //   NOTE: we might sleep there
   _tn_wait_queue_notify_deleted(&(mutex->wait_queue), TN_INTSAVE_DATA_ARG_GIVE);

   if (mutex->holder != NULL){
      //-- If the mutex is locked
      _tn_mutex_do_unlock(mutex);

      //-- NOTE: redundant reset, because it will anyway
      //         be reset in tn_mutex_create()
      //
      //         Probably we need to remove it.
      tn_list_reset(&(mutex->mutex_queue));
   }

   mutex->id_mutex = 0; //-- mutex does not exist now

   tn_enable_interrupt();

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_mutex_lock(struct TN_Mutex *mutex, unsigned long timeout)
{
   TN_INTSAVE_DATA;

   enum TN_Retval ret = TERR_NO_ERR;

#if TN_CHECK_PARAM
   if (mutex == NULL){
      ret = TERR_WRONG_PARAM;
      goto out;
   }
   if (mutex->id_mutex != TN_ID_MUTEX){
      ret = TERR_NOEXS;
      goto out;
   }
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   if (tn_curr_run_task == mutex->holder){
      //-- mutex is already locked by current task
      //   if recursive locking enabled (TN_MUTEX_REC), increment lock count,
      //   otherwise error is returned
      __mutex_lock_cnt_change(mutex, 1);
      ret = __MUTEX_REC_LOCK_RETVAL;
      goto out_ei;
   }

   if (
         mutex->attr == TN_MUTEX_ATTR_CEILING
         && tn_curr_run_task->base_priority < mutex->ceil_priority
      )
   {
      //-- base priority of current task higher
      ret = TERR_ILUSE;
      goto out_ei;
   }

   if (mutex->holder == NULL){
      //-- mutex is not locked, let's lock it
      _mutex_do_lock(mutex);
      goto out_ei;
   } else {
      //-- mutex is already locked
      if (timeout == 0){
         //-- in polling mode, just return TERR_TIMEOUT
         ret = TERR_TIMEOUT;
         goto out_ei;
      } else {
         //-- timeout specified, so, wait until mutex is free or timeout expired
         _add_curr_task_to_mutex_wait_queue(mutex, timeout);

         //-- ret will be set later to tn_curr_run_task->task_wait_rc;
         goto out_ei_switch_context;
      }
   }

   //-- should never be here
   ret = TERR_INTERNAL;
   goto out;

out:
   return ret;

out_ei:
   tn_enable_interrupt();
   return ret;

out_ei_switch_context:
   tn_enable_interrupt();
   tn_switch_context();

   ret = tn_curr_run_task->task_wait_rc;
   return ret;
}

//----------------------------------------------------------------------------
//  Try to lock mutex
//----------------------------------------------------------------------------
enum TN_Retval tn_mutex_lock_polling(struct TN_Mutex *mutex)
{
   return tn_mutex_lock(mutex, 0);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_mutex_unlock(struct TN_Mutex *mutex)
{
   enum TN_Retval ret = TERR_NO_ERR;

   TN_INTSAVE_DATA;

#if TN_CHECK_PARAM
   if(mutex == NULL){
      ret = TERR_WRONG_PARAM;
      goto out;
   }
   if(mutex->id_mutex != TN_ID_MUTEX){
      ret = TERR_NOEXS;
      goto out;
   }
#endif

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   //-- unlocking is enabled only for the owner and already locked mutex
   if (tn_curr_run_task != mutex->holder){
      ret = TERR_ILUSE;
      goto out_ei;
   }

   __mutex_lock_cnt_change(mutex, -1);

   if (mutex->cnt > 0){
      //-- there was recursive lock, so here we just decremented counter, 
      //   but don't unlock the mutex. TERR_NO_ERR will be returned.
      goto out_ei;
   } else {
      //-- lock counter is 0, so, unlock mutex
      _tn_mutex_do_unlock(mutex);
      goto out_ei_switch_context;
   }

out:
   return ret;

out_ei:
   tn_enable_interrupt();
   return ret;

out_ei_switch_context:
   tn_enable_interrupt();
   tn_switch_context();
   return ret;
}





/*******************************************************************************
 *    INTERNAL TNKERNEL FUNCTIONS
 ******************************************************************************/

/**
 * See comments in tn_internal.h file
 */
BOOL _tn_mutex_do_unlock(struct TN_Mutex * mutex)
{
   //-- explicitly reset lock count to 0, because it might be not zero
   //   if mutex is unlocked because task is being deleted.
   mutex->cnt = 0;

   {
      int pr;

      //-- Delete curr mutex from task's locked mutexes queue
      tn_list_remove_entry(&(mutex->mutex_queue));

      //-- Now, we need to determine new priority of current task.
      //   We start from its base priority, but if there are other
      //   mutexes that are locked by the task, we should check
      //   what priority we should set.
      pr = tn_curr_run_task->base_priority;

      {
         struct TN_Mutex *tmp_mutex;
         tn_list_for_each_entry(tmp_mutex, &(tn_curr_run_task->mutex_queue), mutex_queue){
            switch (tmp_mutex->attr){
               case TN_MUTEX_ATTR_CEILING:
                  if (tmp_mutex->ceil_priority < pr){
                     pr = tmp_mutex->ceil_priority;
                  }
                  break;

               case TN_MUTEX_ATTR_INHERIT:
                  pr = _find_max_blocked_priority(tmp_mutex, pr);
                  break;

               default:
                  //-- should never happen
                  TN_FATAL_ERROR("wrong mutex attr=%d", tmp_mutex->attr);
                  break;
            }
         }
      }

      //-- New priority determined, set it
      if (pr != tn_curr_run_task->priority){
         _tn_change_running_task_priority(tn_curr_run_task, pr);
      }
   }


   //-- Check for the task(s) that want to lock the mutex
   if (tn_is_list_empty(&(mutex->wait_queue))){
      //-- no more tasks want to lock the mutex,
      //   so, set holder to NULL and return.
      mutex->holder = NULL;
   } else {
      //-- there are tasks that want to lock the mutex,
      //   so, lock it by the first task in the queue

      struct TN_Task *task;

      //-- remove task from mutex's wait_queue
      task = tn_list_first_entry_remove(&(mutex->wait_queue), typeof(*task), task_queue);

      //-- set it as mutex holder
      mutex->holder = task;

      if (     mutex->attr == TN_MUTEX_ATTR_CEILING
            && task->priority > mutex->ceil_priority){
         //-- we need to ceil priority of that task
         task->priority = mutex->ceil_priority;
      }

      _tn_task_wait_complete(task);
      tn_list_add_tail(&(task->mutex_queue), &(mutex->mutex_queue));
   }

   return TRUE;
}



#endif //-- TN_USE_MUTEXES

