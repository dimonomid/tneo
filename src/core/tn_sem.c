/*******************************************************************************
 *
 * TNeo: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright 2013, 2014 Anders Montonen.
 *    TNeo:                      copyright 2014       Dmitry Frank.
 *
 *    TNeo was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeo.
 *
 *    Permission to use, copy, modify, and distribute this software in source
 *    and binary forms and its documentation for any purpose and without fee
 *    is hereby granted, provided that the above copyright notice appear
 *    in all copies and that both that copyright notice and this permission
 *    notice appear in supporting documentation.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE DMITRY FRANK AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DMITRY FRANK OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *    THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- common tnkernel headers
#include "tn_common.h"
#include "tn_sys.h"

//-- internal tnkernel headers
#include "_tn_tasks.h"
#include "_tn_list.h"


//-- header of current module
#include "_tn_sem.h"

//-- header of other needed modules
#include "tn_tasks.h"




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
_TN_STATIC_INLINE enum TN_RCode _check_param_generic(
      const struct TN_Sem *sem
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (sem == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (!_tn_sem_is_valid(sem)){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

/**
 * Additional param checking when creating semaphore
 */
_TN_STATIC_INLINE enum TN_RCode _check_param_create(
      const struct TN_Sem *sem,
      int start_count,
      int max_count
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (sem == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (0
         || _tn_sem_is_valid(sem)
         || max_count <= 0
         || start_count < 0
         || start_count > max_count
         )
   {
      rc = TN_RC_WPARAM;
   }

   return rc;
}

#else
#  define _check_param_generic(sem)                            (TN_RC_OK)
#  define _check_param_create(sem, start_count, max_count)     (TN_RC_OK)
#endif
// }}}


/**
 * Generic function that performs job from task context
 *
 * @param sem        semaphore to perform job on
 * @param p_worker   pointer to actual worker function
 * @param timeout    see `#TN_TickCnt`
 */
_TN_STATIC_INLINE enum TN_RCode _sem_job_perform(
      struct TN_Sem *sem,
      enum TN_RCode (p_worker)(struct TN_Sem *sem),
      TN_TickCnt timeout
      )
{
   enum TN_RCode rc = _check_param_generic(sem);
   TN_BOOL waited_for_sem = TN_FALSE;

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();      //-- disable interrupts
      rc = p_worker(sem);     //-- call actual worker function

      //-- if we should wait, put current task to wait
      if (rc == TN_RC_TIMEOUT && timeout != 0){
         _tn_task_curr_to_wait_action(
               &(sem->wait_queue), TN_WAIT_REASON_SEM, timeout
               );

         //-- rc will be set later thanks to waited_for_sem
         waited_for_sem = TN_TRUE;
      }

#if TN_DEBUG
      //-- if we're going to wait, _tn_need_context_switch() must return TN_TRUE
      if (!_tn_need_context_switch() && waited_for_sem){
         _TN_FATAL_ERROR("");
      }
#endif

      TN_INT_RESTORE();       //-- restore previous interrupts state
      _tn_context_switch_pend_if_needed();
      if (waited_for_sem){
         //-- get wait result
         rc = _tn_curr_run_task->task_wait_rc;
      }

   }
   return rc;
}

/**
 * Generic function that performs job from interrupt context
 *
 * @param sem        semaphore to perform job on
 * @param p_worker   pointer to actual worker function
 */
_TN_STATIC_INLINE enum TN_RCode _sem_job_iperform(
      struct TN_Sem *sem,
      enum TN_RCode (p_worker)(struct TN_Sem *sem)
      )
{
   enum TN_RCode rc = _check_param_generic(sem);

   //-- perform additional params checking (if enabled by TN_CHECK_PARAM)
   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA_INT;

      TN_INT_IDIS_SAVE();     //-- disable interrupts
      rc = p_worker(sem);     //-- call actual worker function
      TN_INT_IRESTORE();      //-- restore previous interrupts state
      _TN_CONTEXT_SWITCH_IPEND_IF_NEEDED();
   }
   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _sem_signal(struct TN_Sem *sem)
{
   enum TN_RCode rc = TN_RC_OK;

   //-- wake up first (if any) task from the semaphore wait queue
   if (  !_tn_task_first_wait_complete(
            &sem->wait_queue, TN_RC_OK,
            TN_NULL, TN_NULL, TN_NULL
            )
      )
   {
      //-- no tasks are waiting for that semaphore,
      //   so, just increase its count if possible.
      if (sem->count < sem->max_count){
         sem->count++;
      } else {
         rc = TN_RC_OVERFLOW;
      }
   }

   return rc;
}

_TN_STATIC_INLINE enum TN_RCode _sem_wait(struct TN_Sem *sem)
{
   enum TN_RCode rc = TN_RC_OK;

   //-- decrement semaphore count if possible.
   //   If not, return TN_RC_TIMEOUT
   //   (it is handled in _sem_job_perform() / _sem_job_iperform())
   if (sem->count > 0){
      sem->count--;
   } else {
      rc = TN_RC_TIMEOUT;
   }

   return rc;
}





/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (tn_sem.h)
 */
enum TN_RCode tn_sem_create(
      struct TN_Sem *sem,
      int start_count,
      int max_count
      )
{
   //-- perform additional params checking (if enabled by TN_CHECK_PARAM)
   enum TN_RCode rc = _check_param_create(sem, start_count, max_count);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {

      _tn_list_reset(&(sem->wait_queue));

      sem->count     = start_count;
      sem->max_count = max_count;
      sem->id_sem    = TN_ID_SEMAPHORE;

   }
   return rc;
}

/*
 * See comments in the header file (tn_sem.h)
 */
enum TN_RCode tn_sem_delete(struct TN_Sem * sem)
{
   //-- perform additional params checking (if enabled by TN_CHECK_PARAM)
   enum TN_RCode rc = _check_param_generic(sem);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      //-- Remove all tasks from wait queue, returning the TN_RC_DELETED code.
      _tn_wait_queue_notify_deleted(&(sem->wait_queue));

      sem->id_sem = TN_ID_NONE;        //-- Semaphore does not exist now
      TN_INT_RESTORE();

      //-- we might need to switch context if _tn_wait_queue_notify_deleted()
      //   has woken up some high-priority task
      _tn_context_switch_pend_if_needed();
   }
   return rc;
}

/*
 * See comments in the header file (tn_sem.h)
 */
enum TN_RCode tn_sem_signal(struct TN_Sem *sem)
{
   return _sem_job_perform(sem, _sem_signal, 0);
}

/*
 * See comments in the header file (tn_sem.h)
 */
enum TN_RCode tn_sem_isignal(struct TN_Sem *sem)
{
   return _sem_job_iperform(sem, _sem_signal);
}

/*
 * See comments in the header file (tn_sem.h)
 */
enum TN_RCode tn_sem_wait(struct TN_Sem *sem, TN_TickCnt timeout)
{
   return _sem_job_perform(sem, _sem_wait, timeout);
}

/*
 * See comments in the header file (tn_sem.h)
 */
enum TN_RCode tn_sem_wait_polling(struct TN_Sem *sem)
{
   return _sem_job_perform(sem, _sem_wait, 0);
}

/*
 * See comments in the header file (tn_sem.h)
 */
enum TN_RCode tn_sem_iwait_polling(struct TN_Sem *sem)
{
   return _sem_job_iperform(sem, _sem_wait);
}


