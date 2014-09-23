/**
 *   \file
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _TN_COMMON_H
#define _TN_COMMON_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- Configuration constants
#define  TN_API_MAKE_ALIG_ARG__TYPE       1     //-- this way is used in the majority of ports.
#define  TN_API_MAKE_ALIG_ARG__SIZE       2     //-- this way is stated in TNKernel docs
                                                //   and used in port by AlexB.


//--- As a starting point, you might want to copy tn_cfg_default.h -> tn_cfg.h,
//    and then edit it if you want to change default configuration.
//    NOTE: the file tn_cfg.h is specified in .hgignore file, in order to not include
//    project-specific configuration in the common TNKernel repository.
#include "tn_cfg.h"

//--- default cfg file is included too, so that you are free to not set
//    all available options in your tn_cfg.h file.
#include "tn_cfg_default.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Magic number for object validity verification
 */
enum TN_ObjId {
   TN_ID_TASK           = 0x47ABCF69,
   TN_ID_SEMAPHORE      = 0x6FA173EB,
   TN_ID_EVENT          = 0x5E224F25,
   TN_ID_DATAQUEUE      = 0x8C8A6C89,
   TN_ID_FSMEMORYPOOL   = 0x26B7CE8B,
   TN_ID_MUTEX          = 0x17129E45,
   TN_ID_RENDEZVOUS     = 0x74289EBD,
};

/**
 * Result code returned by kernel services
 */
enum TN_RCode {
   ///
   /// successful
   TN_RC_OK                   =   0,
   ///
   /// timeout or polling failure
   TN_RC_TIMEOUT              =  -1,
   ///
   /// trying to increment semaphore count more than its max count,
   /// or trying to return extra memory block to fixed memory pool.
   /// @see tn_sem.h
   /// @see tn_fmem.h
   TN_RC_OVERFLOW             =  -2,
   ///
   /// wrong context error: returned if user calls some task service from
   /// interrupt or vice versa
   TN_RC_WCONTEXT             =  -3,
   ///
   /// wrong task state error: requested operation requires different 
   /// task state
   TN_RC_WSTATE               =  -4,
   ///
   /// this code is returned by most of the kernel functions when 
   /// wrong params were given to function. This error code can be returned
   /// if only build-time option `TN_CHECK_PARAM` is non-zero
   /// @see `TN_CHECK_PARAM`
   TN_RC_WPARAM               =  -5,
   ///
   /// Illegal usage. Returned in the following cases:
   /// * task tries to unlock or delete the mutex that is locked by different
   ///   task,
   /// * task tries to lock mutex with priority ceiling whose priority is
   ///   lower than task's priority
   /// @see tn_mutex.h
   TN_RC_ILLEGAL_USE          =  -6,
   ///
   /// returned when user tries to perform some operation on invalid object
   /// (mutex, semaphore, etc).
   /// Object validity is checked by comparing special `id_...` value with the
   /// value from `enum TN_ObjId`
   TN_RC_INVALID_OBJ          =  -7,
   /// object for whose event task was waiting is deleted.
   TN_RC_DELETED              =  -8,
   /// task was released from waiting forcibly because some other task 
   /// called `tn_task_release_wait()`
   TN_RC_FORCED               =  -9,
   /// internal kernel error, should never be returned by kernel services.
   /// If it is returned, it's a bug in the kernel.
   TN_RC_INTERNAL             = -10,
};



/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


//--- wait reasons

#define  NO_TIME_SLICE                   0
#define  MAX_TIME_SLICE             0xFFFE



#ifndef NULL
#  define NULL       ((void *)0)
#endif

#ifndef BOOL
#  define BOOL       int
#endif

#ifndef TRUE
#  define TRUE       (1 == 1)
#endif

#ifndef FALSE
#  define FALSE      (1 == 0)
#endif


#if !defined(container_of)
/* given a pointer @ptr to the field @member embedded into type (usually
 * struct) @type, return pointer to the embedding instance of @type. */
#define container_of(ptr, type, member) \
   ((type *)((char *)(ptr)-(char *)(&((type *)0)->member)))
#endif


//-- TN_MAKE_ALIG() macro
#define  TN_MAKE_ALIG_SIZE(a)  (((a) + (TN_ALIG - 1)) & (~(TN_ALIG - 1)))

//-- self-checking
#if (!defined TN_API_MAKE_ALIG_ARG)
#  error TN_API_MAKE_ALIG_ARG is not defined
#elif (!defined TN_API_MAKE_ALIG_ARG__TYPE)
#  error TN_API_MAKE_ALIG_ARG__TYPE is not defined
#elif (!defined TN_API_MAKE_ALIG_ARG__SIZE)
#  error TN_API_MAKE_ALIG_ARG__SIZE is not defined
#endif

//-- define MAKE_ALIG accordingly to config
#if (TN_API_MAKE_ALIG_ARG == TN_API_MAKE_ALIG_ARG__TYPE)
#  define  TN_MAKE_ALIG(a)  TN_MAKE_ALIG_SIZE(sizeof(a))
#elif (TN_API_MAKE_ALIG_ARG == TN_API_MAKE_ALIG_ARG__SIZE)
#  define  TN_MAKE_ALIG(a)  TN_MAKE_ALIG_SIZE(a)
#else
#  error wrong TN_API_MAKE_ALIG_ARG
#endif



/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#endif // _TN_COMMON_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


