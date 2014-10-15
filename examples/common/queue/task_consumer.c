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

#include <xc.h>
#include "task_consumer.h"
#include "task_producer.h"
#include "queue_example.h"
#include "tn.h"

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

//-- task stack size
#define  TASK_CONSUMER_STACK_SIZE      (TN_MIN_STACK_SIZE + 96)

//-- priority of consumer task: the highest one
#define  TASK_CONSUMER_PRIORITY        0 

//-- number of items in the consumer message queue
#define  CONS_QUE_BUF_SIZE    4

//-- max timeout for waiting for memory and free message
#define  WAIT_TIMEOUT         10





//-- gpio register to work with, you might want to switch it to different port
//   NOTE: in real programs, it's better to use some peripheral library.
//   I strongly dislike plib as well as "brand new" Harmony by Microchip,
//   I use my own peripheral library, but in order to write simple example,
//   I decided to write it as simple as possible.
#define  TRIS_REG          TRISE
#define  TRIS_REG_CLR      TRISECLR
#define  TRIS_REG_SET      TRISESET
#define  TRIS_REG_INV      TRISEINV

#define  PORT_REG          PORTE
#define  PORT_REG_CLR      PORTECLR
#define  PORT_REG_SET      PORTESET
#define  PORT_REG_INV      PORTEINV



/*******************************************************************************
 *    PRIVATE FUNCTION PROTOTYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

struct TaskConsumerMsg {
   enum E_TaskConsCmd   cmd;
   enum E_TaskConsPin   pin_num;
};



/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

//-- define array for task stack
TN_STACK_ARR_DEF(task_consumer_stack, TASK_CONSUMER_STACK_SIZE);

//-- task descriptor: it's better to explicitly zero it
static struct TN_Task task_consumer = {};

//-- define queue and buffer for it
struct TN_DQueue     cons_que;
void                *cons_que_buf[ CONS_QUE_BUF_SIZE ];

//-- define fixed memory pool and buffer for it
struct TN_FMem       cons_fmem;
TN_FMEM_BUF_DEF(cons_fmem_buf, struct TaskConsumerMsg, CONS_QUE_BUF_SIZE);



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
   TRIS_REG_CLR = TASK_CONS_PIN_MASK;

   //-- clear current port value
   PORT_REG    = TASK_CONS_PIN_MASK;


   //-- create all the rest application tasks:
   //
   //   create the producer task
   task_producer_create();

   //TODO: use semaphore or event
   SYSRETVAL_CHECK_TO( tn_task_sleep(100) );
}

static void task_consumer_body(void *par)
{
   //-- in this particular application, consumer task is the first application
   //   task that is started, so, we should perform all the app initialization 
   //   here, and then start other tasks. All of this is done in the appl_init().
   appl_init();


   //-- init things specific to consumer task

   //-- create memory pool
   SYSRETVAL_CHECK(
         tn_fmem_create(
            &cons_fmem,
            cons_fmem_buf,
            TN_MAKE_ALIG_SIZE(sizeof(struct TaskConsumerMsg)),
            CONS_QUE_BUF_SIZE
            )
         );

   //-- create queue
   SYSRETVAL_CHECK(
         tn_queue_create(&cons_que, (void *)cons_que_buf, CONS_QUE_BUF_SIZE)
         );

   struct TaskConsumerMsg *p_msg;

   //-- enter endless loop in which we will receive and handle messages
   for (;;)
   {
      //-- wait for message, potentially can wait forever
      //   (if there are no messages)
      enum TN_RCode rc = SYSRETVAL_CHECK(
            tn_queue_receive(&cons_que, (void *)&p_msg, TN_WAIT_INFINITE)
            );

      if (rc == TN_RC_OK){

         //-- message successfully received, let's check command
         switch (p_msg->cmd){

            case TASK_CONS_CMD__PIN_TOGGLE:
               //-- toggle specified bit
               PORT_REG_INV = (1 << p_msg->pin_num);
               break;

            default:
               //-- should never be here
               SOFTWARE_BREAK();
               break;
         }

         //-- free memory
         SYSRETVAL_CHECK(tn_fmem_release(&cons_fmem, (void *)p_msg));
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
            NULL,
            (TN_TASK_CREATE_OPT_START)
            )
         );

}

/**
 * See comments in the header file
 */
int task_consumer_msg_send(enum E_TaskConsCmd cmd, enum E_TaskConsPin pin_num)
{
   int ret = 0;

   struct TaskConsumerMsg *p_msg;
   enum TN_RCode tn_rc;

   //-- get memory block from memory pool
   tn_rc = SYSRETVAL_CHECK_TO(
         tn_fmem_get(&cons_fmem, (void *)&p_msg, WAIT_TIMEOUT)
         );
   if (tn_rc == TN_RC_OK){

      //-- put correct data to the allocated memory
      p_msg->cmd     = cmd;
      p_msg->pin_num = pin_num;

      //-- send it to the consumer task
      tn_rc = SYSRETVAL_CHECK_TO(
            tn_queue_send(&cons_que, (void *)p_msg, WAIT_TIMEOUT)
            );

      if (tn_rc == TN_RC_OK){
         ret = 1/*success*/;
      } else {
         //-- there was some error while sending the message,
         //   so, we should free buffer that we've allocated
         SYSRETVAL_CHECK(tn_fmem_release(&cons_fmem, (void *)p_msg));
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



