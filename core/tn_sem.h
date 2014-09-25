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

#ifndef _TN_SEM_H
#define _TN_SEM_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_Sem {
   struct TN_ListItem  wait_queue;
   int count;
   int max_count;
   enum TN_ObjId id_sem;     //-- ID for verification(is it a semaphore or another object?)
                   // All semaphores have the same id_sem magic number (ver 2.x)
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

enum TN_RCode tn_sem_create(struct TN_Sem * sem, int start_count, int max_count);
enum TN_RCode tn_sem_delete(struct TN_Sem * sem);
enum TN_RCode tn_sem_signal(struct TN_Sem * sem);
enum TN_RCode tn_sem_isignal(struct TN_Sem * sem);
enum TN_RCode tn_sem_acquire(struct TN_Sem * sem, unsigned long timeout);
enum TN_RCode tn_sem_acquire_polling(struct TN_Sem * sem);
enum TN_RCode tn_sem_iacquire_polling(struct TN_Sem * sem);


#endif // _TN_SEM_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


