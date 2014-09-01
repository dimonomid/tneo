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
typedef struct tn_mutex       TN_MUTEX;
typedef struct tn_dqueue      TN_DQUE;
typedef struct tn_event       TN_EVENT;
typedef struct tn_task        TN_TCB;
typedef struct tn_fmp         TN_FMP;

#if 0
typedef struct _TN_SEM TN_SEM;
#endif




/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*
 * compatibility with old struct names
 */
#define  _CDLL_QUEUE    tn_que_head
#define  _TN_MUTEX      tn_mutex
#define  _TN_DQUE       tn_dqueue
#define  _TN_EVENT      tn_event
#define  _TN_TCB        tn_task
#define  _TN_FMP        tn_fmp

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#endif // _TN_OLDSYMBOLS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


