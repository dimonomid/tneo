/**
 * \file
 *
 * Data queue usage example.
 *
 * For general information about the pattern, refer to the top of
 * queue_example.h file
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "queue_example.h"

#include "task_consumer.h"
#include "task_producer.h"

#include "tn.h"



/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

static struct TN_EventGrp que_example_events;




/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * Each example should define this funtion: it creates first application task
 */
void init_task_create(void)
{
   task_producer_create();
}


/**
 * See comments in the header file
 */
void queue_example_init(void)
{
   //-- create application events 
   //   (see enum E_QueExampleFlag in the header)
   SYSRETVAL_CHECK(tn_eventgrp_create(&que_example_events, (0)));
}

/**
 * See comments in the header file
 */
struct TN_EventGrp *queue_example_eventgrp_get(void)
{
   return &que_example_events;
}

/*******************************************************************************
 *    end of file
 ******************************************************************************/



