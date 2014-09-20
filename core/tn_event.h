/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _TN_EVENT_H
#define _TN_EVENT_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_list.h"
#include "tn_common.h"



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

enum TN_EventAttr {
   TN_EVENT_ATTR_SINGLE    = (1 << 0),    //-- only one task may wait for event
   TN_EVENT_ATTR_MULTI     = (1 << 1),    //-- many tasks may wait for event
   TN_EVENT_ATTR_CLR       = (1 << 2),    //-- when task finishes waiting for
                                          //   event, the event is automatically
                                          //   cleared (may be used only with 
                                          //   TN_EVENT_ATTR_SINGLE flag)
};

enum TN_EventWCond {
   TN_EVENT_WCOND_OR       = (1 << 3),    //-- any set bit is enough for event
   TN_EVENT_WCOND_AND      = (1 << 4),    //-- all bits should be send for event
};

struct TN_Event {
   struct TN_ListItem   wait_queue;
   enum TN_EventAttr    attr;       //-- see enum TN_EventAttr
   unsigned int         pattern;    //-- current flags pattern
   enum TN_ObjId        id_event;   //-- id for verification
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

enum TN_Retval tn_event_create(
      struct TN_Event * evf,
      enum TN_EventAttr attr,
      unsigned int pattern
      );

enum TN_Retval tn_event_delete(struct TN_Event * evf);
enum TN_Retval tn_event_wait(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern,
      unsigned long        timeout
      );
enum TN_Retval tn_event_wait_polling(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern
      );
enum TN_Retval tn_event_iwait(
      struct TN_Event     *evf,
      unsigned int         wait_pattern,
      enum TN_EventWCond   wait_mode,
      unsigned int        *p_flags_pattern
      );
enum TN_Retval tn_event_set(struct TN_Event *evf, unsigned int pattern);
enum TN_Retval tn_event_iset(struct TN_Event *evf, unsigned int pattern);
enum TN_Retval tn_event_clear(struct TN_Event *evf, unsigned int pattern);
enum TN_Retval tn_event_iclear(struct TN_Event *evf, unsigned int pattern);


#endif // _TN_EVENT_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


