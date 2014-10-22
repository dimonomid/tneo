/*******************************************************************************
 *
 * TNeoKernel: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeoKernel:                copyright © 2014       Dmitry Frank.
 *
 *    TNeoKernel was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
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

#ifndef __TN_EXCH_LINK_H
#define __TN_EXCH_LINK_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "_tn_sys.h"
#include "tn_exch_link.h"




#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    EXTERNAL TYPES
 ******************************************************************************/

struct TN_ExchLink;



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/


/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/

const struct TN_ExchLink_VTable *_tn_exch_link_vtable(void);

enum TN_RCode _tn_exch_link_create(
      struct TN_ExchLink     *exch_link
      );

enum TN_RCode _tn_exch_link_notify(
      struct TN_ExchLink     *exch_link
      );

enum TN_RCode _tn_exch_link_delete(
      struct TN_ExchLink     *exch_link
      );




/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Checks whether given exchange link object is valid 
 * (actually, just checks against `id_exch_link` field, see `enum #TN_ObjId`)
 */
static inline BOOL _tn_exch_link_is_valid(
      struct TN_ExchLink   *exch_link
      )
{
   return (exch_link->id_exch_link == TN_ID_EXCHANGE_LINK);
}



#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // __TN_EXCH_LINK_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


