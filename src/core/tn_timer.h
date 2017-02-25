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
 * Timer is a kernel object that is used to ask the kernel to call some
 * user-provided function at a particular time in the future, based on the
 * $(TN_SYS_TIMER_LINK) tick.
 *
 * If you need to repeatedly wake up particular task, you can create semaphore
 * which you should \ref tn_sem_wait() "wait for" in the task, and \ref
 * tn_sem_isignal() "signal" in the timer callback (remember that you should
 * use `tn_sem_isignal()` in this callback, since it is called from an ISR).
 *
 * If you need to perform rather fast action (such as toggle some pin, or the
 * like), consider doing that right in the timer callback, in order to avoid
 * context switch overhead.
 *
 * The timer callback approach provides ultimate flexibility.
 *
 * In the spirit of TNeo, timers are as lightweight as possible. That's
 * why there is only one type of timer: the single-shot timer. If you need your
 * timer to fire repeatedly, you can easily restart it from the timer function
 * by the `tn_timer_start()`, so it's not a problem.
 *
 * When timer fires, the user-provided function is called. Be aware of the
 * following:
 *
 * - Function is called from an ISR context (namely, from $(TN_SYS_TIMER_LINK)
 *   ISR, by the `tn_tick_int_processing()`);
 * - Function is called with global interrupts enabled.
 *
 * Consequently:
 *
 * - It's legal to call interrupt services from this function;
 * - You should make sure that your interrupt stack is enough for this
 *   function;
 * - The function should be as fast as possible;
 *
 * See `#TN_TimerFunc` for the prototype of the function that could be
 * scheduled.
 *
 * TNeo offers two implementations of timers: static and dynamic. Refer
 * to the page \ref time_ticks for details.
 *
 * \section timers_static_implementation Implementation of static timers
 *
 * Although you don't have to understand the implementation of timers to use
 * them, it is probably worth knowing, particularly because the kernel have an
 * option `#TN_TICK_LISTS_CNT` to customize the balance between performance of
 * `tn_tick_int_processing()` and memory occupied by timers.
 *
 * The easiest implementation of timers could be something like this: we
 * have just a single list with all active timers, and at every system tick
 * we should walk through all the timers in this list, and do the following
 * with each timer:
 *
 * - Decrement timeout by 1
 * - If new timeout is 0, then remove that timer from the list (i.e. make timer
 *   inactive), and fire the appropriate timer function.
 *
 * This approach has drawbacks:
 *
 * - We can't manage timers from the function called by timer. If we do so
 *   (say, if we start new timer), then the timer list gets modified. But we
 *   are currently iterating through this list, so, it's quite easy to mix
 *   things up.
 * - It is inefficient on rather large amount of timers and/or with large
 *   timeouts: we should iterate through all of them each system tick.
 *
 * The latter is probably not so critical in the embedded world since large
 * amount of timers is unlikely there; whereas the former is actually notable.
 *
 * So, different approach was applied. The main idea is taken from the mainline
 * Linux kernel source, but the implementation was simplified much because (1)
 * embedded systems have much less resources, and (2) the kernel doesn't need
 * to scale as well as Linux does. You can read about Linux timers
 * implementation in the book "Linux Device Drivers", 3rd edition:
 *
 * - Time, Delays, and Deferred Work
 *   - Kernel Timers
 *     - The Implementation of Kernel Timers
 *
 * This book is freely available at http://lwn.net/Kernel/LDD3/ .
 *
 * So, TNeo's implementation:
 *
 * We have configurable value `N` that is a power of two, typical values are
 * `4`, `8` or `16`.
 *
 * If the timer expires in the next `1` to `(N - 1)` system ticks, it is added
 * to one of the `N` lists (the so-called "tick" lists) devoted to short-range
 * timers using the least significant bits of the `timeout` value. If it
 * expires farther in the future, it is added to the "generic" list.
 *
 * Each `N`-th system tick, all the timers from "generic" list are walked
 * through, and the following is performed with each timer:
 *
 * - `timeout` value decremented by `N`
 * - if resulting `timeout` is less than `N`, timer is moved to the appropriate
 *   "tick" list.
 *
 * At *every* system tick, all the timers from current "tick" list are fired
 * unconditionally. This is an efficient and nice solution.
 *
 * The attentive reader may want to ask why do we use `(N - 1)` "tick" lists if
 * we actually have `N` lists. That's because, again, we want to be able to
 * modify timers from the timer function. If we use `N` lists, and user wants
 * to add new timer with `timeout` equal to `N`, then new timer will be added
 * to the same list which is iterated through at the moment, and things will be
 * mixed up.
 *
 * If we use `(N - 1)` lists, we are guaranteed that new timers can't be added
 * to the current "tick" list while we are iterating through it. 
 * (although timer can be deleted from that list, but it's ok)
 *
 * The `N` in the TNeo is configured by the compile-time option
 * `#TN_TICK_LISTS_CNT`.
 */


#ifndef _TN_TIMER_H
#define _TN_TIMER_H

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

struct TN_Timer;


/**
 * Prototype of the function that should be called by timer.
 *
 * When timer fires, the user-provided function is called. Be aware of the
 * following:
 *    - Function is called from ISR context (namely, from $(TN_SYS_TIMER_LINK)
 *      ISR, by the `tn_tick_int_processing()`);
 *    - Function is called with global interrupts enabled.
 *
 * Consequently:
 *
 *   - It's legal to call interrupt services from this function;
 *   - The function should be as fast as possible.
 *
 * @param timer
 *    Timer that caused function to be called
 * @param p_user_data
 *    The user-provided pointer given to `tn_timer_create()`.
 */
typedef void (TN_TimerFunc)(struct TN_Timer *timer, void *p_user_data);

/**
 * Timer
 */
struct TN_Timer {
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   enum TN_ObjId id_timer;
   ///
   /// A list item to be included in the $(TN_SYS_TIMER_LINK) queue
   struct TN_ListItem timer_queue;
   ///
   /// Function to be called by timer
   TN_TimerFunc *func;
   ///
   /// User data pointer that is given to user-provided `func`.
   void *p_user_data;

#if TN_DYNAMIC_TICK || defined(DOXYGEN_ACTIVE)
   ///
   /// $(TN_IF_ONLY_DYNAMIC_TICK_SET)
   /// 
   /// Tick count value when timer was started
   TN_TickCnt start_tick_cnt;
   ///
   /// $(TN_IF_ONLY_DYNAMIC_TICK_SET)
   ///
   /// Timeout value (it is set just once, and stays unchanged until timer is
   /// expired, cancelled or restarted)
   TN_TickCnt timeout;
#endif

#if !TN_DYNAMIC_TICK || defined(DOXYGEN_ACTIVE)
   ///
   /// $(TN_IF_ONLY_DYNAMIC_TICK_NOT_SET)
   ///
   /// Current (left) timeout value
   TN_TickCnt timeout_cur;
#endif
};





#if TN_DYNAMIC_TICK || defined(DOXYGEN_ACTIVE)

/**
 * $(TN_IF_ONLY_DYNAMIC_TICK_SET)
 *
 * Prototype of callback function that should schedule next time to call 
 * `tn_tick_int_processing()`.
 *
 * See `tn_callback_dyn_tick_set()`
 *
 * @param timeout 
 *    Timeout after which `tn_tick_int_processing()` should be called next
 *    time. Note the following:
 *    - It might be `#TN_WAIT_INFINITE`, which means that there are no active
 *      timeouts, and so, there's no need for tick interrupt at all.
 *    - It might be `0`; in this case, it's <i>already</i> time to call
 *      `tn_tick_int_processing()`. You might want to set interrupt request
 *      bit then, in order to get to it as soon as possible.
 *    - In other cases, the function should schedule next call to
 *      `tn_tick_int_processing()` in the `timeout` tick periods.
 *
 */
typedef void (TN_CBTickSchedule)(TN_TickCnt timeout);

/**
 * $(TN_IF_ONLY_DYNAMIC_TICK_SET)
 *
 * Prototype of callback function that should return current system tick
 * counter value.
 *
 * See `tn_callback_dyn_tick_set()`
 *
 * @return current system tick counter value.
 */
typedef TN_TickCnt (TN_CBTickCntGet)(void);

#endif




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
 * Construct the timer. `id_timer` field should not contain
 * `#TN_ID_TIMER`, otherwise, `#TN_RC_WPARAM` is returned.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param timer
 *    Pointer to already allocated `struct TN_Timer`
 * @param func
 *    Function to be called by timer, can't be `TN_NULL`. See `TN_TimerFunc()`
 * @param p_user_data
 *    User data pointer that is given to user-provided `func`.
 *
 * @return 
 *    * `#TN_RC_OK` if timer was successfully created;
 *    * `#TN_RC_WPARAM` if wrong params were given.
 */
enum TN_RCode tn_timer_create(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      );

/**
 * Destruct the timer. If the timer is active, it is cancelled first.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param timer     timer to destruct
 *
 * @return 
 *    * `#TN_RC_OK` if timer was successfully deleted;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_timer_delete(struct TN_Timer *timer);

/**
 * Start or restart the timer: that is, schedule the timer's function (given to
 * `tn_timer_create()`) to be called later by the kernel. See `TN_TimerFunc()`.
 *
 * It is legal to restart already active timer. In this case, the timer will be
 * cancelled first.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param timer
 *    Timer to start
 * @param timeout
 *    Number of system ticks after which timer should fire (i.e. function 
 *    should be called). **Note** that `timeout` can't be `#TN_WAIT_INFINITE` or
 *    `0`.
 *
 * @return 
 *    * `#TN_RC_OK` if timer was successfully started;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * `#TN_RC_WPARAM` if wrong params were given: say, `timeout` is either 
 *      `#TN_WAIT_INFINITE` or `0`.
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_timer_start(struct TN_Timer *timer, TN_TickCnt timeout);

/**
 * If timer is active, cancel it. If timer is already inactive, nothing is
 * changed.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param timer
 *    Timer to cancel
 *
 * @return 
 *    * `#TN_RC_OK` if timer was successfully cancelled;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_timer_cancel(struct TN_Timer *timer);

/**
 * Set user-provided function and pointer to user data for the timer.
 * Can be called if timer is either active or inactive.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param timer
 *    Pointer to timer
 * @param func
 *    Function to be called by timer, can't be `TN_NULL`. See `TN_TimerFunc()`
 * @param p_user_data
 *    User data pointer that is given to user-provided `func`.
 *
 * @return 
 *    * `#TN_RC_OK` if operation was successfull;
 *    * `#TN_RC_WPARAM` if wrong params were given.
 */
enum TN_RCode tn_timer_set_func(
      struct TN_Timer  *timer,
      TN_TimerFunc     *func,
      void             *p_user_data
      );

/**
 * Returns whether given timer is active or inactive.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param timer
 *    Pointer to timer
 * @param p_is_active
 *    Pointer to `#TN_BOOL` variable in which resulting value should be stored
 *
 * @return
 *    * `#TN_RC_OK` if operation was successfull;
 *    * `#TN_RC_WPARAM` if wrong params were given.
 */
enum TN_RCode tn_timer_is_active(struct TN_Timer *timer, TN_BOOL *p_is_active);

/**
 * Returns how many $(TN_SYS_TIMER_LINK) ticks (at most) is left for the timer
 * to expire. If timer is inactive, 0 is returned.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param timer
 *    Pointer to timer
 * @param p_time_left
 *    Pointer to `#TN_TickCnt` variable in which resulting value should be
 *    stored
 *
 * @return
 *    * `#TN_RC_OK` if operation was successfull;
 *    * `#TN_RC_WPARAM` if wrong params were given.
 */
enum TN_RCode tn_timer_time_left(
      struct TN_Timer *timer,
      TN_TickCnt *p_time_left
      );

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_TIMER_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


