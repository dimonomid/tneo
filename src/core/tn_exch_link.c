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

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- common tnkernel headers
#include "tn_common.h"
#include "tn_sys.h"

//-- internal tnkernel headers
#include "_tn_exch_link.h"


//-- header of current module
#include "tn_exch_link.h"



/*******************************************************************************
 *    PUBLIC DATA
 ******************************************************************************/


/*******************************************************************************
 *    PRIVATE FUNCTION PROTOTYPES
 ******************************************************************************/

static enum TN_RCode _notify_error(struct TN_ExchLink *exch_link);
static enum TN_RCode _dtor(struct TN_ExchLink *exch_link);



/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

/**
 * Virtual methods table of "abstract class" `#TN_ExchLink`.
 */
static const struct TN_ExchLink_VTable _vtable = {
   .notify     = _notify_error,
   .dtor       = _dtor,
};
//static BOOL _vtable_initialized = FALSE;




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/





/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
static inline enum TN_RCode _check_param_generic(
      struct TN_ExchLink *exch_link
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (exch_link == NULL){
      rc = TN_RC_WPARAM;
   } else if (!_tn_exch_link_is_valid(exch_link)){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

static inline enum TN_RCode _check_param_create(
      struct TN_ExchLink  *exch_link
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (exch_link == NULL){
      rc = TN_RC_WPARAM;
   } else if (_tn_exch_link_is_valid(exch_link)){
      rc = TN_RC_WPARAM;
   }

   return rc;
}

#else
#  define _check_param_generic(exch_link)       (TN_RC_OK)
#  define _check_param_create(exch_link)        (TN_RC_OK)
#endif
// }}}

static enum TN_RCode _notify_error(struct TN_ExchLink *exch_link)
{
   //-- should never be here
   _TN_FATAL_ERROR("called notify() of base TN_ExchLink");
   return TN_RC_INTERNAL;
}

static enum TN_RCode _dtor(struct TN_ExchLink *exch_link)
{
   exch_link->id_exch_link = 0;  //-- exchange link does not exist now
   return TN_RC_OK;
}

#if 0
static void _vtable_init()
{
   if (!_vtable_initialized){
      _vtable.notify    = _notify_error;
      _vtable.dtor      = _dtor;

      _vtable_initialized = TRUE;
   }
}
#endif







/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

/**
 * See comments in the _tn_exch_link.h file
 */
const struct TN_ExchLink_VTable *_tn_exch_link_vtable(void)
{
   //_vtable_init();
   return &_vtable;
}


/**
 * See comments in the _tn_exch_link.h file
 */
enum TN_RCode _tn_exch_link_create(
      struct TN_ExchLink     *exch_link
      )
{
   enum TN_RCode rc = _check_param_create(exch_link);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      //_vtable_init();
      exch_link->vtable = &_vtable;

      tn_list_reset(&(exch_link->links_list_item));

      exch_link->id_exch_link = TN_ID_EXCHANGE_LINK;
   }

   return rc;
}


/**
 * See comments in the _tn_exch_link.h file
 */
enum TN_RCode _tn_exch_link_notify(
      struct TN_ExchLink     *exch_link
      )
{
   enum TN_RCode rc = _check_param_generic(exch_link);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      rc = exch_link->vtable->notify(exch_link);
   }

   return rc;
}



/**
 * See comments in the _tn_exch_link.h file
 */
enum TN_RCode _tn_exch_link_delete(
      struct TN_ExchLink     *exch_link
      )
{
   enum TN_RCode rc = _check_param_generic(exch_link);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      rc = exch_link->vtable->dtor(exch_link);
   }

   return rc;
}



