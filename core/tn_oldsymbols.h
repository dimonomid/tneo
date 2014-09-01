/*******************************************************************************
 *   Description:   This file contains compatibility layer for old projects
 *                  that use old tnkernel names.
 *
 *                  Usage of them in new projects is discouraged.
 *                  Typedefs that hide structures are bad practices.
 *                  So, instead of "TN_MUTEX" use "struct TNMutex".
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

typedef struct TNQueueHead    CDLL_QUEUE;
typedef struct TNMutex        TN_MUTEX;

#if 0
typedef struct _TN_EVENT TN_EVENT;
typedef struct _TN_TCB TN_TCB;
typedef struct _TN_FMP TN_FMP;
typedef struct _TN_DQUE TN_DQUE;
typedef struct _TN_SEM TN_SEM;
#endif




/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define  _CDLL_QUEUE    TNQueueHead
#define  _TN_MUTEX      TNMutex

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#endif // _TN_OLDSYMBOLS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


