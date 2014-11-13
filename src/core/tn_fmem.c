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
#include "_tn_tasks.h"
#include "_tn_list.h"


//-- header of current module
#include "tn_fmem.h"
#include "_tn_fmem.h"

//-- header of other needed modules
#include "tn_tasks.h"



/*******************************************************************************
 *    EXTERNAL DATA
 ******************************************************************************/




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if TN_CHECK_PARAM
static _TN_INLINE enum TN_RCode _check_param_fmem_create(struct TN_FMem *fmem)
{
   enum TN_RCode rc = TN_RC_OK;

   if (fmem == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (_tn_fmem_is_valid(fmem)){
      rc = TN_RC_WPARAM;
   }

   return rc;
}

static _TN_INLINE enum TN_RCode _check_param_fmem_delete(struct TN_FMem *fmem)
{
   enum TN_RCode rc = TN_RC_OK;

   if (fmem == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (!_tn_fmem_is_valid(fmem)){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}

static _TN_INLINE enum TN_RCode _check_param_job_perform(
      struct TN_FMem *fmem,
      void *p_data
      )
{
   enum TN_RCode rc = TN_RC_OK;

   if (fmem == TN_NULL || p_data == TN_NULL){
      rc = TN_RC_WPARAM;
   } else if (!_tn_fmem_is_valid(fmem)){
      rc = TN_RC_INVALID_OBJ;
   }

   return rc;
}
#else
#  define _check_param_fmem_create(fmem)               (TN_RC_OK)
#  define _check_param_fmem_delete(fmem)               (TN_RC_OK)
#  define _check_param_job_perform(fmem, p_data)       (TN_RC_OK)
#endif
// }}}

static void _cb_before_task_wait_complete(
      struct TN_Task   *task,
      void             *user_data_1,
      void             *user_data_2
      )
{
   task->subsys_wait.fmem.data_elem = user_data_1;
}

static _TN_INLINE enum TN_RCode _fmem_get(struct TN_FMem *fmem, void **p_data)
{
   enum TN_RCode rc;
   void *ptr = TN_NULL;

   if (fmem->free_blocks_cnt > 0){
      ptr = fmem->free_list;
      fmem->free_list = *(void **)fmem->free_list;   //-- ptr - to new free list
      fmem->free_blocks_cnt--;

      *p_data = ptr;
      rc = TN_RC_OK;
   } else {
      rc = TN_RC_TIMEOUT;
   }

   return rc;
}

static _TN_INLINE enum TN_RCode _fmem_release(struct TN_FMem *fmem, void *p_data)
{
   enum TN_RCode rc = TN_RC_OK;

   if (  !_tn_task_first_wait_complete(
            &fmem->wait_queue, TN_RC_OK,
            _cb_before_task_wait_complete, p_data, TN_NULL
            )
      )
   {
      //-- no task is waiting for free memory block, so,
      //   insert in to the memory pool

      if (fmem->free_blocks_cnt < fmem->blocks_cnt){
         //-- insert block into free block list
         *(void **)p_data = fmem->free_list;
         fmem->free_list = p_data;
         fmem->free_blocks_cnt++;
      } else {
#if TN_DEBUG
         if (fmem->free_blocks_cnt > fmem->blocks_cnt){
            _TN_FATAL_ERROR(
                  "free_blocks_cnt should never be more than blocks_cnt"
                  );
         }
#endif
         //-- the memory pool already has all the blocks free
         rc = TN_RC_OVERFLOW;
      }
   }

   return rc;
}





/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_fmem_create(
      struct TN_FMem   *fmem,
      void             *start_addr,
      unsigned int      block_size,
      int               blocks_cnt
      )
{
   enum TN_RCode rc;

   rc = _check_param_fmem_create(fmem);
   if (rc != TN_RC_OK){
      goto out;
   }

   //-- basic check: start_addr should not be TN_NULL,
   //   and blocks_cnt should be at least 2
   if (start_addr == TN_NULL || blocks_cnt < 2){
      rc = TN_RC_WPARAM;
      goto out;
   }

   //-- check that start_addr is aligned properly
   {
      TN_UIntPtr start_addr_aligned 
         = TN_MAKE_ALIG_SIZE((TN_UIntPtr)start_addr);

      if (start_addr_aligned != (TN_UIntPtr)start_addr){
         rc = TN_RC_WPARAM;
         goto out;
      }
   }

   //-- check that block_size is aligned properly
   {
      unsigned int block_size_aligned = TN_MAKE_ALIG_SIZE(block_size);
      if (block_size_aligned != block_size){
         rc = TN_RC_WPARAM;
         goto out;
      }
   }

   //-- checks are done; proceed to actual creation

   fmem->start_addr = start_addr;
   fmem->block_size = block_size;
   fmem->blocks_cnt = blocks_cnt;

   //-- reset wait_queue
   _tn_list_reset(&(fmem->wait_queue));

   //-- init block pointers
   {
      void **p_tmp;
      unsigned char *p_block;
      int i;

      p_tmp    = (void **)fmem->start_addr;
      p_block  = (unsigned char *)fmem->start_addr + fmem->block_size;
      for (i = 0; i < (fmem->blocks_cnt - 1); i++){
         *p_tmp  = (void *)p_block;  //-- contents of cell = addr of next block
         p_tmp   = (void **)p_block;
         p_block += fmem->block_size;
      }
      *p_tmp = TN_NULL;          //-- Last memory block first cell contents -  TN_NULL

      fmem->free_list       = fmem->start_addr;
      fmem->free_blocks_cnt = fmem->blocks_cnt;
   }

   //-- set id
   fmem->id_fmp = TN_ID_FSMEMORYPOOL;

out:
   return rc;
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_fmem_delete(struct TN_FMem *fmem)
{
   enum TN_RCode rc = _check_param_fmem_delete(fmem);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      //-- remove all tasks (if any) from fmem's wait queue
      _tn_wait_queue_notify_deleted(&(fmem->wait_queue));

      fmem->id_fmp = TN_ID_NONE;   //-- Fixed-size memory pool does not exist now

      TN_INT_RESTORE();

      //-- we might need to switch context if _tn_wait_queue_notify_deleted()
      //   has woken up some high-priority task
      _tn_context_switch_pend_if_needed();

   }
   return rc;
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_fmem_get(
      struct TN_FMem *fmem,
      void **p_data,
      TN_Timeout timeout
      )
{
   TN_BOOL waited_for_data = TN_FALSE;
   enum TN_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      rc = _fmem_get(fmem, p_data);

      if (rc == TN_RC_TIMEOUT && timeout > 0){
         _tn_task_curr_to_wait_action(
               &(fmem->wait_queue),
               TN_WAIT_REASON_WFIXMEM,
               timeout
               );
         waited_for_data = TN_TRUE;
      }

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();
      if (waited_for_data){

         //-- get wait result
         rc = tn_curr_run_task->task_wait_rc;

         //-- if wait result is TN_RC_OK, copy memory block pointer to the
         //   user's location
         if (rc == TN_RC_OK){
            *p_data = tn_curr_run_task->subsys_wait.fmem.data_elem;
         }

      }

   }
   return rc;
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_fmem_get_polling(struct TN_FMem *fmem,void **p_data)
{
   enum TN_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();
      rc = _fmem_get(fmem, p_data);
      TN_INT_RESTORE();
   }

   return rc;
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_fmem_iget_polling(struct TN_FMem *fmem, void **p_data)
{
   enum TN_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA_INT;

      TN_INT_IDIS_SAVE();

      rc = _fmem_get(fmem, p_data);

      TN_INT_IRESTORE();
      _TN_CONTEXT_SWITCH_IPEND_IF_NEEDED();
   }
   return rc;
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_fmem_release(struct TN_FMem *fmem, void *p_data)
{
   enum TN_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_task_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA;

      TN_INT_DIS_SAVE();

      rc = _fmem_release(fmem, p_data);

      TN_INT_RESTORE();
      _tn_context_switch_pend_if_needed();
   }
   return rc;
}


/*
 * See comments in the header file (tn_dqueue.h)
 */
enum TN_RCode tn_fmem_irelease(struct TN_FMem *fmem, void *p_data)
{
   enum TN_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != TN_RC_OK){
      //-- just return rc as it is
   } else if (!tn_is_isr_context()){
      rc = TN_RC_WCONTEXT;
   } else {
      TN_INTSAVE_DATA_INT;

      TN_INT_IDIS_SAVE();

      rc = _fmem_release(fmem, p_data);

      TN_INT_IRESTORE();
      _TN_CONTEXT_SWITCH_IPEND_IF_NEEDED();
   }

   return rc;
}


