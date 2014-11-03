/**
 * TNeoKernel PIC32 basic example
 */

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include <xc.h>
#include "tn.h"

#include "example_arch.h"



/*******************************************************************************
 *    PIC32 HARDWARE CONFIGURATION
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



/*******************************************************************************
 *    EXTERN FUNCTION PROTOTYPE
 ******************************************************************************/

//-- defined by particular example: create first application task(s)
extern void init_task_create(void);



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

//-- idle callback that is called periodically from idle task
void idle_task_callback (void)
{
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
   SOFTWARE_BREAK();
   //for(;;);
}

void __attribute((__interrupt__, auto_psv)) _StackError (void)
{
   SOFTWARE_BREAK();
   //for(;;);
}

void __attribute((__interrupt__, auto_psv)) _MathError (void)
{
   SOFTWARE_BREAK();
   //for(;;);
}

void __attribute((__interrupt__, auto_psv)) _OscillatorFail (void)
{
   SOFTWARE_BREAK();
   //for(;;);
}

void __attribute((__interrupt__, auto_psv)) _DefaultInterrupt (void)
{
   SOFTWARE_BREAK();
   //for(;;);
}


