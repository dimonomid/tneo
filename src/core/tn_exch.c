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
#include "tn_exch_link.h"

//-- internal tnkernel headers
#include "_tn_sys.h"
#include "_tn_exch.h"
#include "_tn_exch_link.h"


//-- header of current module
#include "tn_exch.h"





/*******************************************************************************
 *    PUBLIC DATA
 ******************************************************************************/




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/





/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
static inline enum TN_RCode _check_param_generic(
      struct TN_Exch *exch
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (exch == NULL){
      rc = TN_RC_WPARAM;
   } else if (exch->id_exch != TN_ID_EXCHANGE){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

static inline enum TN_RCode _check_param_create(
      struct TN_Exch  *exch
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (exch == NULL){
      rc = TN_RC_WPARAM;
   } else if (exch->id_exch == TN_ID_EXCHANGE){
      rc = TN_RC_WPARAM;
   }

   return rc;
}

#else
#  define _check_param_generic(exch)            (TN_RC_OK)
#  define _check_param_create(exch)             (TN_RC_OK)
#endif
// }}}

static enum TN_RCode _notify_all(
      struct TN_Exch   *exch
      )
{
   enum TN_RCode rc = TN_RC_OK;
   struct TN_ExchLink *exch_link;
   
   tn_list_for_each_entry(exch_link, &(exch->links_list), links_list_item){
      rc = _tn_exch_link_notify(exch_link);
      if (rc != TN_RC_OK){
         break;
      }
   }

   return rc;
}



/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (tn_exch.h)
 */
enum TN_RCode tn_exch_create(
      struct TN_Exch   *exch,
      void             *data,
      unsigned int      size
      )
{
   enum TN_RCode rc = _check_param_create(exch);
   if (rc != TN_RC_OK){
      goto out;
   }

   //-- basic check: data should not be NULL,
   //   and size should not be 0
   if (data == NULL || size == 0){
      rc = TN_RC_WPARAM;
      goto out;
   }


   //-- check that `data` is aligned properly
   {
      unsigned long data_aligned 
         = TN_MAKE_ALIG_SIZE((unsigned long)data);

      if (data_aligned != (unsigned int)data){
         rc = TN_RC_WPARAM;
         goto out;
      }
   }

   //-- check that `size` is aligned properly
   {
      unsigned int size_aligned = TN_MAKE_ALIG_SIZE(size);
      if (size_aligned != size){
         rc = TN_RC_WPARAM;
         goto out;
      }
   }

   //-- checks are done; proceed to actual creation
   exch->data = data;
   exch->size = size;

   //-- reset links_list
   tn_list_reset(&(exch->links_list));

   //-- set id
   exch->id_exch = TN_ID_EXCHANGE;

out:
   return rc;
}

/*
 * See comments in the header file (tn_exch.h)
 */
enum TN_RCode tn_exch_delete(struct TN_Exch *exch)
{
   enum TN_RCode rc = _check_param_generic(exch);
   int sr_saved;

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      sr_saved = tn_arch_sr_save_int_dis();
      exch->id_exch = 0;   //-- Exchange object does not exist now
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}

/**
 * See comments in the header file (tn_exch.h)
 */
enum TN_RCode tn_exch_read(
      struct TN_Exch   *exch,
      void             *data_tgt
      )
{
   int sr_saved;
   enum TN_RCode rc = _check_param_generic(exch);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      sr_saved = tn_arch_sr_save_int_dis();
      rc = _tn_exch_read(exch, data_tgt);
      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}


/**
 * See comments in the header file (tn_exch.h)
 */
enum TN_RCode tn_exch_write(
      struct TN_Exch   *exch,
      const void       *data
      )
{
   enum TN_RCode rc = _check_param_generic(exch);
   int sr_saved;

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      sr_saved = tn_arch_sr_save_int_dis();

      _tn_exch_write(exch, data);
      rc = _notify_all(exch);

      tn_arch_sr_restore(sr_saved);
   }

   return rc;
}





/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

/**
 * See comments in the _tn_exch.h file
 */
void _tn_exch_write(
      struct TN_Exch   *exch,
      const void       *data
      )
{
   _tn_memcpy_uword(
         (TN_UWord *)exch->data,
         (const TN_UWord *)data,
         _TN_SIZE_BYTES_TO_UWORDS(exch->size)
         );
}


/**
 * See comments in the _tn_exch.h file
 */
enum TN_RCode _tn_exch_read(
      struct TN_Exch   *exch,
      void             *data_tgt
      )
{
   _tn_memcpy_uword(
         (TN_UWord *)data_tgt,
         (const TN_UWord *)exch->data,
         _TN_SIZE_BYTES_TO_UWORDS(exch->size)
         );

   return TN_RC_OK;
}



