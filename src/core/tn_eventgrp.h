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
 * Event group.
 *
 * An event group has an internal variable (of type `#TN_UWord`), which is
 * interpreted as a bit pattern where each bit represents an event. An event
 * group also has a wait queue for the tasks waiting on these events. A task
 * may set specified bits when an event occurs and may clear specified bits
 * when necessary. 
 *
 * The tasks waiting for an event(s) are placed in the event group's wait
 * queue. An event group is a very suitable synchronization object for cases
 * where (for some reasons) one task has to wait for many tasks, or vice versa,
 * many tasks have to wait for one task.
 *
 * \section eventgrp_connect Connecting an event group to other system objects
 *
 * Sometimes task needs to wait for different system events, the most common
 * examples are:
 *
 * - wait for a message from the queue(s) plus wait for some
 *   application-dependent event (such as a flag to finish the task, or
 *   whatever);
 * - wait for messages from multiple queues.
 *
 * If the kernel doesn't offer a mechanism for that, programmer usually have to
 * use polling services on these queues and sleep for a few system ticks.
 * Obviously, this approach has serious drawbacks: we have a lot of useless
 * context switches, and response for the message gets much slower. Actually,
 * we lost the main goal of the preemptive kernel when we use polling services
 * like that.
 *
 * TNeo offers a solution: an event group can be connected to other
 * kernel objects, and these objects will maintain certain flags inside that
 * event group automatically.
 *
 * So, in case of multiple queues, we can act as follows (assume we have two
 * queues: Q1 and Q2) :
 * 
 * - create event group EG;
 * - connect EG with flag 1 to Q1;
 * - connect EG with flag 2 to Q2;
 * - when task needs to receive a message from either Q1 or Q2, it just waits
 *   for the any of flags 1 or 2 in the EG, this is done in the single call 
 *   to `tn_eventgrp_wait()`.
 * - when that event happened, task checks which flag is set, and receive
 *   message from the appropriate queue.
 *
 * Please note that task waiting for the event should **not** clear the flag
 * manually: this flag is maintained completely by the queue. If the queue is
 * non-empty, the flag is set. If the queue becomes empty, the flag is cleared.
 * 
 * For the information on system services related to queue, refer to the \ref 
 * tn_dqueue.h "queue reference".
 *
 * There is an example project available that demonstrates event group
 * connection technique: `examples/queue_eventgrp_conn`. Be sure to examine the
 * readme there.
 *
 */

#ifndef _TN_EVENTGRP_H
#define _TN_EVENTGRP_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"
#include "tn_sys.h"



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Events waiting mode that should be given to `#tn_eventgrp_wait()` and
 * friends.
 */
enum TN_EGrpWaitMode {
   ///
   /// Task waits for **any** of the event bits from the `wait_pattern` 
   /// to be set in the event group.
   /// This flag is mutually exclusive with `#TN_EVENTGRP_WMODE_AND`.
   TN_EVENTGRP_WMODE_OR       = (1 << 0),
   ///
   /// Task waits for **all** of the event bits from the `wait_pattern` 
   /// to be set in the event group.
   /// This flag is mutually exclusive with `#TN_EVENTGRP_WMODE_OR`.
   TN_EVENTGRP_WMODE_AND      = (1 << 1),
   ///
   /// When a task <b>successfully</b> ends waiting for event bit(s), these
   /// bits get cleared atomically and automatically. Other bits stay
   /// unchanged.
   TN_EVENTGRP_WMODE_AUTOCLR  = (1 << 2),
};

/**
 * Modify operation: set, clear or toggle. To be used in `tn_eventgrp_modify()`
 * / `tn_eventgrp_imodify()` functions.
 */
enum TN_EGrpOp {
   ///
   /// Set flags that are set in given `pattern` argument. Note that this
   /// operation can lead to the context switch, since other high-priority 
   /// task(s) might wait for the event.
   TN_EVENTGRP_OP_SET,
   ///
   /// Clear flags that are set in the given `pattern` argument.
   /// This operation can **not** lead to the context switch, 
   /// since tasks can't wait for events to be cleared.
   TN_EVENTGRP_OP_CLEAR,
   ///
   /// Toggle flags that are set in the given `pattern` argument. Note that this
   /// operation can lead to the context switch, since other high-priority 
   /// task(s) might wait for the event that was just set (if any).
   TN_EVENTGRP_OP_TOGGLE,
};


