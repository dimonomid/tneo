/***************************************************************************************************
 *   Description:   TODO
 *
 **************************************************************************************************/

#ifndef _TN_MEM_H
#define _TN_MEM_H

/***************************************************************************************************
 *                                         INCLUDED FILES                                          *
 **************************************************************************************************/

/***************************************************************************************************
 *                                          PUBLIC TYPES                                           *
 **************************************************************************************************/

typedef struct _TN_FMP
{
   CDLL_QUEUE wait_queue;

   unsigned int block_size; //-- Actual block size (in bytes)
   int num_blocks;          //-- Capacity (Fixed-sized blocks actual max qty)
   void *start_addr;        //-- Memory pool actual start address
   void *free_list;         //-- Ptr to free block list
   int fblkcnt;             //-- Num of free blocks
   int id_fmp;              //-- ID for verification(is it a fixed-sized blocks memory pool or another object?)
                            // All Fixed-sized blocks memory pool have the same id_fmp magic number (ver 2.x)
} TN_FMP;

/***************************************************************************************************
 *                                         GLOBAL VARIABLES                                        *
 **************************************************************************************************/

/***************************************************************************************************
 *                                           DEFINITIONS                                           *
 **************************************************************************************************/

/***************************************************************************************************
 *                                    PUBLIC FUNCTION PROTOTYPES                                   *
 **************************************************************************************************/

int tn_fmem_create(TN_FMP  * fmp,
      void * start_addr,
      unsigned int block_size,
      int num_blocks);
int tn_fmem_delete(TN_FMP * fmp);
int tn_fmem_get(TN_FMP * fmp, void ** p_data, unsigned long timeout);
int tn_fmem_get_polling(TN_FMP * fmp, void ** p_data);
int tn_fmem_get_ipolling(TN_FMP * fmp, void ** p_data);
int tn_fmem_release(TN_FMP * fmp, void * p_data);
int tn_fmem_irelease(TN_FMP * fmp, void * p_data);


#endif // _TN_MEM_H
/***************************************************************************************************
  end of file
 **************************************************************************************************/


