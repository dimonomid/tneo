/**
 * TNeoKernel PIC32 basic example
 */

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include <xc.h>
#include <plib.h>
#include <stdint.h>
#include "tn.h"




/*******************************************************************************
 *    PIC32 HARDWARE CONFIGURATION
 ******************************************************************************/

#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config FPLLIDIV = DIV_4         // PLL Input Divider (PIC32 Starter Kit: use divide by 2 only)
#pragma config FPLLMUL  = MUL_20        // PLL Multiplier
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_2         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select
#pragma config DEBUG    = OFF           // Debugger Disabled for Starter Kit




/*******************************************************************************
 *    MACROS
 ******************************************************************************/

//-- instruction that causes debugger to halt
#define PIC32_SOFTWARE_BREAK()  __asm__ volatile ("sdbbp 0")

//-- system frequency
#define SYS_FREQ           80000000UL

//-- peripheral bus frequency
#define PB_FREQ            40000000UL

//-- kernel ticks (system timer) frequency
#define SYS_TMR_FREQ       1000

//-- system timer prescaler
#define SYS_TMR_PRESCALER           T5_PS_1_8
#define SYS_TMR_PRESCALER_VALUE     8

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

//-- allocate arrays for stacks.
//
//   notice special architecture-dependent macros we use here,
//   they are needed to make sure that all requirements
//   regarding to stack are met.

TN_ARCH_STK_ATTR_BEFORE
unsigned int idle_task_stack[ IDLE_TASK_STACK_SIZE ]
TN_ARCH_STK_ATTR_AFTER;

TN_ARCH_STK_ATTR_BEFORE
unsigned int interrupt_stack[ INTERRUPT_STACK_SIZE ]
TN_ARCH_STK_ATTR_AFTER;

TN_ARCH_STK_ATTR_BEFORE
unsigned int task_a_stack[ TASK_A_STK_SIZE ]
TN_ARCH_STK_ATTR_AFTER;

TN_ARCH_STK_ATTR_BEFORE
unsigned int task_b_stack[ TASK_B_STK_SIZE ]
TN_ARCH_STK_ATTR_AFTER;

TN_ARCH_STK_ATTR_BEFORE
unsigned int task_c_stack[ TASK_C_STK_SIZE ]
TN_ARCH_STK_ATTR_AFTER;



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
tn_soft_isr(_TIMER_5_VECTOR)
{
   INTClearFlag(INT_T5);
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
      mPORTEToggleBits(BIT_0);
      tn_task_sleep(500);
   }
}

void task_b_body(void *par)
{
   for(;;)
   {
      mPORTEToggleBits(BIT_1);
      tn_task_sleep(1000);
   }
}

void task_c_body(void *par)
{
   for(;;)
   {
      mPORTEToggleBits(BIT_2);
      tn_task_sleep(1500);
   }
}

/**
 * Hardware init: called from main() with interrupts disabled
 */
void hw_init(void)
{
   SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

   //turn off ADC function for all pins
   AD1PCFG = 0xffffffff;

   //-- enable timer5 interrupt
   OpenTimer5((0
            | T5_ON
            | T5_IDLE_STOP
            | SYS_TMR_PRESCALER
            | T5_SOURCE_INT
            ),
         (SYS_TMR_PERIOD - 1)
         );

   //-- set timer5 interrupt priority to 2, enable it
   INTSetVectorPriority(INT_TIMER_5_VECTOR, INT_PRIORITY_LEVEL_2);
   INTSetVectorSubPriority(INT_TIMER_5_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
   INTClearFlag(INT_T5);
   INTEnable(INT_T5, INT_ENABLED);

   //-- TNeoKernel PIC32 requirement:
   //   set up the software interrupt 0 with a priority of 1, subpriority 0
   //
   //   NOTE: the ISR is declared in kernel-provided file 
   //   tn_arch_pic32_int_vec1.S, which should be included in the application
   //   project itself, in order to dispatch vector correctly.
   INTSetVectorPriority(INT_CORE_SOFTWARE_0_VECTOR, INT_PRIORITY_LEVEL_1);
   INTSetVectorSubPriority(INT_CORE_SOFTWARE_0_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
   INTClearFlag(INT_CS0);
   INTEnable(INT_CS0, INT_ENABLED);

   //-- configure LED port pins
   mPORTESetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2);
   mPORTEClearBits(BIT_0 | BIT_1 | BIT_2);

   //-- enable multi-vectored interrupt mode
   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
}

/**
 * Application init: called from the first created application task
 */
void appl_init(void)
{
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

int32_t main(void)
{
#ifndef PIC32_STARTER_KIT
   /*The JTAG is on by default on POR.  A PIC32 Starter Kit uses the JTAG, but
     for other debug tool use, like ICD 3 and Real ICE, the JTAG should be off
     to free up the JTAG I/O */
   DDPCONbits.JTAGEN = 0;
#endif

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

void __attribute__((naked, nomips16, noreturn)) _general_exception_handler(void)
{
   PIC32_SOFTWARE_BREAK();
   for (;;) ;
}

