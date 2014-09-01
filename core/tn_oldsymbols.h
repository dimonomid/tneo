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

typedef struct TN_QueHead    CDLL_QUEUE;
typedef struct TN_Mutex       TN_MUTEX;
typedef struct TN_DQueue      TN_DQUE;
typedef struct TN_Event       TN_EVENT;
typedef struct TN_Task        TN_TCB;
typedef struct TN_Fmp         TN_FMP;
typedef struct TN_Sem         TN_SEM;





/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*
 * compatibility with old struct names
 */
#define  _CDLL_QUEUE    TN_QueHead
#define  _TN_MUTEX      TN_Mutex
#define  _TN_DQUE       TN_DQueue
#define  _TN_EVENT      TN_Event
#define  _TN_TCB        TN_Task
#define  _TN_FMP        TN_Fmp
#define  _TN_SEM        TN_Sem

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#endif // _TN_OLDSYMBOLS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


