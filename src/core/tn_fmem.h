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
 * Fixed memory blocks pool.
 *
 * A fixed-sized memory blocks pool is used for managing fixed-sized memory
 * blocks dynamically. A pool has a memory area where fixed-sized memory blocks
 * are allocated and the wait queue for acquiring a memory block.  If there are
 * no free memory blocks, a task trying to acquire a memory block will be
 * placed into the wait queue until a free memory block arrives (another task
 * returns it to the memory pool).
 */

#ifndef _TN_MEM_H
#define _TN_MEM_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Fixed memory blocks pool
 */
struct TN_FMem {
   ///
   /// list of tasks waiting for free memory block
   struct TN_ListItem   wait_queue;

   ///
   /// block size (in bytes); note that it should be a multiple of `TN_ALIGN`,
   /// use a macro `TN_MAKE_ALIG_SIZE()` for that.
   ///
   /// @see `TN_MAKE_ALIG_SIZE()`
   unsigned int         block_size;
   ///
   /// capacity (total blocks count)
   int                  blocks_cnt;
   ///
   /// free blocks count
   int                  free_blocks_cnt;
   ///
   /// memory pool start address; note that it should be a multiple of
   /// `TN_ALIGN`.
   void                *start_addr;
   ///
   /// ptr to free block list
   void                *free_list;
   ///
   /// id for object validity verification
   enum TN_ObjId        id_fmp;
};


/**
 * FMem-specific fields related to waiting task,
 * to be included in struct TN_Task.
 */
struct TN_FMemTaskWait {
   /// if task tries to receive data from memory pool,
   /// and there's no more free blocks in the pool, location to store 
   /// pointer is saved in this field
   void *data_elem;
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

/**
 * Construct fixed memory blocks pool. `id_fmp` field should not contain
 * `TN_ID_FSMEMORYPOOL`, otherwise, `TN_RC_WPARAM` is returned.
 *
 * Note that `start_addr` and `block_size` should be aligned by
 * `TN_MAKE_ALIG_SIZE()` macro. Typical buffer declaration is as follows:
 *
 *     #define MY_MEMORY_BUF_SIZE    8
 *
 *     struct MyMemoryItem {
 *        // ... arbitrary fields ...
 *     };
 *     
 *     int my_fmp_buf[
 *             MY_MEMORY_BUF_SIZE 
 *           * (TN_MAKE_ALIG_SIZE(sizeof(struct MyMemoryItem)) / sizeof(int))
 *           ];
 *     struct TN_Fmp my_fmp;
 *
 * And then, construct your `my_fmp` as follows:
 *
 *     tn_fmem_create( &my_fmp,
 *                     my_fmp_buf,
 *                     TN_MAKE_ALIG_SIZE(sizeof(struct MyMemoryItem)),
 *                     MY_MEMORY_BUF_SIZE );
 *
 * If given `start_addr` and/or `block_size` aren't aligned properly,
 * `TN_RC_WPARAM` is returned.
 *
 * @param fmem       pointer to already allocated struct TN_FMem.
 * @param start_addr pointer to start of the array; should be aligned properly,
 *                   see example above
 * @param block_size size of memory block; should be a multiple of `TN_ALIGN`,
 *                   see example above
 * @param blocks_cnt capacity (total number of blocks in the memory pool)
 *
 * @see TN_MAKE_ALIG_SIZE
 */
enum TN_RCode tn_fmem_create(
      struct TN_FMem   *fmem,
      void             *start_addr,
      unsigned int      block_size,
      int               blocks_cnt
      );

/**
 * Destruct fixed memory blocks pool.
 *
 * All tasks that wait for free memory block become runnable with
 * `TN_RC_DELETED` code returned.
 *
 * @param fmem       pointer to memory pool to be deleted
 */
enum TN_RCode tn_fmem_delete(struct TN_FMem *fmem);;

/**
 * Get memory block from the pool. Start address of the memory block is returned
 * through the `p_data` argument. The content of memory block is undefined.
 * If there is no free block in the pool, behavior depends on `timeout` value:
 *
 * * `0`:                  function returns `TN_RC_TIMEOUT` immediately;
 * * `TN_WAIT_INFINITE`:   task is switched to waiting state until
 *                         there is empty block;
 * * other:                task is switched to waiting state until
 *                         there is empty block or until
 *                         specified timeout expires.
 *
 * @param fmem
 *    Pointer to memory pool
 * @param p_data
 *    Address of the `(void *)` to which received block address will be saved
 * @param timeout
 *    Timeout to wait if there is no free memory block.
 *
 * @return
 *    * `TN_RC_OK` if block was successfully returned through `p_data`;
 *    * `TN_RC_TIMEOUT` if there is no free block and timeout expired;
 */
enum TN_RCode tn_fmem_get(
      struct TN_FMem *fmem,
      void **p_data,
      TN_Timeout timeout
      );

/**
 * The same as `tn_fmem_get()` with zero timeout
 */
enum TN_RCode tn_fmem_get_polling(struct TN_FMem *fmem, void **p_data);

/**
 * The same as `tn_fmem_get()` with zero timeout, but for using in the ISR.
 */
enum TN_RCode tn_fmem_iget_polling(struct TN_FMem *fmem, void **p_data);


/**
 * Release memory block back to the pool. The kernel does not check the 
 * validity of the membership of given block in the memory pool.
 * If all the memory blocks in the pool are free already, `TN_RC_OVERFLOW`
 * is returned.
 *
 * @param fmem
 *    Pointer to memory pool.
 * @param p_data
 *    Address of the memory block to release.
 */
enum TN_RCode tn_fmem_release(struct TN_FMem *fmem, void *p_data);

/**
 * The same as `tn_fmem_get()`, but for using in the ISR.
 */
enum TN_RCode tn_fmem_irelease(struct TN_FMem *fmem, void *p_data);


#endif // _TN_MEM_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


