/**
 * \file
 *
 * Data queue usage example.
 *
 * For general information about the pattern, refer to the top of
 * queue_example.h file
 */


#include <xc.h>
#include "task_producer.h"
#include "task_consumer.h"
#include "queue_example.h"
#include "tn.h"



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

//-- task stack size
#define  TASK_PRODUCER_STACK_SIZE      (TN_MIN_STACK_SIZE + 96)

//-- highest priority
#define  TASK_PRODUCER_PRIORITY        0 



/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

//-- define array for task stack
TN_STACK_ARR_DEF(task_producer_stack, TASK_PRODUCER_STACK_SIZE);

//-- task descriptor: it's better to explicitly zero it
static struct TN_Task task_producer = {};



/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Body function for producer task
 */
static void task_producer_body(void *par)
{
   //-- nothing special to initialize here

   //-- cry that producer task has initialized
   tn_eventgrp_modify(
         queue_example_eventgrp_get(),
         TN_EVENTGRP_OP_SET,
         QUE_EXAMPLE_FLAG__TASK_PRODUCER_INIT
         );

   //-- wait until consumer task initialized, since we are about to
   //   send messages to it
   tn_eventgrp_wait(
         queue_example_eventgrp_get(),
         QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT, 
         TN_EVENTGRP_WMODE_AND,
         NULL,
         TN_WAIT_INFINITE
         );

   for (;;)
   {
      int i;
      
      for (i = 0; i < 3/*pins count*/;i++){
         //-- Wait before sending message
         SYSRETVAL_CHECK_TO( tn_task_sleep(100) );

         enum E_TaskConsPin pin_num = 0;

         //-- determine pin_num
         switch (i){
            case 0:
               pin_num = TASK_CONS_PIN__0;
               break;
            case 1:
               pin_num = TASK_CONS_PIN__1;
               break;
            case 2:
               pin_num = TASK_CONS_PIN__2;
               break;
            default:
               //-- should never be here
               SOFTWARE_BREAK();
               break;
         }

         //-- Send the message to consumer
         if (!task_consumer_msg_send(
                  TASK_CONS_CMD__PIN_TOGGLE, 
                  pin_num
                  )
            )
         {
            //-- failed to send the message
         }
      }

   }
}



/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * See comments in the header file
 */
void task_producer_create(void)
{

   SYSRETVAL_CHECK(
         tn_task_create(
            &task_producer,
            task_producer_body,
            TASK_PRODUCER_PRIORITY,
            task_producer_stack,
            TASK_PRODUCER_STACK_SIZE,
            NULL,
            (TN_TASK_CREATE_OPT_START)
            )
         );

}


