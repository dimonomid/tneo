/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

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

struct TN_Fmp {
   struct TN_ListItem   wait_queue;       //-- task wait queue

   unsigned int         block_size;       //-- block size (in bytes)
   int                  blocks_cnt;       //-- capacity (total blocks count)
   int                  free_blocks_cnt;  //-- free blocks count
   void                *start_addr;       //-- memory pool start address
   void                *free_list;        //-- ptr to free block list
   enum TN_ObjId        id_fmp;           //-- id for verification
                                          //   (is it a fixed-sized blocks 
                                          //   memory pool or another object?)
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

enum TN_Retval tn_fmem_create(
      struct TN_Fmp    *fmp,
      void             *start_addr,
      unsigned int      block_size,
      int               blocks_cnt
      );
enum TN_Retval tn_fmem_delete(struct TN_Fmp *fmp);;
enum TN_Retval tn_fmem_get(struct TN_Fmp *fmp, void **p_data, unsigned long timeout);
enum TN_Retval tn_fmem_get_polling(struct TN_Fmp *fmp, void **p_data);
enum TN_Retval tn_fmem_get_ipolling(struct TN_Fmp *fmp, void **p_data);
enum TN_Retval tn_fmem_release(struct TN_Fmp *fmp, void *p_data);
enum TN_Retval tn_fmem_irelease(struct TN_Fmp *fmp, void *p_data);


#endif // _TN_MEM_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


