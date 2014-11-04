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

#include "example_arch.h"



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
tn_p32_soft_isr(_TIMER_5_VECTOR)
{
   INTClearFlag(INT_T5);
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
   SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

   //-- turn off ADC function for all pins
   AD1PCFG = 0xffffffff;

   //-- Set power-saving mode to an idle mode
   {
      SYSKEY = 0x0;
      SYSKEY = 0xAA996655;
      SYSKEY = 0x556699AA;

      OSCCONCLR = 0x10;

      SYSKEY = 0x0;
   }

   //-- enable timer5 interrupt.
   //   It is set to continue in the idle mode, so that system timer
   //   will wake system up each system tick.
   OpenTimer5((0
            | T5_ON
            | T5_IDLE_CON
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

   //-- enable multi-vectored interrupt mode
   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
}

//-- idle callback that is called periodically from idle task
void idle_task_callback (void)
{
   //TODO: uncomment if you want the idle task to use real power-saving mode.
   //      actual mode to use ("idle") is set up in the function hw_init(),
   //      here it is just activated.
   //asm volatile ("wait");
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
   SOFTWARE_BREAK();
   for (;;) ;
}

