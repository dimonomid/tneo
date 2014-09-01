/*******************************************************************************
 *   Description:   This file contains compatibility layer for old projects
 *                  that use old tnkernel names.
 *
 *                  Usage of them in new projects is discouraged.
 *                  Typedefs that hide structures are bad practices.
 *                  So, instead of "TN_MUTEX" use "struct tn_mutex".
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

typedef struct tn_que_head    CDLL_QUEUE;
typedef struct tn_mutex        TN_MUTEX;
typedef struct tn_dqueue       TN_DQUE;

#if 0
typedef struct _TN_EVENT TN_EVENT;
typedef struct _TN_TCB TN_TCB;
typedef struct _TN_FMP TN_FMP;
typedef struct _TN_SEM TN_SEM;
#endif




/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define  _CDLL_QUEUE    tn_que_head
#define  _TN_MUTEX      tn_mutex
#define  _TN_DQUE       tn_dqueue

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#endif // _TN_OLDSYMBOLS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


