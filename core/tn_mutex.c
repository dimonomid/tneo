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


#ifdef TN_USE_MUTEXES

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

static inline void __mutex_do_lock(struct TN_Mutex *mutex)
{
   mutex->holder = tn_curr_run_task;
   __mutex_lock_cnt_change(mutex, 1);

   //-- Add mutex to task's locked mutexes queue
   queue_add_tail(&(tn_curr_run_task->mutex_queue), &(mutex->mutex_queue));

   if (mutex->attr == TN_MUTEX_ATTR_CEILING){
      //-- Ceiling protocol

      if(tn_curr_run_task->priority > mutex->ceil_priority){
         _tn_change_running_task_priority(tn_curr_run_task, mutex->ceil_priority);
      }
   }
}

static inline void __mutex_add_to_wait_queue(struct TN_Mutex *mutex, unsigned long timeout)
{
   int wait_reason;

   if (mutex->attr == TN_MUTEX_ATTR_INHERIT){
      //-- Priority inheritance protocol

      //-- if run_task curr priority higher holder's curr priority
      if (tn_curr_run_task->priority < mutex->holder->priority){
         _tn_set_current_priority(mutex->holder, tn_curr_run_task->priority);
      }

      wait_reason = TSK_WAIT_REASON_MUTEX_I;
   } else {
      //-- Priority ceiling protocol
      wait_reason = TSK_WAIT_REASON_MUTEX_C;
   }

   _tn_task_curr_to_wait_action(&(mutex->wait_queue), wait_reason, timeout);
}

/*
 * This is "worker" function that is called by both
 * tn_mutex_lock and tn_mutex_lock_polling.
 *
 * @param mutex - mutex which should be locked
 * @param timeout - timeout after which TERR_TIMEOUT will be returned
 *    without actually locking mutex. If it is 0, and mutex is already locked,
 *    return TERR_TIMEOUT immediately.
 */
static inline enum TN_Retval __mutex_lock(struct TN_Mutex *mutex, unsigned long timeout)
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
      __mutex_do_lock(mutex);
      goto out_ei;
   } else {
      //-- mutex is already locked
      if (timeout == 0){
         //-- in polling mode, just return TERR_TIMEOUT
         ret = TERR_TIMEOUT;
         goto out_ei;
      } else {
         //-- timeout specified, so, wait until mutex is free or timeout expired
         __mutex_add_to_wait_queue(mutex, timeout);

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

   queue_reset(&(mutex->wait_queue));
   queue_reset(&(mutex->mutex_queue));
   queue_reset(&(mutex->lock_mutex_queue));

   mutex->attr          = attribute;
   mutex->holder        = NULL;
   mutex->ceil_priority = ceil_priority;
   mutex->cnt           = 0;
   mutex->id_mutex      = TN_ID_MUTEX;

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_mutex_delete(struct TN_Mutex * mutex)
{
   TN_INTSAVE_DATA;

#if TN_CHECK_PARAM
   if(mutex == NULL)
      return TERR_WRONG_PARAM;
   if(mutex->id_mutex != TN_ID_MUTEX)
      return TERR_NOEXS;
#endif

   TN_CHECK_NON_INT_CONTEXT
    
   if(tn_curr_run_task != mutex->holder)
      return TERR_ILUSE;

   //-- Remove all tasks(if any) from mutex's wait queue

   tn_disable_interrupt(); // v.2.7 - thanks to Eugene Scopal

   _tn_wait_queue_notify_deleted(&(mutex->wait_queue), TN_INTSAVE_DATA_ARG_GIVE);

   if(mutex->holder != NULL)  //-- If the mutex is locked
   {
      do_unlock_mutex(mutex);
      queue_reset(&(mutex->mutex_queue));
   }
   mutex->id_mutex = 0; // Mutex not exists now

   tn_enable_interrupt();

   return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_mutex_lock(struct TN_Mutex * mutex, unsigned long timeout)
{
#if 0
#if TN_CHECK_PARAM
   if (timeout == 0){
      return TERR_WRONG_PARAM;
   }
#endif
#endif
   return __mutex_lock(mutex, timeout);
}

//----------------------------------------------------------------------------
//  Try to lock mutex
//----------------------------------------------------------------------------
enum TN_Retval tn_mutex_lock_polling(struct TN_Mutex * mutex)
{
   return __mutex_lock(mutex, 0);
}

//----------------------------------------------------------------------------
enum TN_Retval tn_mutex_unlock(struct TN_Mutex * mutex)
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
   if(tn_curr_run_task != mutex->holder){
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
      do_unlock_mutex(mutex);
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

//----------------------------------------------------------------------------
//   Routines
//----------------------------------------------------------------------------
enum TN_Retval do_unlock_mutex(struct TN_Mutex * mutex)
{
   struct TN_QueHead * curr_que;
   struct TN_Mutex * tmp_mutex;
   struct TN_Task * task;
   int pr;

   //-- Delete curr mutex from task's locked mutexes queue

   queue_remove_entry(&(mutex->mutex_queue));
   pr = tn_curr_run_task->base_priority;

   //---- No more mutexes, locked by the our task

   if(!is_queue_empty(&(tn_curr_run_task->mutex_queue)))
   {
      curr_que = tn_curr_run_task->mutex_queue.next;
      while(curr_que != &(tn_curr_run_task->mutex_queue))
      {
         tmp_mutex = get_mutex_by_mutex_queque(curr_que);

         if(tmp_mutex->attr == TN_MUTEX_ATTR_CEILING)
         {
            if(tmp_mutex->ceil_priority < pr)
               pr = tmp_mutex->ceil_priority;
         }
         else if(tmp_mutex->attr == TN_MUTEX_ATTR_INHERIT)
         {
            pr = find_max_blocked_priority(tmp_mutex, pr);
         }
         curr_que = curr_que->next;
      }
   }

   //-- Restore original priority

   if(pr != tn_curr_run_task->priority)
      _tn_change_running_task_priority(tn_curr_run_task, pr);


   //-- Check for the task(s) that want to lock the mutex

   if(is_queue_empty(&(mutex->wait_queue)))
   {
      mutex->holder = NULL;
      return 1/*true*/;
   }

   //--- Now lock the mutex by the first task in the mutex queue

   curr_que = queue_remove_head(&(mutex->wait_queue));
   task     = get_task_by_tsk_queue(curr_que);
   mutex->holder = task;

   if(mutex->attr == TN_MUTEX_ATTR_CEILING &&
                             task->priority > mutex->ceil_priority)
      task->priority = mutex->ceil_priority;

   _tn_task_wait_complete(task);
   queue_add_tail(&(task->mutex_queue), &(mutex->mutex_queue));

   return 1/*true*/;
}

//----------------------------------------------------------------------------
enum TN_Retval find_max_blocked_priority(struct TN_Mutex *mutex, int ref_priority)
{
   int         priority;
   struct TN_QueHead *curr_que;
   struct TN_Task     *task;

   priority = ref_priority;

   curr_que = mutex->wait_queue.next;
   while (curr_que != &(mutex->wait_queue)){
      task = get_task_by_tsk_queue(curr_que);

      if(task->priority < priority){
         //--  task priority is higher
         priority = task->priority;
      }

      curr_que = curr_que->next;
   }

   return priority;
}

//----------------------------------------------------------------------------
#endif //-- TN_USE_MUTEXES
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


