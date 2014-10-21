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

/**
 * \file
 *
 * Exchange
 *
 */


#ifndef _TN_EXCH_H
#define _TN_EXCH_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"
#include "tn_sys.h"



/*******************************************************************************
 *    EXTERN TYPES
 ******************************************************************************/

struct TN_DQueue;
struct TN_FMem;
struct TN_EventGrp;



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_Exch;

typedef void (TN_ExchCallbackFunc)(
      struct TN_Exch   *exch,
      void             *data,
      unsigned int      size,
      void             *p_user_data
      );


/**
 * Exchange
 */
struct TN_Exch {
   ///
   /// List of all connected links (`struct TN_ExchLinkQueue`, etc)
   struct TN_ListItem links_list;
   ///
   /// Pointer to actual exchange data
   void *data;
   ///
   /// Size of the exchange data in bytes, should be a multiple of
   /// `sizeof(#TN_UWord)`
   unsigned int size;
   ///
   /// id for object validity verification
   enum TN_ObjId id_exch;
};

struct TN_ExchLinkQueue {
   ///
   /// A list item to be included in the exchange links list
   struct TN_ListItem links_list_item;
   ///
   /// Poiner to the queue in which exchange should send messages
   struct TN_DQueue *queue;
   ///
   /// Poiner to the fixed memory pool from which memory should be
   /// allocated for the message.
   ///
   /// NOTE: if the size of exchange item is <= `sizeof(void *)`,
   /// fixed memory pool might be not used (`NULL`): then, data
   /// will be passed by the queue only.
   ///
   /// If fixed memory pool is used, its size should match the size of
   /// exchange data.
   struct TN_FMem *fmem;
};

struct TN_ExchLinkEvent {
   ///
   /// A list item to be included in the exchange links list
   struct TN_ListItem links_list_item;
   ///
   /// Poiner to the eventgrp to set event in
   struct TN_EventGrp *eventgrp;
   ///
   /// Flags pattern to set in the given event group
   TN_UWord pattern;
};

struct TN_ExchLinkCallback {
   ///
   /// A list item to be included in the exchange links list
   struct TN_ListItem links_list_item;
   ///
   /// Poiner to the eventgrp to set event in
   TN_ExchCallbackFunc *func;
   ///
   /// User data to be given to callback function
   void *p_user_data;
};


/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Convenience macro for the definition of buffer for data. See
 * `tn_exch_create()` for usage example. TODO: example
 *
 * @param name
 *    C variable name of the buffer array (this name should be given 
 *    to the `tn_exch_create()` function as the `data` argument)
 * @param item_type
 *    Type of item in the memory pool, like `struct MyExchangeData`.
 */
#define TN_EXCH_DATA_BUF_DEF(name, item_type)                     \
   TN_UWord name[                                                 \
      (TN_MAKE_ALIG_SIZE(sizeof(item_type)) / sizeof(TN_UWord))   \
   ]


/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Construct the exchange object. `id_exch` field should not contain
 * `#TN_ID_EXCHANGE`, otherwise, `#TN_RC_WPARAM` is returned.
 *
 * Note that `data` and `size` should be a multiple of `sizeof(#TN_UWord)`.
 *
 * For the definition of buffer, convenience macro `TN_EXCH_DATA_BUF_DEF()`
 * was invented.
 *
 * Typical definition looks as follows:
 *
 * \code{.c}
 *     //-- type of data that exchange object stores
 *     struct MyExchangeData {
 *        // ... arbitrary fields ...
 *     };
 *     
 *     //-- define buffer for memory pool
 *     TN_EXCH_DATA_BUF_DEF(my_exch_buf, struct MyExchangeData);
 *
 *     //-- define memory pool structure
 *     struct TN_Exch my_exch;
 * \endcode
 *
 * And then, construct your `my_exch` as follows:
 *
 * \code{.c}
 *     enum TN_RCode rc;
 *     rc = tn_exch_create( &my_exch,
 *                          my_exch_buf,
 *                          TN_MAKE_ALIG_SIZE(sizeof(struct MyExchangeData))
 *                        );
 *     if (rc != TN_RC_OK){
 *        //-- handle error
 *     }
 * \endcode
 *
 * If given `data` and/or `size` aren't aligned properly, `#TN_RC_WPARAM` is
 * returned.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_CALL_FROM_ISR)
 * $(TN_LEGEND_LINK)
 *
 * @param exch
 *    Pointer to already allocated `struct TN_Exch`
 * @param data
 *    Pointer to already allocated exchange data buffer, its size must be a
 *    multiple of `sizeof(#TN_UWord)`
 * @param size
 *    Size of the exchange data buffer in bytes, must be a multiple of
 *    `sizeof(#TN_UWord)`
 *
 * @return 
 *    * `#TN_RC_OK` if exchange object was successfully created;
 *    * `#TN_RC_WPARAM` if wrong params were given.
 */
enum TN_RCode tn_exch_create(
      struct TN_Exch   *exch,
      void             *data,
      unsigned int      size
      );

/**
 * Destruct the exchange object.
 *
 * $(TN_CALL_FROM_TASK)
 * $(TN_LEGEND_LINK)
 *
 * @param exch     exchange object to destruct
 *
 * @return 
 *    * `#TN_RC_OK` if object was successfully deleted;
 *    * `#TN_RC_WCONTEXT` if called from wrong context;
 *    * If `#TN_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#TN_RC_WPARAM` and `#TN_RC_INVALID_OBJ`.
 */
enum TN_RCode tn_exch_delete(struct TN_Exch *exch);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _TN_EXCH_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


