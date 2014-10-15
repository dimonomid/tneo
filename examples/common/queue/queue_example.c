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


/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * Each example should define this funtion: it creates first application task
 */
void init_task_create(void)
{
   task_consumer_create();
}

/*******************************************************************************
 *    end of file
 ******************************************************************************/



