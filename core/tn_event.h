/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _TN_EVENT_H
#define _TN_EVENT_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_utils.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

typedef struct _TN_EVENT {
   struct TNQueueHead wait_queue;
   int attr;               //-- Eventflag attribute
   unsigned int pattern;   //-- Initial value of the eventflag bit pattern
   int id_event;           //-- ID for verification(is it a event or another object?)
   // All events have the same id_event magic number (ver 2.x)
} TN_EVENT;



/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define  TN_EVENT_ATTR_SINGLE            1
#define  TN_EVENT_ATTR_MULTI             2
#define  TN_EVENT_ATTR_CLR               4

#define  TN_EVENT_WCOND_OR               8
#define  TN_EVENT_WCOND_AND           0x10



/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

int tn_event_create(TN_EVENT * evf,
                      int attr,
                      unsigned int pattern);
int tn_event_delete(TN_EVENT * evf);
int tn_event_wait(TN_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern,
                    unsigned long timeout);
int tn_event_wait_polling(TN_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern);
int tn_event_iwait(TN_EVENT * evf,
                    unsigned int wait_pattern,
                    int wait_mode,
                    unsigned int * p_flags_pattern);
int tn_event_set(TN_EVENT * evf, unsigned int pattern);
int tn_event_iset(TN_EVENT * evf, unsigned int pattern);
int tn_event_clear(TN_EVENT * evf, unsigned int pattern);
int tn_event_iclear(TN_EVENT * evf, unsigned int pattern);


#endif // _TN_EVENT_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


