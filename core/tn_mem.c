/*

  TNKernel real-time kernel

  Copyright © 2004, 2013 Yuri Tiomkin
  All rights reserved.

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

/* ver 2.7  */

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/


//-- common tnkernel headers
#include "tn_common.h"
#include "tn_sys.h"
#include "tn_internal.h"

//-- header of current module
#include "tn_mem.h"

//-- header of other needed modules
#include "tn_tasks.h"



/*******************************************************************************
 *    EXTERNAL DATA
 ******************************************************************************/




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

static inline enum TN_Retval _fmem_get(struct TN_Fmp *fmp, void **p_data)
{
   enum TN_Retval rc;
   void *ptr = NULL;

   if (fmp->free_blocks_cnt > 0){
      ptr = fmp->free_list;
      fmp->free_list = *(void **)fmp->free_list;   //-- ptr - to new free list
      fmp->free_blocks_cnt--;
   }

   if (ptr != NULL){
      *p_data = ptr;
      rc = TERR_NO_ERR;
   } else {
      rc = TERR_TIMEOUT;
   }

   return rc;
}

static inline enum TN_Retval _fmem_release(struct TN_Fmp *fmp, void *p_data)
{
   struct TN_Task *task;

   enum TN_Retval rc = TERR_NO_ERR;

   if (!tn_is_list_empty(&(fmp->wait_queue))){
      task = tn_list_first_entry(&(fmp->wait_queue), typeof(*task), task_queue);

      task->data_elem = p_data;

      _tn_task_wait_complete(task, TERR_NO_ERR);
   } else {
      if (fmp->free_blocks_cnt < fmp->blocks_cnt){
         *(void **)p_data = fmp->free_list;   //-- insert block into free block list
         fmp->free_list = p_data;
         fmp->free_blocks_cnt++;
      } else {
         rc = TERR_OVERFLOW;
      }
   }

   return rc;
}



#if TN_CHECK_PARAM
static inline enum TN_Retval _check_param_fmem_create(struct TN_Fmp *fmp)
{
   enum TN_Retval rc = TERR_NO_ERR;

   if (fmp == NULL){
      rc = TERR_WRONG_PARAM;
   } else if (fmp->id_fmp == TN_ID_FSMEMORYPOOL){
      rc = TERR_WRONG_PARAM;
   }

   return rc;
}

static inline enum TN_Retval _check_param_fmem_delete(struct TN_Fmp *fmp)
{
   enum TN_Retval rc = TERR_NO_ERR;

   if (fmp == NULL){
      rc = TERR_WRONG_PARAM;
   } else if (fmp->id_fmp != TN_ID_FSMEMORYPOOL){
      rc = TERR_NOEXS;
   }

   return rc;
}

static inline enum TN_Retval _check_param_fmem_get(struct TN_Fmp *fmp, void **p_data)
{
   enum TN_Retval rc = TERR_NO_ERR;

   if (fmp == NULL || p_data == NULL){
      rc = TERR_WRONG_PARAM;
   } else if (fmp->id_fmp != TN_ID_FSMEMORYPOOL){
      rc = TERR_NOEXS;
   }

   return rc;
}

static inline enum TN_Retval _check_param_fmem_release(struct TN_Fmp *fmp, void *p_data)
{
   enum TN_Retval rc = TERR_NO_ERR;

   if (fmp == NULL || p_data == NULL){
      rc = TERR_WRONG_PARAM;
   } else if (fmp->id_fmp != TN_ID_FSMEMORYPOOL){
      rc = TERR_NOEXS;
   }

   return rc;
}
#else
#  define _check_param_fmem_create(fmp)               (TERR_NO_ERR)
#  define _check_param_fmem_delete(fmp)               (TERR_NO_ERR)
#  define _check_param_fmem_get(fmp, p_data)          (TERR_NO_ERR)
#  define _check_param_fmem_release(fmp, p_data)      (TERR_NO_ERR)
#endif



/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