/**
 * Attributes that could be given to the event group object.
 *
 * Makes sense if only `#TN_OLD_EVENT_API` option is non-zero; otherwise,
 * there's just one dummy attribute available: `#TN_EVENTGRP_ATTR_NONE`.
 */
enum TN_EGrpAttr {
#if TN_OLD_EVENT_API || defined(DOXYGEN_ACTIVE)
   /// \attention deprecated. Available if only `#TN_OLD_EVENT_API` option is
   /// non-zero.
   ///
   /// Indicates that only one task could wait for events in this event group.
   /// This flag is mutually exclusive with `#TN_EVENTGRP_ATTR_MULTI` flag.
   TN_EVENTGRP_ATTR_SINGLE    = (1 << 0),
   ///
   /// \attention deprecated. Available if only `#TN_OLD_EVENT_API` option is
   /// non-zero.
   ///
   /// Indicates that multiple tasks could wait for events in this event group.
   /// This flag is mutually exclusive with `#TN_EVENTGRP_ATTR_SINGLE` flag.
   TN_EVENTGRP_ATTR_MULTI     = (1 << 1),
   ///
   /// \attention strongly deprecated. Available if only `#TN_OLD_EVENT_API`
   /// option is non-zero. Use `#TN_EVENTGRP_WMODE_AUTOCLR` instead.
   ///
   /// Can be specified only in conjunction with `#TN_EVENTGRP_ATTR_SINGLE`
   /// flag. Indicates that <b>ALL</b> flags in this event group should be
   /// cleared when task successfully waits for any event in it.
   ///
   /// This actually makes little sense to clear ALL events, but this is what
   /// compatibility mode is for (see `#TN_OLD_EVENT_API`)
   TN_EVENTGRP_ATTR_CLR       = (1 << 2),
#endif

#if !TN_OLD_EVENT_API || defined(DOXYGEN_ACTIVE)
   ///
   /// Dummy attribute that does not change anything. It is needed only for
   /// the assistance of the events compatibility mode (see
   /// `#TN_OLD_EVENT_API`)
   TN_EVENTGRP_ATTR_NONE      = (0),
#endif
};


/**
 * Event group
 */
struct TN_EventGrp {
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   enum TN_ObjId        id_event;
   ///
   /// task wait queue
   struct TN_ListItem   wait_queue;
   ///
   /// current flags pattern
   TN_UWord             pattern;

#if TN_OLD_EVENT_API || defined(DOXYGEN_ACTIVE)
   ///
   /// Attributes that are given to that events group,
   /// available if only `#TN_OLD_EVENT_API` option is non-zero.
   enum TN_EGrpAttr     attr;
#endif

};

/**
 * EventGrp-specific fields related to waiting task,
 * to be included in struct TN_Task.
 */
struct TN_EGrpTaskWait {
   ///
   /// event wait pattern 
   TN_UWord wait_pattern;
   ///
   /// event wait mode: `AND` or `OR`
   enum TN_EGrpWaitMode wait_mode;
   ///
   /// pattern that caused task to finish waiting
   TN_UWord actual_pattern;
};

/**
 * A link to event group: used when event group can be connected to 
 * some kernel object, such as queue.
 */
