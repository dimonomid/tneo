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

#ifndef __TN_EVENTGRP_H
#define __TN_EVENTGRP_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "_tn_sys.h"





#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/


/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Establish link to the event group. 
 *
 * \attention Caller must disable interrupts.
 *
 * @param eventgrp_link    
 *    eventgrp_link object which should be modified. The structure
 *    `eventgrp_link` is typically contained in some other structure,
 *    for example, #TN_DQueue.
 *
 * @param eventgrp
 *    Event group object to connect
 *
 * @param pattern
 *    Flags pattern that should be maintained by object to which
 *    event group is being connected. Can't be 0.
 */
enum TN_RCode _tn_eventgrp_link_set(
      struct TN_EGrpLink  *eventgrp_link,
      struct TN_EventGrp  *eventgrp,
      TN_UWord             pattern
      );

/**
 * Reset link to the event group, i.e. make it non connected to any event
 * group. (no matter whether it is already established).
 */
enum TN_RCode _tn_eventgrp_link_reset(
      struct TN_EGrpLink  *eventgrp_link
      );

/**
 * Set or clear flag(s) in the connected event group, if any. Flag(s) are
 * specified by `pattern` argument given to `#_tn_eventgrp_link_set()`.
 *
 * If given `eventgrp_link` doesn't contain any connected event group, nothing
 * is performed and no error returned.
 *
 * \attention Caller must disable interrupts.
 *
 * @param eventgrp_link    
 *    eventgrp_link object which contains connected event group.
 *    `eventgrp_link` is typically contained in some other structure,
 *    for example, #TN_DQueue.
 *
 * @param set
 *    - if `TN_TRUE`, flag(s) are being set;
 *    - if `TN_FALSE`, flag(s) are being cleared.
 *
 */
enum TN_RCode _tn_eventgrp_link_manage(
      struct TN_EGrpLink  *eventgrp_link,
      TN_BOOL              set
      );



/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Checks whether given event group object is valid 
 * (actually, just checks against `id_event` field, see `enum #TN_ObjId`)
 */
_TN_STATIC_INLINE TN_BOOL _tn_eventgrp_is_valid(
      const struct TN_EventGrp   *eventgrp
      )
{
   return (eventgrp->id_event == TN_ID_EVENTGRP);
}




#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // __TN_EVENTGRP_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