enum TN_Retval tn_fmem_create(
      struct TN_Fmp    *fmp,
      void             *start_addr,
      unsigned int      block_size,
      int               blocks_cnt
      )
{
   enum TN_Retval rc;

   rc = _check_param_fmem_create(fmp);
   if (rc != TERR_NO_ERR){
      goto out;
   }

   //-- basic check: start_addr should not be NULL,
   //   and blocks_cnt should be at least 2
   if (start_addr == NULL || blocks_cnt < 2){
      rc = TERR_WRONG_PARAM;
      goto out;
   }

   //-- check that start_addr is aligned properly
   {
      unsigned long start_addr_aligned = TN_MAKE_ALIG_SIZE((unsigned long)start_addr);
      if (start_addr_aligned != (unsigned int)start_addr){
         rc = TERR_WRONG_PARAM;
         goto out;
      }
   }

   //-- check that block_size is aligned properly
   {
      unsigned int block_size_aligned = TN_MAKE_ALIG_SIZE(block_size);
      if (block_size_aligned != block_size){
         rc = TERR_WRONG_PARAM;
         goto out;
      }
   }

   //-- checks are done; proceed to actual creation

   fmp->start_addr = start_addr;
   fmp->block_size = block_size;
   fmp->blocks_cnt = blocks_cnt;

   //-- reset wait_queue
   tn_list_reset(&(fmp->wait_queue));

   //-- init block pointers
   {
      void **p_tmp;
      unsigned char *p_block;
      int i;

      p_tmp    = (void **)fmp->start_addr;
      p_block  = (unsigned char *)fmp->start_addr + fmp->block_size;
      for (i = 0; i < (fmp->blocks_cnt - 1); i++){
         *p_tmp  = (void *)p_block;  //-- contents of cell = addr of next block
         p_tmp   = (void **)p_block;
         p_block += fmp->block_size;
      }
      *p_tmp = NULL;          //-- Last memory block first cell contents -  NULL

      fmp->free_list       = fmp->start_addr;
      fmp->free_blocks_cnt = fmp->blocks_cnt;
   }

   //-- set id
   fmp->id_fmp = TN_ID_FSMEMORYPOOL;

out:
   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_fmem_delete(struct TN_Fmp *fmp)
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc;
   
   rc = _check_param_fmem_delete(fmp);
   if (rc != TERR_NO_ERR){
      goto out;
   }

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   //-- remove all tasks (if any) from fmp's wait queue
   _tn_wait_queue_notify_deleted(&(fmp->wait_queue));

   fmp->id_fmp = 0;   //-- Fixed-size memory pool does not exist now

   tn_enable_interrupt();

   //-- we might need to switch context if _tn_wait_queue_notify_deleted()
   //   has woken up some high-priority task
   _tn_switch_context_if_needed();

out:
   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_fmem_get(struct TN_Fmp *fmp, void **p_data, unsigned long timeout)
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc;
   BOOL waited_for_data = FALSE;
   
   rc = _check_param_fmem_get(fmp, p_data);
   if (rc != TERR_NO_ERR){
      goto out;
   }

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = _fmem_get(fmp, p_data);

   if (rc == TERR_TIMEOUT && timeout > 0){
      _tn_task_curr_to_wait_action(
            &(fmp->wait_queue),
            TSK_WAIT_REASON_WFIXMEM,
            timeout
            );
      waited_for_data = TRUE;
   }

   tn_enable_interrupt();
   _tn_switch_context_if_needed();
   if (waited_for_data){
      //-- now, data_elem field should contain valid value, so,
      //   return it to caller.
      *p_data = tn_curr_run_task->data_elem;

      rc = tn_curr_run_task->task_wait_rc;
   }

out:
   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_fmem_get_polling(struct TN_Fmp *fmp,void **p_data)
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc;

   rc = _check_param_fmem_get(fmp, p_data);
   if (rc != TERR_NO_ERR){
      goto out;
   }

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = _fmem_get(fmp, p_data);

   tn_enable_interrupt();

out:
   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_fmem_get_ipolling(struct TN_Fmp *fmp, void **p_data)
{
   TN_INTSAVE_DATA_INT;
   enum TN_Retval rc;

   rc = _check_param_fmem_get(fmp, p_data);
   if (rc != TERR_NO_ERR){
      goto out;
   }

   TN_CHECK_INT_CONTEXT;

   tn_idisable_interrupt();

   rc = _fmem_get(fmp, p_data);

   tn_ienable_interrupt();

out:
   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_fmem_release(struct TN_Fmp *fmp, void *p_data)
{
   TN_INTSAVE_DATA;
   enum TN_Retval rc;

   rc = _check_param_fmem_release(fmp, p_data);
   if (rc != TERR_NO_ERR){
      goto out;
   }

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = _fmem_release(fmp, p_data);

   tn_enable_interrupt();
   _tn_switch_context_if_needed();

out:
   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_fmem_irelease(struct TN_Fmp *fmp, void *p_data)
{
   TN_INTSAVE_DATA_INT;
   enum TN_Retval rc;
   
   rc = _check_param_fmem_release(fmp, p_data);
   if (rc != TERR_NO_ERR){
      goto out;
   }

   TN_CHECK_INT_CONTEXT;

   tn_idisable_interrupt();

   rc = _fmem_release(fmp, p_data);

   tn_ienable_interrupt();

out:
   return rc;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


