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

   if (fmp->fblkcnt > 0){
      ptr = fmp->free_list;
      fmp->free_list = *(void **)fmp->free_list;   //-- ptr - to new free list
      fmp->fblkcnt--;
   }

   if (ptr != NULL){
      *p_data = ptr;
      rc = TERR_NO_ERR;
   } else {
      rc = TERR_TIMEOUT;
   }

   return rc;
}

static inline enum TN_Retval _fmem_release(struct TN_Fmp *fmp, void *p_data, int *p_need_switch_context)
{
   struct TN_Task *task;

   enum TN_Retval rc = TERR_NO_ERR;
   *p_need_switch_context = 0;

   if (!tn_is_list_empty(&(fmp->wait_queue))){
      task = tn_list_first_entry(&(fmp->wait_queue), typeof(*task), task_queue);

      task->data_elem = p_data;

      *p_need_switch_context = _tn_task_wait_complete(
               task, (TN_WCOMPL__REMOVE_WQUEUE | TN_WCOMPL__TO_RUNNABLE)
               );
;
   } else {
      if (fmp->fblkcnt < fmp->num_blocks){
         *(void **)p_data = fmp->free_list;   //-- insert block into free block list
         fmp->free_list = p_data;
         fmp->fblkcnt++;
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

/**
 * Structure's field fmp->id_id_fmp have to be set to 0
 */
enum TN_Retval tn_fmem_create(struct TN_Fmp *fmp,
                     void *start_addr,
                     unsigned int block_size,
                     int num_blocks)
{
   void ** p_tmp;
   unsigned char *p_block;
   unsigned long i, j;
   enum TN_Retval rc;

   rc = _check_param_fmem_create(fmp);
   if (rc != TERR_NO_ERR){
      goto out;
   }

   if (start_addr == NULL || num_blocks < 2 || block_size < sizeof(int)){
      fmp->fblkcnt      = 0;
      fmp->num_blocks   = 0;
      fmp->id_fmp       = 0;
      fmp->free_list    = NULL;

      rc = TERR_WRONG_PARAM;
      goto out;
   }

   tn_list_reset(&(fmp->wait_queue));

  //-- Prepare addr/block aligment

   i = ((unsigned long)start_addr + (TN_ALIG - 1)) & (~(TN_ALIG - 1));
   fmp->start_addr  = (void*)i;
   fmp->block_size = (block_size + (TN_ALIG - 1)) & (~(TN_ALIG - 1));

   i = (unsigned long)start_addr + block_size * num_blocks;
   j = (unsigned long)fmp->start_addr + fmp->block_size * num_blocks;

   fmp->num_blocks = num_blocks;

   while (j > i){ //-- Get actual num_blocks
      j -= fmp->block_size;
      fmp->num_blocks--;
   }

   if (fmp->num_blocks < 2){
      fmp->fblkcnt    = 0;
      fmp->num_blocks = 0;
      fmp->free_list  = NULL;

      rc = TERR_WRONG_PARAM;
      goto out;
   }

  //-- Set blocks ptrs for allocation -------

   p_tmp    = (void **)fmp->start_addr;
   p_block  = (unsigned char *)fmp->start_addr + fmp->block_size;
   for (i = 0; i < (fmp->num_blocks - 1); i++){
      *p_tmp  = (void *)p_block;  //-- contents of cell = addr of next block
      p_tmp   = (void **)p_block;
      p_block += fmp->block_size;
   }
   *p_tmp = NULL;          //-- Last memory block first cell contents -  NULL

   fmp->free_list = fmp->start_addr;
   fmp->fblkcnt   = fmp->num_blocks;

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
   _tn_wait_queue_notify_deleted(&(fmp->wait_queue), TN_INTSAVE_DATA_ARG_GIVE);

   fmp->id_fmp = 0;   //-- Fixed-size memory pool does not exist now

   tn_enable_interrupt();

out:
   return rc;
}

//----------------------------------------------------------------------------
enum TN_Retval tn_fmem_get(struct TN_Fmp *fmp, void **p_data, unsigned long timeout)
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

   if (rc == TERR_TIMEOUT && timeout > 0){
      _tn_task_curr_to_wait_action( &(fmp->wait_queue),
            TSK_WAIT_REASON_WFIXMEM,
            timeout );
      tn_enable_interrupt();
      tn_switch_context();

      //-- When returns to this point, in the 'data_elem' have to be valid value
      *p_data = tn_curr_run_task->data_elem; //-- Return to caller

      rc = tn_curr_run_task->task_wait_rc;
      goto out;
   }

   tn_enable_interrupt();

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

   TN_CHECK_INT_CONTEXT

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

   int need_switch_context = 0;

   TN_CHECK_NON_INT_CONTEXT;

   tn_disable_interrupt();

   rc = _fmem_release(fmp, p_data, &need_switch_context);

   tn_enable_interrupt();
   if (need_switch_context){
      tn_switch_context();
   }

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

   int need_switch_context = 0;

   TN_CHECK_INT_CONTEXT;

   tn_idisable_interrupt();

   rc = _fmem_release(fmp, p_data, &need_switch_context);

   tn_ienable_interrupt();
   //-- in interrupt, ignore need_switch_context

out:
   return rc;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


