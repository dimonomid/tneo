/*******************************************************************************
 *    Description: TODO
 *
 ******************************************************************************/


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include <xc.h>
#include <plib.h>
#include "task_consumer.h"
#include "task_producer.h"
#include "queue_example.h"
#include "tn.h"

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

//-- task stack size
#define  TASK_CONSUMER_STACK_SIZE      (TN_MIN_STACK_SIZE + 96)

//-- highest priority
#define  TASK_CONSUMER_PRIORITY        0 



/*******************************************************************************
 *    PRIVATE FUNCTION PROTOTYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

//-- define array for task stack
TN_STACK_ARR_DEF(task_consumer_stack, TASK_CONSUMER_STACK_SIZE);

//-- task descriptor: it's better to explicitly zero it
static struct TN_Task task_consumer = {};



/*******************************************************************************
 *    PUBLIC DATA
 ******************************************************************************/

/*******************************************************************************
 *    EXTERNAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    EXTERNAL FUNCTION PROTOTYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Application init: called from the first created application task
 */
static void appl_init(void)
{
   //-- configure LED port pins

   //-- set output for needed pins
   TRISECLR = (1 << TASK_CONS_PIN__0);

   //-- clear current port value
   PORTE    = 0;


   //-- create all the rest application tasks:
   //
   //   create the producer task
   task_producer_create();

   //TODO: use semaphore or event
   SYSRETVAL_CHECK_TO( tn_task_sleep(100) );
}

static void task_consumer_body(void *par)
{
   appl_init();

   for (;;)
   {
      SYSRETVAL_CHECK_TO( tn_task_sleep(100) );

      //-- toggle bits
      PORTEINV = (1 << 0);
   }
}






/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

void task_consumer_create(void)
{

   SYSRETVAL_CHECK(
         tn_task_create(
            &task_consumer,
            task_consumer_body,
            TASK_CONSUMER_PRIORITY,
            task_consumer_stack,
            TASK_CONSUMER_STACK_SIZE,
            NULL,
            (TN_TASK_CREATE_OPT_START)
            )
         );

}





/*******************************************************************************
 *    end of file
 ******************************************************************************/



