/**
 * \file
 *
 * Example project that demonstrates usage of queues in TNeo.
 */

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "task_consumer.h"
#include "task_producer.h"
#include "example_queue_eventgrp_conn.h"
#include "tn.h"

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

//-- task stack size
#define  TASK_CONSUMER_STACK_SIZE      (TN_MIN_STACK_SIZE + 96)

//-- priority of consumer task: the highest one
#define  TASK_CONSUMER_PRIORITY        0 

//-- number of items in the consumer message queue A
#define  CONS_QUE_A_BUF_SIZE    4

//-- number of items in the consumer message queue B
#define  CONS_QUE_B_BUF_SIZE    2

//-- max timeout for waiting for memory and free message
#define  WAIT_TIMEOUT         10





/*******************************************************************************
 *    PRIVATE FUNCTION PROTOTYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

/**
 * Type of message that consumer receives.
 */
struct TaskConsumerMsgA {
   enum E_TaskConsCmd   cmd;        //-- command to perform
                                    //   (well, now there's just one command)
   enum E_TaskConsPin   pin_num;    //-- number of pin for which command
                                    //   should be performed
};

struct TaskConsumerMsgB {
   int      bool_on; //-- whether all pins should be turned on or off
};



/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

//-- define array for task stack
TN_STACK_ARR_DEF(task_consumer_stack, TASK_CONSUMER_STACK_SIZE);

//-- task descriptor: it's better to explicitly zero it
static struct TN_Task task_consumer = {};

//-- define queue A and buffer for it
struct TN_DQueue     cons_que_a;
void                *cons_que_a_buf[ CONS_QUE_A_BUF_SIZE ];

//-- define fixed memory pool for queue A and buffer for it
struct TN_FMem       cons_fmem_a;
TN_FMEM_BUF_DEF(cons_fmem_a_buf, struct TaskConsumerMsgA, CONS_QUE_A_BUF_SIZE);

//-- define queue B and buffer for it
struct TN_DQueue     cons_que_b;
void                *cons_que_b_buf[ CONS_QUE_B_BUF_SIZE ];

//-- define fixed memory pool for queue B and buffer for it
struct TN_FMem       cons_fmem_b;
TN_FMEM_BUF_DEF(cons_fmem_b_buf, struct TaskConsumerMsgB, CONS_QUE_B_BUF_SIZE);





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

static void msg_a_handle(struct TaskConsumerMsgA *p_msg)
{
   //-- message successfully received, let's check command
   switch (p_msg->cmd){

      case TASK_CONS_CMD__PIN_TOGGLE:
         //-- toggle specified bit
         queue_example_arch_pins_toggle(1 << p_msg->pin_num);
         break;

      default:
         //-- should never be here
         SOFTWARE_BREAK();
         break;
   }
}

static void msg_b_handle(struct TaskConsumerMsgB *p_msg)
{
   //-- message successfully received, let's check command
   if (p_msg->bool_on){
      queue_example_arch_pins_set(TASK_CONS_PIN_MASK);
   } else {
      queue_example_arch_pins_clear(TASK_CONS_PIN_MASK);
   }
}

static void task_consumer_body(void *par)
{
   //-- init things specific to consumer task

   //-- init queue A {{{
   //-- create memory pool for queue A
   SYSRETVAL_CHECK(
         tn_fmem_create(
            &cons_fmem_a,
            cons_fmem_a_buf,
            TN_MAKE_ALIG_SIZE(sizeof(struct TaskConsumerMsgA)),
            CONS_QUE_A_BUF_SIZE
            )
         );

   //-- create queue A
   SYSRETVAL_CHECK(
         tn_queue_create(&cons_que_a, (void *)cons_que_a_buf, CONS_QUE_A_BUF_SIZE)
         );
   // }}}

   //-- init queue B {{{
   //-- create memory pool for queue B
   SYSRETVAL_CHECK(
         tn_fmem_create(
            &cons_fmem_b,
            cons_fmem_b_buf,
            TN_MAKE_ALIG_SIZE(sizeof(struct TaskConsumerMsgB)),
            CONS_QUE_B_BUF_SIZE
            )
         );

   //-- create queue B
   SYSRETVAL_CHECK(
         tn_queue_create(&cons_que_b, (void *)cons_que_b_buf, CONS_QUE_B_BUF_SIZE)
         );
   // }}}


   //-- connect application's common event group to both queues with
   //   appropriate flags
   SYSRETVAL_CHECK(
         tn_queue_eventgrp_connect(
            &cons_que_a, 
            queue_example_eventgrp_get(), 
            QUE_EXAMPLE_FLAG__MSG_A
            )
         );

   SYSRETVAL_CHECK(
         tn_queue_eventgrp_connect(
            &cons_que_b, 
            queue_example_eventgrp_get(), 
            QUE_EXAMPLE_FLAG__MSG_B
            )
         );



   //-- cry that consumer task has initialized
   SYSRETVAL_CHECK(
         tn_eventgrp_modify(
            queue_example_eventgrp_get(),
            TN_EVENTGRP_OP_SET,
            QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT
            )
         );

   //-- enter endless loop in which we will receive and handle messages
   for (;;)
   {
      //-- flags pattern that is filled by tn_eventgrp_wait() when event happens
      TN_UWord flags_pattern = 0;

      //-- wait for messages (i.e. for connected flags), potentially can wait forever
      //   (if there are no messages)
      enum TN_RCode rc = SYSRETVAL_CHECK(
            tn_eventgrp_wait(
               queue_example_eventgrp_get(), 
               (QUE_EXAMPLE_FLAG__MSG_A | QUE_EXAMPLE_FLAG__MSG_B), 
               TN_EVENTGRP_WMODE_OR, 
               &flags_pattern, 
               TN_WAIT_INFINITE
               )
            );

      if (rc == TN_RC_OK){
         //-- At least, one of the queues contains new message.
         //   Let's check which flags are set and receive message
         //   from appropriate queue.
         //
         //   We specify zero timeout here, because message
         //   should be already available: there aren't other
         //   customers that could "steal" that message from this task.

         if (flags_pattern & QUE_EXAMPLE_FLAG__MSG_A){
            //-- message A is ready to receive, let's receive it.
            struct TaskConsumerMsgA *p_msg_a;
            rc = SYSRETVAL_CHECK(
                  tn_queue_receive(&cons_que_a, (void *)&p_msg_a, 0)
                  );
            if (rc == TN_RC_OK){
               //-- handle the message
               msg_a_handle(p_msg_a);

               //-- free memory
               SYSRETVAL_CHECK(tn_fmem_release(&cons_fmem_a, (void *)p_msg_a));
            } else {
               //-- failed to receive message: should never be here
               SOFTWARE_BREAK();
            }

         }

         if (flags_pattern & QUE_EXAMPLE_FLAG__MSG_B){
            //-- message B is ready to receive, let's receive it.
            struct TaskConsumerMsgB *p_msg_b;
            rc = SYSRETVAL_CHECK(
                  tn_queue_receive(&cons_que_b, (void *)&p_msg_b, 0)
                  );
            if (rc == TN_RC_OK){
               //-- handle the message
               msg_b_handle(p_msg_b);

               //-- free memory
               SYSRETVAL_CHECK(tn_fmem_release(&cons_fmem_b, (void *)p_msg_b));
            } else {
               //-- failed to receive message: should never be here
               SOFTWARE_BREAK();
            }

         }
      } else {
         //-- failed to wait for events: should never be here,
         //   since we wait infinitely
         SOFTWARE_BREAK();
      }

   }
}






