/**
 * \file
 *
 * Example project that demonstrates usage of queues in TNeo.
 */


#include "task_producer.h"
#include "task_consumer.h"
#include "example_queue_eventgrp_conn.h"
#include "tn.h"



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

//-- task stack size
#define  TASK_PRODUCER_STACK_SIZE      (TN_MIN_STACK_SIZE + 96)

//-- highest priority
#define  TASK_PRODUCER_PRIORITY        0 

//-- timer period, in system ticks
#define  MY_TIMER_PERIOD               750


/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

//-- define array for task stack
TN_STACK_ARR_DEF(task_producer_stack, TASK_PRODUCER_STACK_SIZE);

//-- task descriptor: it's better to explicitly zero it
static struct TN_Task task_producer = {};

static struct TN_Timer my_timer = {};


/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Callback function called by kernel timer (that is, from ISR context)
 */
static void my_timer_callback(struct TN_Timer *timer, void *p_user_data)
{
   //-- restart timer again
   tn_timer_start(&my_timer, MY_TIMER_PERIOD);

   //-- whether we should turn all LEDs on or off
   static int bool_all_on = 0;
   bool_all_on = !bool_all_on;

   if (!task_consumer_msg_b_send(bool_all_on)){
      //-- failed to send the message
      //   in this partucilar application, it should never happen,
      //   so, halt the debugger
      SOFTWARE_BREAK();
   }
}

/**
 * Application init: called from the first created application task
 */
static void appl_init(void)
{
   //-- init common application objects
   queue_example_init();

   //-- create all the rest application tasks:

   //-- create the consumer task {{{
   {
      task_consumer_create();

      //-- wait until consumer task initialized
      SYSRETVAL_CHECK(
            tn_eventgrp_wait(
               queue_example_eventgrp_get(),
               QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT, 
               TN_EVENTGRP_WMODE_AND,
               TN_NULL,
               TN_WAIT_INFINITE
               )
            );
   }
   // }}}

   //-- create and start timer for sending message B {{{
   {
      tn_timer_create(&my_timer, my_timer_callback, TN_NULL);
      tn_timer_start(&my_timer, MY_TIMER_PERIOD);
   }
   //}}}

}

/**
 * Body function for producer task
 */
static void task_producer_body(void *par)
{
   //-- in this particular application, producer task is the first application
   //   task that is started, so, we should perform all the app initialization 
   //   here, and then start other tasks. All of this is done in the appl_init().
   appl_init();

   //-- cry that producer task has initialized
   SYSRETVAL_CHECK(
         tn_eventgrp_modify(
            queue_example_eventgrp_get(),
            TN_EVENTGRP_OP_SET,
            QUE_EXAMPLE_FLAG__TASK_PRODUCER_INIT
            )
         );

   //-- at this point, application is completely initialized, and we can
   //   get to business: enter endless loop and repeatedly send
   //   messages to the consumer
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
         if (!task_consumer_msg_a_send(
                  TASK_CONS_CMD__PIN_TOGGLE, 
                  pin_num
                  )
            )
         {
            //-- failed to send the message
            //   in this partucilar application, it should never happen,
            //   so, halt the debugger
            SOFTWARE_BREAK();
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
            TN_NULL,
            (TN_TASK_CREATE_OPT_START)
            )
         );

}


