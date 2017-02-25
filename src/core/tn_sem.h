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

/**
 * \file
 *
 * A semaphore: an object to provide signaling mechanism.
 *
 * There is a lot of confusion about differences between semaphores and
 * mutexes, so, it's quite recommended to read small article by Michael Barr:
 * [Mutexes and Semaphores Demystified](http://goo.gl/YprPBW).
 * 
 * Very short:
 *
 * While mutex is seemingly similar to a semaphore with maximum count of `1`
 * (the so-called binary semaphore), their usage is very different: the purpose
 * of mutex is to protect shared resource. A locked mutex is "owned" by the
 * task that locked it, and only the same task may unlock it. This ownership
 * allows to implement algorithms to prevent priority inversion.  So, mutex is
 * a *locking mechanism*.
 *
 * Semaphore, on the other hand, is *signaling mechanism*. It's quite legal and
 * encouraged for semaphore to be waited for in the task A, and then signaled
 * from task B or even from ISR. It may be used in situations like "producer
 * and consumer", etc.
 *
 * In addition to the article mentioned above, you may want to look at the
 * [related question on stackoverflow.com](http://goo.gl/ZBReHK).
 *
 */

#ifndef _TN_SEM_H
#define _TN_SEM_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Semaphore
 */
struct TN_Sem {
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   enum TN_ObjId id_sem;
   ///
   /// List of tasks that wait for the semaphore
   struct TN_ListItem wait_queue;
   ///
   /// Current semaphore counter value
   int count;
   ///
   /// Max value of `count`
   int max_count;
};


/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Construct the semaphore. `id_sem` field should not contain
 * `#TN_ID_SEMAPHORE`, otherwise, `#TN_RC_WPARAM` is returned.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param sem
 *    Pointer to already allocated `struct TN_Sem`
 * @param start_count
 *    Initial counter value, typically it is equal to `max_count`
 * @param max_count
 *    Maximum counter value.
 *
 * @return 
 *    * `#TN_RC_OK` if semaphore was successfully created;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#TN_RC_WPARAM`.
 */
enum TN_RCode tn_sem_create(
      struct TN_Sem *sem,
      int start_count,
      int max_count
      );

/**
 * Destruct the semaphore.
 *
 * All tasks that wait for the semaphore become runnable with
 * `#TN_RC_DELETED` code returned.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param sem     semaphore to destruct
 *
 * @return 
 *    * `#TN_RC_OK` if semaphore was successfully deleted;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_sem_delete(struct TN_Sem *sem);

/**
 * Signal the semaphore.
 *
 * If current semaphore counter (`count`) is less than `max_count`, counter is
 * incremented by one, and first task (if any) that \ref tn_sem_wait() "waits"
 * for the semaphore becomes runnable with `#TN_RC_OK` returned from
 * `tn_sem_wait()`.
 *
 * if semaphore counter is already has its max value, no action performed and
 * `#TN_RC_OVERFLOW` is returned
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param sem     semaphore to signal
 * 
 * @return
 *    * `#TN_RC_OK` if successful
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * `#TN_RC_OVERFLOW` if `count` is already at maximum value (`max_count`)
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_sem_signal(struct TN_Sem *sem);

/**
 * The same as `tn_sem_signal()` but for using in the ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 */
enum TN_RCode tn_sem_isignal(struct TN_Sem *sem);

/**
 * Wait for the semaphore.
 *
 * If the current semaphore counter (`count`) is non-zero, it is decremented
 * and `#TN_RC_OK` is returned. Otherwise, behavior depends on `timeout` value:
 * task might switch to $(TN_TASK_STATE_WAIT) state until someone \ref
 * tn_sem_signal "signaled" the semaphore or until the `timeout` expired. refer
 * to `#TN_TickCnt`.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_CAN_SLEEP)
 * $(TN_LEGEND_LINK)
 *
 * @param sem     semaphore to wait for
 * @param timeout refer to `#TN_TickCnt`
 *
 * @return
 *    * `#TN_RC_OK` if waiting was successfull
 *    * Other possible return codes depend on `timeout` value,
 *      refer to `#TN_TickCnt`
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_sem_wait(struct TN_Sem *sem, TN_TickCnt timeout);

/**
 * The same as `tn_sem_wait()` with zero timeout.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_sem_wait_polling(struct TN_Sem *sem);

/**
 * The same as `tn_sem_wait()` with zero timeout, but for using in the ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_sem_iwait_polling(struct TN_Sem *sem);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_SEM_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


