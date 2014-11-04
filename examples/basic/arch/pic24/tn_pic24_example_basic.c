/**
 * TNeoKernel PIC32 basic example
 */

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include <xc.h>
#include "tn.h"




/*******************************************************************************
 *    HARDWARE CONFIGURATION
 ******************************************************************************/

#pragma config JTAGEN         = OFF       //-- JTAG disabled
#pragma config GCP            = OFF       //-- Code protect off
#pragma config GWRP           = OFF       //-- Code write enabled
#pragma config BKBUG          = ON        //-- Debug enabled
#pragma config FWDTEN         = OFF       //-- Watchdog disabled

#pragma config WPDIS          = WPDIS     //-- Segmented code protection disabled
#pragma config WPCFG          = WPCFGDIS  //-- Last page and config words protection
                                          //   disabled
#pragma config POSCMOD        = HS        //-- HS oscillator
#pragma config FNOSC          = PRIPLL    //-- Primary oscillator (XT, HS, EC) with
                                          //   PLL module
#pragma config IESO           = OFF       //-- Two-speed start-up disabled
#pragma config OSCIOFNC       = ON        //-- OSCO functions as OSCO (FOSC / 2),
                                          //   not as GPIO
#pragma config PLLDIV         = DIV4      //-- Osc input divided by 4 (16 MHz)
#pragma config COE            = ON        //-- Clip on emulation mode enabled
#pragma config ICS            = PGx2      //-- Emulator functions are shared with 
                                          //   PGEC2/PGED2




/*******************************************************************************
 *    MACROS
 ******************************************************************************/

//-- instruction that causes debugger to halt
#define PIC24_SOFTWARE_BREAK()  {__asm__ volatile(".pword 0xDA4000"); __asm__ volatile ("nop");}

//-- system frequency
#define SYS_FREQ           32000000UL

//-- peripheral bus frequency
#define PB_FREQ            (SYS_FREQ / 2)

//-- kernel ticks (system timer) frequency
#define SYS_TMR_FREQ       1000

//-- system timer prescaler
//#define SYS_TMR_PRESCALER           T5_PS_1_8
#define SYS_TMR_PRESCALER_VALUE     64
#define SYS_TMR_PRESCALER_REGVALUE  2

//-- system timer period (auto-calculated)
#define SYS_TMR_PERIOD              \
   (PB_FREQ / SYS_TMR_PRESCALER_VALUE / SYS_TMR_FREQ)




//-- idle task stack size, in words
#define IDLE_TASK_STACK_SIZE          (TN_MIN_STACK_SIZE + 16)

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

struct TN_Task task_a = {};
struct TN_Task task_b = {};
struct TN_Task task_c = {};



/*******************************************************************************
 *    ISRs
 ******************************************************************************/

/**
 * system timer ISR
 */

tn_p24_soft_isr(_T1Interrupt, auto_psv)
{
   //-- clear interrupt flag
   TN_BFA(TN_BFA_WR, IFS0, T1IF, 0);

   //-- proceed system tick
   tn_tick_int_processing();
}



/*******************************************************************************
 *    FUNCTIONS
 ******************************************************************************/

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
      //-- atomically invert LATEbits.LATE0
      TN_BFA(TN_BFA_INV, LATE, LATE0, 1);
      tn_task_sleep(500);
   }
}

void task_b_body(void *par)
{
   for(;;)
   {
      //-- atomically invert LATEbits.LATE1
      TN_BFA(TN_BFA_INV, LATE, LATE1, 1);
      tn_task_sleep(1000);
   }
}

void task_c_body(void *par)
{
   for(;;)
   {
      //-- atomically invert LATEbits.LATE2
      TN_BFA(TN_BFA_INV, LATE, LATE2, 1);
      tn_task_sleep(1500);
   }
}

/**
 * Hardware init: called from main() with interrupts disabled
 */
void hw_init(void)
{
   //-- set up timer1
   TN_BFA(TN_BFA_WR, T1CON, TCS, 0);
   TN_BFA(TN_BFA_WR, T1CON, TGATE, 0);
   TN_BFA(TN_BFA_WR, T1CON, TSIDL, 1);

   //-- set prescaler: 1:64
   TN_BFA(TN_BFA_WR, T1CON, TCKPS, SYS_TMR_PRESCALER_REGVALUE); 
   //-- set period
   PR1 = (SYS_TMR_PERIOD - 1);

   //-- set timer1 interrupt
   TN_BFA(TN_BFA_WR, IPC0, T1IP, 2);   //-- set timer1 interrupt priority: 2
   TN_BFA(TN_BFA_WR, IFS0, T1IF, 0);   //-- clear interrupt flag
   TN_BFA(TN_BFA_WR, IEC0, T1IE, 1);   //-- enable interrupt

   //-- eventually, turn the timer on
   TN_BFA(TN_BFA_WR, T1CON, TON, 1);   
}

/**
 * Application init: called from the first created application task
 */
void appl_init(void)
{
   //-- configure LED port pins to output

   TN_BFA(TN_BFA_WR, TRISE, TRISE0, 0);   
   TN_BFA(TN_BFA_WR, TRISE, TRISE1, 0);   
   TN_BFA(TN_BFA_WR, TRISE, TRISE2, 0);   

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
         TN_NULL,
         (TN_TASK_CREATE_OPT_START)
         );

   tn_task_create(
         &task_c,
         task_c_body,
         TASK_C_PRIORITY,
         task_c_stack,
         TASK_C_STK_SIZE,
         TN_NULL,
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
         TN_NULL,                   //-- task function parameter
         TN_TASK_CREATE_OPT_START   //-- creation option
         );

}

int main(void)
{
   //-- unconditionally disable system interrupts
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




/*******************************************************************************
 *    ERROR TRAPS
 ******************************************************************************/

void __attribute((__interrupt__, auto_psv)) _AddressError (void)
{
   PIC24_SOFTWARE_BREAK();
   //for(;;);
}

void __attribute((__interrupt__, auto_psv)) _StackError (void)
{
   PIC24_SOFTWARE_BREAK();
   //for(;;);
}

void __attribute((__interrupt__, auto_psv)) _MathError (void)
{
   PIC24_SOFTWARE_BREAK();
   //for(;;);
}

void __attribute((__interrupt__, auto_psv)) _OscillatorFail (void)
{
   PIC24_SOFTWARE_BREAK();
   //for(;;);
}

void __attribute((__interrupt__, auto_psv)) _DefaultInterrupt (void)
{
   PIC24_SOFTWARE_BREAK();
   //for(;;);
}


