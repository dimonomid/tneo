/*******************************************************************************
 *
 * TNeo: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeo:                      copyright © 2014       Dmitry Frank.
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
 * Exchange link: queue (in terms of OOP, it's an "class, inherited from
 * `#TN_ExchLink`").
 *
 * For internal kernel usage only.
 *
 */


#ifndef _TN_EXCH_LINK_QUEUE_H
#define _TN_EXCH_LINK_QUEUE_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_exch_link.h"




#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_ExchLinkQueue;


/**
 * Base structure for \ref tn_exch.h "exchange" link.
 *
 * For internal kernel usage only.
 */
struct TN_ExchLinkQueue {
   ///
   /// Exchange link: in terms of OOP, it's a superclass (or base class)
   struct TN_ExchLink super;
   ///
   /// A pointer to queue to send messages to.
   struct TN_DQueue *queue;
   ///
   /// A pointer to fixed-memory pool to get memory from.
   /// Note: if data size is <= `sizeof(#TN_UWord)`, `fmem` might be `TN_NULL`.
   struct TN_FMem *fmem;
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

enum TN_RCode tn_exch_link_queue_create(
      struct TN_ExchLinkQueue   *exch_link_queue,
      struct TN_DQueue          *queue,
      struct TN_FMem            *fmem
      );

enum TN_RCode tn_exch_link_queue_delete(
      struct TN_ExchLinkQueue   *exch_link_queue
      );

static inline struct TN_ExchLink *tn_exch_link_queue_base_get(
      struct TN_ExchLinkQueue   *exch_link_queue
      )
{
   return &exch_link_queue->super;
}



#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_EXCH_LINK_QUEUE_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


