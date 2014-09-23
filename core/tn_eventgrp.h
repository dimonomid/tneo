/*******************************************************************************
 *
 * TNeoKernel: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeoKernel:                copyright © 2014       Dmitry Frank.
 *
 *    TNeoKernel was born as a thorough review and re-implementation 
 *    of TNKernel. New kernel has well-formed code, bugs of ancestor are fixed
 *    as well as new features added, and it is tested carefully with unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeoKernel.
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
 *   Description:   TODO
 *
 */

#ifndef _TN_EVENTGRP_H
#define _TN_EVENTGRP_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Events waiting mode: wait for all flags to be set or just for any of the 
 * specified flags to be set.
 */
enum TN_EGrpWaitMode {
   /// any set flag is enough for releasing task from waiting
   TN_EVENTGRP_WMODE_OR     = (1 << 0),
   /// all flags must be set for releasing task from waiting
   TN_EVENTGRP_WMODE_AND    = (1 << 1),
};

/**
 * Modify operation: set, clear or toggle
 */
enum TN_EGrpOp {
   TN_EVENTGRP_OP_SET,      //!< set flags that are set in pattern argument
   TN_EVENTGRP_OP_CLEAR,    //!< clear flags that are set in pattern argument
   TN_EVENTGRP_OP_TOGGLE,   //!< toggle flags that are set in pattern argument
};

/**
 * Event group
 */
struct TN_EventGrp {
   struct TN_ListItem   wait_queue; //!< task wait queue
   unsigned int         pattern;    //!< current flags pattern
   enum TN_ObjId        id_event;   //!< id for object validity verification
};

/**
 * EventGrp-specific fields for struct TN_Task.
 */
struct TN_EGrpTaskWait {
   ///
   /// event wait pattern (relevant if only `task_state` is `WAIT` or
   /// `WAITSUSP`, and `task_wait_reason` is `EVENT`)
   /// @see enum TN_TaskState
   /// @see enum TN_WaitReason
   int  wait_pattern;
   ///
   /// event wait mode: `AND` or `OR`
   enum TN_EGrpWaitMode wait_mode;
   ///
   /// pattern that caused task to finish waiting
   int  actual_pattern;
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

enum TN_RCode tn_eventgrp_create(
      struct TN_EventGrp *eventgrp,
      unsigned int initial_pattern
      );

enum TN_RCode tn_eventgrp_delete(struct TN_EventGrp *eventgrp);

enum TN_RCode tn_eventgrp_wait(
      struct TN_EventGrp  *eventgrp,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern,
      unsigned long        timeout
      );

enum TN_RCode tn_eventgrp_wait_polling(
      struct TN_EventGrp  *eventgrp,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern
      );

enum TN_RCode tn_eventgrp_iwait_polling(
      struct TN_EventGrp  *eventgrp,
      unsigned int         wait_pattern,
      enum TN_EGrpWaitMode wait_mode,
      unsigned int        *p_flags_pattern
      );

enum TN_RCode tn_eventgrp_modify(
      struct TN_EventGrp  *eventgrp,
      enum TN_EGrpOp       operation,
      unsigned int         pattern
      );

enum TN_RCode tn_eventgrp_imodify(
      struct TN_EventGrp  *eventgrp,
      enum TN_EGrpOp       operation,
      unsigned int         pattern
      );

#endif // _TN_EVENTGRP_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