/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * See comments in the header file
 */
void task_consumer_create(void)
{

   SYSRETVAL_CHECK(
         tn_task_create(
            &task_consumer,
            task_consumer_body,
            TASK_CONSUMER_PRIORITY,
            task_consumer_stack,
            TASK_CONSUMER_STACK_SIZE,
            TN_NULL,
            (TN_TASK_CREATE_OPT_START)
            )
         );

}

/**
 * See comments in the header file
 */
int task_consumer_msg_a_send(enum E_TaskConsCmd cmd, enum E_TaskConsPin pin_num)
{
   int ret = 0;

   struct TaskConsumerMsgA *p_msg;
   enum TN_RCode tn_rc;

   //-- get memory block from memory pool
   tn_rc = SYSRETVAL_CHECK_TO(
         tn_is_task_context()
         ? tn_fmem_get(&cons_fmem_a, (void *)&p_msg, WAIT_TIMEOUT)
         : tn_fmem_iget_polling(&cons_fmem_a, (void *)&p_msg)
         );
   if (tn_rc == TN_RC_OK){

      //-- put correct data to the allocated memory
      p_msg->cmd     = cmd;
      p_msg->pin_num = pin_num;

      //-- send it to the consumer task
      tn_rc = SYSRETVAL_CHECK_TO(
            tn_is_task_context()
            ? tn_queue_send(&cons_que_a, (void *)p_msg, WAIT_TIMEOUT)
            : tn_queue_isend_polling(&cons_que_a, (void *)p_msg)
            );

      if (tn_rc == TN_RC_OK){
         ret = 1/*success*/;
      } else {
         //-- there was some error while sending the message,
         //   so, we should free buffer that we've allocated
         SYSRETVAL_CHECK(
               tn_is_task_context()
               ? tn_fmem_release(&cons_fmem_a, (void *)p_msg)
               : tn_fmem_irelease(&cons_fmem_a, (void *)p_msg)
               );
      }
   } else {
      //-- there was some error while allocating memory,
      //   nothing to do here: ret is still 0, and it is
      //   going to be returned
   }

   return ret;
}

/**
 * See comments in the header file
 */
int task_consumer_msg_b_send(int bool_on)
{
   int ret = 0;

   struct TaskConsumerMsgB *p_msg;
   enum TN_RCode tn_rc;

   //-- get memory block from memory pool
   tn_rc = SYSRETVAL_CHECK_TO(
         tn_is_task_context()
         ? tn_fmem_get(&cons_fmem_b, (void *)&p_msg, WAIT_TIMEOUT)
         : tn_fmem_iget_polling(&cons_fmem_b, (void *)&p_msg)
         );
   if (tn_rc == TN_RC_OK){

      //-- put correct data to the allocated memory
      p_msg->bool_on = bool_on;

      //-- send it to the consumer task
      tn_rc = SYSRETVAL_CHECK_TO(
            tn_is_task_context()
            ? tn_queue_send(&cons_que_b, (void *)p_msg, WAIT_TIMEOUT)
            : tn_queue_isend_polling(&cons_que_b, (void *)p_msg)
            );

      if (tn_rc == TN_RC_OK){
         ret = 1/*success*/;
      } else {
         //-- there was some error while sending the message,
         //   so, we should free buffer that we've allocated
         SYSRETVAL_CHECK(
               tn_is_task_context()
               ? tn_fmem_release(&cons_fmem_b, (void *)p_msg)
               : tn_fmem_irelease(&cons_fmem_b, (void *)p_msg)
               );
      }
   } else {
      //-- there was some error while allocating memory,
      //   nothing to do here: ret is still 0, and it is
      //   going to be returned
   }

   return ret;
}





/*******************************************************************************
 *    end of file
 ******************************************************************************/



