//
// main.c
//

#include <stdio.h>
#include "stm32f4xx.h"

#include "tn.h"


//-- system frequency
#define SYS_FREQ           168000000L

//-- kernel ticks (system timer) frequency
#define SYS_TMR_FREQ       1000

//-- system timer period (auto-calculated)
#define SYS_TMR_PERIOD              \
   (SYS_FREQ / SYS_TMR_FREQ)



//-- idle task stack size, in words
#define IDLE_TASK_STACK_SIZE          (TN_MIN_STACK_SIZE + 32)

//-- interrupt stack size, in words
#define INTERRUPT_STACK_SIZE          (TN_MIN_STACK_SIZE + 64)

//-- stack sizes of user tasks
#define TASK_A_STK_SIZE    (TN_MIN_STACK_SIZE + 96)
#define TASK_B_STK_SIZE    (TN_MIN_STACK_SIZE + 96)
#define TASK_C_STK_SIZE    (TN_MIN_STACK_SIZE + 96)

//-- user task priorities
#define TASK_A_PRIORITY    7
#define TASK_B_PRIORITY    6
#define TASK_C_PRIORITY    5




#define LED0 (1 << 12)
#define LED1 (1 << 13)
#define LED2 (1 << 14)
#define LED3 (1 << 15)


/*******************************************************************************
 *    DATA
 ******************************************************************************/

//-- Allocate arrays for stacks: stack for idle task
//   and for interrupts are the requirement of the kernel;
//   others are application-dependent.
//
//   We use convenience macro TN_STACK_ARR_DEF() for that.

TN_STACK_ARR_DEF(idle_task_stack, IDLE_TASK_STACK_SIZE);
TN_STACK_ARR_DEF(interrupt_stack, INTERRUPT_STACK_SIZE);

TN_STACK_ARR_DEF(task_a_stack, TASK_A_STK_SIZE);
TN_STACK_ARR_DEF(task_b_stack, TASK_B_STK_SIZE);
TN_STACK_ARR_DEF(task_c_stack, TASK_C_STK_SIZE);



//-- task structures

struct TN_Task task_a;
struct TN_Task task_b;
struct TN_Task task_c;



/*******************************************************************************
 *    ISRs
 ******************************************************************************/

/**
 * system timer ISR
 */
void SysTick_Handler(void)
{
   tn_tick_int_processing();
}



/*******************************************************************************
 *    FUNCTIONS
 ******************************************************************************/

/*
 * Needed for STM debug printf
 */
int fputc(int c, FILE *stream)
{
   return ITM_SendChar(c);
}



void appl_init(void);

void task_a_body(void *par)
{
   //-- this is a first created application task, so it needs to perform
   //   all the application initialization.
   appl_init();

   //-- and then, let's get to the primary job of the task
   //   (job for which task was created at all)
   for(;;)
   {
      if (GPIOD->ODR & LED1){
         GPIOD->BSRRH = LED1;
      } else {
         GPIOD->BSRRL = LED1;
      }

      //printf("task a\n");
      tn_task_sleep(500);

   }
}

void task_b_body(void *par)
{
   for(;;)
   {
      if (GPIOD->ODR & LED2){
         GPIOD->BSRRH = LED2;
      } else {
         GPIOD->BSRRL = LED2;
      }

      //printf("task b\n");
      tn_task_sleep(1000);
   }
}

void task_c_body(void *par)
{
   for(;;)
   {
      if (GPIOD->ODR & LED3){
         GPIOD->BSRRH = LED3;
      } else {
         GPIOD->BSRRL = LED3;
      }

      //printf("task c\n");
      tn_task_sleep(1500);
   }
}

/**
 * Hardware init: called from main() with interrupts disabled
 */
void hw_init(void)
{
   //-- init system timer
   SysTick_Config(SYS_TMR_PERIOD);
}

/**
 * Application init: called from the first created application task
 */
void appl_init(void)
{
   //-- configure LED port pins
   {
      RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; // Enable Port D clock

      //-- Set pin 12, 13, 14, 15 as general purpose output mode (pull-push)
      GPIOD->MODER |= (0
            | GPIO_MODER_MODER12_0
            | GPIO_MODER_MODER13_0
            | GPIO_MODER_MODER14_0
            | GPIO_MODER_MODER15_0
            ) ;

      // GPIOD->OTYPER |= 0; //-- No need to change - use pull-push output

      GPIOD->OSPEEDR |= (0
            | GPIO_OSPEEDER_OSPEEDR12 // 100MHz operations
            | GPIO_OSPEEDER_OSPEEDR13
            | GPIO_OSPEEDER_OSPEEDR14
            | GPIO_OSPEEDER_OSPEEDR15 
            );

      GPIOD->PUPDR = 0; // No pull up, no pull down
   }

   //-- initialize various on-board peripherals, such as
   //   flash memory, displays, etc.
   //   (in this sample project there's nothing to init)

   //-- initialize various program modules
   //   (in this sample project there's nothing to init)


   //-- create all the rest application tasks
   tn_task_create(
         &task_b,
         task_b_body,
         TASK_B_PRIORITY,
         task_b_stack,
         TASK_B_STK_SIZE,
         NULL,
         (TN_TASK_CREATE_OPT_START)
         );

   tn_task_create(
         &task_c,
         task_c_body,
         TASK_C_PRIORITY,
         task_c_stack,
         TASK_C_STK_SIZE,
         NULL,
         (TN_TASK_CREATE_OPT_START)
         );
}

//-- idle callback that is called periodically from idle task
void idle_task_callback (void)
{
}

//-- create first application task(s)
void init_task_create(void)
{
   //-- task A performs complete application initialization,
   //   it's the first created application task
   tn_task_create(
         &task_a,                   //-- task structure
         task_a_body,               //-- task body function
         TASK_A_PRIORITY,           //-- task priority
         task_a_stack,              //-- task stack
         TASK_A_STK_SIZE,           //-- task stack size (in words)
         NULL,                      //-- task function parameter
         TN_TASK_CREATE_OPT_START   //-- creation option
         );

}


int main(void)
{

   //-- unconditionally disable interrupts
   tn_arch_int_dis();

   //-- init hardware
   hw_init();

   //-- call to tn_sys_start() never returns
   tn_sys_start(
         idle_task_stack,
         IDLE_TASK_STACK_SIZE,
         interrupt_stack,
         INTERRUPT_STACK_SIZE,
         init_task_create,
         idle_task_callback
         );

   //-- unreachable
   return 1;

}
