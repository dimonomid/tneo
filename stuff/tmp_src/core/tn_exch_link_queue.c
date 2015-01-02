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

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- common tnkernel headers
#include "tn_common.h"
#include "tn_sys.h"

#include "tn_exch.h"

//-- internal tnkernel headers
#include "_tn_exch_link.h"
#include "_tn_dqueue.h"
#include "_tn_fmem.h"


//-- header of current module
#include "tn_exch_link_queue.h"



/*******************************************************************************
 *    PUBLIC DATA
 ******************************************************************************/


/*******************************************************************************
 *    PRIVATE FUNCTION PROTOTYPES
 ******************************************************************************/

static enum TN_RCode _notify(struct TN_ExchLink *exch_link);
static enum TN_RCode _dtor(struct TN_ExchLink *exch_link);



/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

/**
 * Virtual methods table
 */
static const struct TN_ExchLink_VTable _vtable = {
   .notify     = _notify,
   .dtor       = _dtor,
};




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define _tn_get_exch_link_queue_by_exch_link(exch_link)                       \
   (exch_link ? container_of(exch_link, struct TN_ExchLinkQueue, super) : 0)





/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
static inline enum TN_RCode _check_param_create(
      struct TN_ExchLinkQueue   *exch_link_queue,
      struct TN_DQueue          *queue,
      struct TN_FMem            *fmem
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (exch_link_queue == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (!_tn_dqueue_is_valid(queue)){
      rc = TN_RC_WPARAM;
   } else if (!_tn_fmem_is_valid(fmem)){
      rc = TN_RC_WPARAM;
   }

   return rc;
}

#else
#  define _check_param_create(exch_link)        (TN_RC_OK)
#endif
// }}}


static enum TN_RCode _notify(struct TN_ExchLink *exch_link)
{
   enum TN_RCode rc = TN_RC_OK;

   struct TN_ExchLinkQueue *exch_link_queue = 
      _tn_get_exch_link_queue_by_exch_link(exch_link);

   void *p_msg = TN_NULL;

   rc = tn_is_task_context()
      ? tn_fmem_get(exch_link_queue->fmem, &p_msg, TN_WAIT_INFINITE)
      : tn_fmem_iget_polling(exch_link_queue->fmem, &p_msg);

   if (rc != TN_RC_OK){
      //-- there was some error: just return rc as it is
   } else {
      //-- memory was received from fixed-memory pool, copy data there
      _tn_memcpy_uword(
            p_msg,
            exch_link->exch->data,
            _TN_SIZE_BYTES_TO_UWORDS(exch_link->exch->size)
            );

      //-- put it to the queue
      rc = tn_is_task_context()
         ? tn_queue_send(exch_link_queue->queue, p_msg, TN_WAIT_INFINITE)
         : tn_queue_isend_polling(exch_link_queue->queue, p_msg);
      if (rc != TN_RC_OK){
         //-- there was some error while sending the message,
         //   so before we return, we should free buffer that we've allocated
         rc = tn_is_task_context()
            ? tn_fmem_release(exch_link_queue->fmem, p_msg)
            : tn_fmem_irelease(exch_link_queue->fmem, p_msg);
      } else {
         //-- everything is fine, so, leave rc = TN_RC_OK
      }

   }

   return rc;
}

static enum TN_RCode _dtor(struct TN_ExchLink *exch_link)
{
   //-- just call desctructor of superclass
   return _tn_exch_link_vtable()->dtor(exch_link);
}







/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

enum TN_RCode tn_exch_link_queue_create(
      struct TN_ExchLinkQueue   *exch_link_queue,
      struct TN_DQueue          *queue,
      struct TN_FMem            *fmem
      )
{
   enum TN_RCode rc = _check_param_create(exch_link_queue, queue, fmem);

   if (rc == TN_RC_OK){
      //-- call constructor of superclass
      rc = _tn_exch_link_create(&exch_link_queue->super);
   }
      
   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else {
      //-- set the virtual functions table of this particular subclass
      exch_link_queue->super.vtable = &_vtable;

      exch_link_queue->queue = queue;
      exch_link_queue->fmem  = fmem;
   }

   return rc;
}

enum TN_RCode tn_exch_link_queue_delete(
      struct TN_ExchLinkQueue   *exch_link_queue
      )
{
   return _tn_exch_link_delete(&exch_link_queue->super);
}



