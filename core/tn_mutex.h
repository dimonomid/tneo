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

#ifndef _TN_MUTEX_H
#define _TN_MUTEX_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"


/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct TN_Mutex {
   struct TN_ListItem wait_queue;        //-- List of tasks that wait a mutex
   struct TN_ListItem mutex_queue;       //-- To include in task's locked mutexes list (if any)
#if TN_MUTEX_DEADLOCK_DETECT
   struct TN_ListItem deadlock_list;     //-- List of other mutexes involved in
                                         //   deadlock  (if no deadlock,
                                         //   this list is empty)
#endif

   int attr;                     //-- Mutex creation attr - CEILING or INHERIT

   struct TN_Task *holder;       //-- Current mutex owner(task that locked mutex)
   int ceil_priority;            //-- When mutex created with CEILING attr
   int cnt;                      //-- Lock count
   enum TN_ObjId id_mutex;                 //-- ID for verification(is it a mutex or another object?)
                     // All mutexes have the same id_mutex magic number (ver 2.x)
};

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define  TN_MUTEX_ATTR_CEILING           1
#define  TN_MUTEX_ATTR_INHERIT           2


#define get_mutex_by_mutex_queque(que)              \
   (que ? container_of(que, struct TN_Mutex, mutex_queue) : 0)

#define get_mutex_by_wait_queque(que)               \
   (que ? container_of(que, struct TN_Mutex, wait_queue) : 0)

#define get_mutex_by_lock_mutex_queque(que) \
   (que ? container_of(que, struct TN_Mutex, mutex_queue) : 0)




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

enum TN_RCode tn_mutex_create(struct TN_Mutex * mutex,
                    int attribute,
                    int ceil_priority);
enum TN_RCode tn_mutex_delete(struct TN_Mutex * mutex);
enum TN_RCode tn_mutex_lock(struct TN_Mutex * mutex, unsigned long timeout);
enum TN_RCode tn_mutex_lock_polling(struct TN_Mutex * mutex);
enum TN_RCode tn_mutex_unlock(struct TN_Mutex * mutex);


#endif // _TN_MUTEX_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