struct TN_EGrpLink {
   ///
   /// event group whose event(s) should be managed by other kernel object
   struct TN_EventGrp *eventgrp;
   ///
   /// event pattern to manage
   TN_UWord pattern;
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
 * The same as `#tn_eventgrp_create()`, but takes additional argument: `attr`.
 * It makes sense if only `#TN_OLD_EVENT_API` option is non-zero.
 *
 * @param eventgrp
 *    Pointer to already allocated struct TN_EventGrp
 * @param attr    
 *    Attributes for that particular event group object, see `struct
 *    #TN_EGrpAttr`
 * @param initial_pattern
 *    Initial events pattern.
 */
enum TN_RCode tn_eventgrp_create_wattr(
      struct TN_EventGrp  *eventgrp,
      enum TN_EGrpAttr     attr,
      TN_UWord             initial_pattern
      );

/**
 * Construct event group. `id_event` field should not contain `#TN_ID_EVENTGRP`,
 * otherwise, `#TN_RC_WPARAM` is returned.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param eventgrp
 *    Pointer to already allocated struct TN_EventGrp
 * @param initial_pattern
 *    Initial events pattern.
 *
 * @return 
 *    * `#TN_RC_OK` if event group was successfully created;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#TN_RC_WPARAM`.
 */
_TN_STATIC_INLINE enum TN_RCode tn_eventgrp_create(
      struct TN_EventGrp  *eventgrp,
      TN_UWord             initial_pattern
      )
{
   return tn_eventgrp_create_wattr(
         eventgrp,
#if TN_OLD_EVENT_API
         (TN_EVENTGRP_ATTR_MULTI),
#else
         (TN_EVENTGRP_ATTR_NONE),
#endif
         initial_pattern
         );
}

/**
 * Destruct event group.
 * 
 * All tasks that wait for the event(s) become runnable with `#TN_RC_DELETED`
 * code returned.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param eventgrp   Pointer to event groupt to be deleted.
 *
 * @return 
 *    * `#TN_RC_OK` if event group was successfully deleted;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_eventgrp_delete(struct TN_EventGrp *eventgrp);

/**
 * Wait for specified event(s) in the event group. If the specified event
 * is already active, function returns `#TN_RC_OK` immediately. Otherwise,
 * behavior depends on `timeout` value: refer to `#TN_TickCnt`.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_CAN_SLEEP)
 * $(TN_LEGEND_LINK)
 *
 * @param eventgrp
 *    Pointer to event group to wait events from
 * @param wait_pattern
 *    Events bit pattern for which task should wait
 * @param wait_mode
 *    Specifies whether task should wait for **all** the event bits from
 *    `wait_pattern` to be set, or for just **any** of them 
 *    (see enum `#TN_EGrpWaitMode`)
 * @param p_flags_pattern
 *    Pointer to the `TN_UWord` variable in which actual event pattern
 *    that caused task to stop waiting will be stored.
 *    May be `TN_NULL`.
 * @param timeout
 *    refer to `#TN_TickCnt`
 *
 * @return
 *    * `#TN_RC_OK` if specified event is active (so the task can check 
 *      variable pointed to by `p_flags_pattern` if it wasn't `TN_NULL`).
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * Other possible return codes depend on `timeout` value,
 *      refer to `#TN_TickCnt`
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_eventgrp_wait(
      struct TN_EventGrp  *eventgrp,
      TN_UWord             wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      TN_UWord            *p_flags_pattern,
      TN_TickCnt           timeout
      );

/**
 * The same as `tn_eventgrp_wait()` with zero timeout.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_eventgrp_wait_polling(
      struct TN_EventGrp  *eventgrp,
      TN_UWord             wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      TN_UWord            *p_flags_pattern
      );

/**
 * The same as `tn_eventgrp_wait()` with zero timeout, but for using in the ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_eventgrp_iwait_polling(
      struct TN_EventGrp  *eventgrp,
      TN_UWord             wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      TN_UWord            *p_flags_pattern
      );

/**
 * Modify current events bit pattern in the event group. Behavior depends
 * on the given `operation`: refer to `enum #TN_EGrpOp`
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 *
 * @param eventgrp
 *    Pointer to event group to modify events in
 * @param operation
 *    Actual operation to perform: set, clear or toggle.
 *    Refer to `enum #TN_EGrpOp`
 * @param pattern
 *    Events pattern to be applied (depending on `operation` value)
 *
 * @return
 *    * `#TN_RC_OK` on success;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_eventgrp_modify(
      struct TN_EventGrp  *eventgrp,
      enum TN_EGrpOp       operation,
      TN_UWord             pattern
      );

/**
 * The same as `tn_eventgrp_modify()`, but for using in the ISR.
 *
 * $(TN_CALL_FROM_ISR)
 * $(TN_CAN_SWITCH_CONTEXT)
 * $(TN_LEGEND_LINK)
 */
enum TN_RCode tn_eventgrp_imodify(
      struct TN_EventGrp  *eventgrp,
      enum TN_EGrpOp       operation,
      TN_UWord             pattern
      );


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_EVENTGRP_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


