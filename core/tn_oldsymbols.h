/*******************************************************************************
 *   Description:   This file contains compatibility layer for old projects
 *                  that use old tnkernel names.
 *
 *                  Usage of them in new projects is discouraged.
 *                  Typedefs that hide structures are bad practices.
 *                  So, instead of "TN_MUTEX" use "struct TN_Mutex", etc.
 *
 ******************************************************************************/

#ifndef _TN_OLDSYMBOLS_H
#define _TN_OLDSYMBOLS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

typedef struct TN_ListItem    CDLL_QUEUE;
typedef struct TN_Mutex       TN_MUTEX;
typedef struct TN_DQueue      TN_DQUE;
typedef struct TN_Task        TN_TCB;
typedef struct TN_Fmp         TN_FMP;
typedef struct TN_Sem         TN_SEM;

//there's no compatibility with TNKernel's event object,
//so, this one is commented
//typedef struct TN_EventGrp    TN_EVENT;




/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*
 * compatibility with old struct names
 */
#define  _CDLL_QUEUE    TN_ListItem
#define  _TN_MUTEX      TN_Mutex
#define  _TN_DQUE       TN_DQueue
#define  _TN_TCB        TN_Task
#define  _TN_FMP        TN_Fmp
#define  _TN_SEM        TN_Sem

//there's no compatibility with TNKernel's event object,
//so, this one is commented
//#define  _TN_EVENT      TN_EventGrp

#define  MAKE_ALIG      TN_MAKE_ALIG


#define  TSK_STATE_RUNNABLE            TN_TASK_STATE_RUNNABLE
#define  TSK_STATE_WAIT                TN_TASK_STATE_WAIT
#define  TSK_STATE_SUSPEND             TN_TASK_STATE_SUSPEND
#define  TSK_STATE_WAITSUSP            TN_TASK_STATE_WAITSUSP
#define  TSK_STATE_DORMANT             TN_TASK_STATE_DORMANT

#define  TN_TASK_START_ON_CREATION     TN_TASK_CREATE_OPT_START
#define  TN_EXIT_AND_DELETE_TASK       TN_TASK_EXIT_OPT_DELETE
/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#endif // _TN_OLDSYMBOLS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


