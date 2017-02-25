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
 * Exchange link (in terms of OOP, it's an "abstract class" of any exchange
 * link).
 *
 * For internal kernel usage only.
 *
 */


#ifndef _TN_EXCH_LINK_H
#define _TN_EXCH_LINK_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_common.h"
#include "tn_list.h"




#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_ExchLink;


/**
 * Virtual method prototype: notify.
 *
 * For internal kernel usage only.
 */
typedef enum TN_RCode (TN_ExchLink_Notify)(struct TN_ExchLink *exch);

/**
 * Virtual method prototype: destructor.
 *
 * For internal kernel usage only.
 */
typedef enum TN_RCode (TN_ExchLink_Dtor)  (struct TN_ExchLink *exch);

/**
 * Virtual methods table for each type of \ref tn_exch.h "exchange" link. 
 *
 * For internal kernel usage only.
 */
struct TN_ExchLink_VTable {
   TN_ExchLink_Notify  *notify;
   TN_ExchLink_Dtor    *dtor;
};

/**
 * Base structure for \ref tn_exch.h "exchange" link.
 *
 * For internal kernel usage only.
 */
struct TN_ExchLink {
   ///
   /// A list item to be included in the exchange links list
   struct TN_ListItem links_list_item;
   ///
   /// Pointer to `#TN_Exch` object to which this link is added.
   /// If not added to any exchange object, it is `TN_NULL`.
   struct TN_Exch *exch;
   ///
   /// Pointer to the virtual methods table
   const struct TN_ExchLink_VTable *vtable;
   ///
   /// Id for object validity verification
   enum TN_ObjId              id_exch_link;
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



#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_EXCH_LINK_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


