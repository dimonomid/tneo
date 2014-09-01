/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _TN_COMMON_H
#define _TN_COMMON_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- Configuration constants
#define  TN_API_TASK_CREATE__NATIVE       1     
#define  TN_API_TASK_CREATE__CONVENIENT   2

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

enum TN_ObjId {
   TN_ID_TASK           = 0x47ABCF69,
   TN_ID_SEMAPHORE      = 0x6FA173EB,
   TN_ID_EVENT          = 0x5E224F25,
   TN_ID_DATAQUEUE      = 0x8C8A6C89,
   TN_ID_FSMEMORYPOOL   = 0x26B7CE8B,
   TN_ID_MUTEX          = 0x17129E45,
   TN_ID_RENDEZVOUS     = 0x74289EBD,
};

enum TN_Retval {
   TERR_NO_ERR               =   0,
   TERR_OVERFLOW             =  -1, //-- OOV
   TERR_WCONTEXT             =  -2, //-- Wrong context context error
   TERR_WSTATE               =  -3, //-- Wrong state   state error
   TERR_TIMEOUT              =  -4, //-- Polling failure or timeout
   TERR_WRONG_PARAM          =  -5,
   TERR_UNDERFLOW            =  -6,
   TERR_OUT_OF_MEM           =  -7,
   TERR_ILUSE                =  -8, //-- Illegal using
   TERR_NOEXS                =  -9, //-- Non-valid or Non-existent object
   TERR_DLT                  = -10, //-- Waiting object deleted
   TERR_INTERNAL             = -12, //-- Internal TNKernel error (should never happen)
};


/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


//--- wait reasons
#define  TSK_WAIT_REASON_NONE             0
#define  TSK_WAIT_REASON_SLEEP            0x0001
#define  TSK_WAIT_REASON_SEM              0x0002
#define  TSK_WAIT_REASON_EVENT            0x0004
#define  TSK_WAIT_REASON_DQUE_WSEND       0x0008
#define  TSK_WAIT_REASON_DQUE_WRECEIVE    0x0010
#define  TSK_WAIT_REASON_MUTEX_C          0x0020          //-- ver 2.x
#define  TSK_WAIT_REASON_MUTEX_C_BLK      0x0040          //-- ver 2.x
#define  TSK_WAIT_REASON_MUTEX_I          0x0080          //-- ver 2.x
#define  TSK_WAIT_REASON_MUTEX_H          0x0100          //-- ver 2.x
#define  TSK_WAIT_REASON_RENDEZVOUS       0x0200          //-- ver 2.x
#define  TSK_WAIT_REASON_WFIXMEM          0x2000



#define  NO_TIME_SLICE                   0
#define  MAX_TIME_SLICE             0xFFFE



#ifndef NULL
#define NULL      0
#endif


//-- Thanks to Vyacheslav Ovsiyenko - for his highly optimized code

#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field)     \
   ((type *)((unsigned char *)(address) - (unsigned char *)(&((type *)0)->field)))
#endif


/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#endif // _TN_COMMON_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


