/**
 * TNeoKernel PIC32 basic example
 */

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "p24Fxxxx.h"
#include "tn.h"




/*******************************************************************************
 *    HARDWARE CONFIGURATION
 ******************************************************************************/

// {{{


/*

   Config word 3 Example: Protect code memory from start (protect 128 flash pages) and protect
   configuration block of programm memoty

   __CSP_CONFIG_3(CODE_WRITE_PROT_MEM_START |
   CODE_WRITE_CONF_MEM_EN    |
   (128)
   );
   */

#define __CSP_CONFIG_3(x) __attribute__((section("__CONFIG3.sec,code"))) int __CSP_CONFIG_3 = ((x) | 0x1E00)

/* Write protection location */

#define CODE_WRITE_PROT_MEM_START   (0 << 15)   /* Code write protect from start of programm memory */
#define CODE_WRITE_PROT_MEM_END     (1 << 15)   /* Code write protect from end of programm memory */

/* Write Protect Configuration Page */

#define CODE_WRITE_CONF_MEM_EN      (1 << 14)   /* Code configuration memory block write protect enabled  */
#define CODE_WRITE_CONF_MEM_DIS     (0 << 14)   /* Code configuration memory block write protect disabled */

/* Write protect enable */

#define CODE_WRITE_BLOCK_EN         (0 << 13)   /* Code write protect enabled  */
#define CODE_WRITE_BLOCK_DIS        (1 << 13)   /* Code write protect disabled */


#define __CSP_CONFIG_2(x) __attribute__((section("__CONFIG2.sec,code"))) int __CSP_CONFIG_2 = ((x) | 0x0004)

/* Two Speed Start-up: */

#define TWO_SPEED_STARTUP_EN        (1 << 15)   /* Enabled  */
#define TWO_SPEED_STARTUP_DIS       (0 << 15)   /* Disabled */

/* USB 96 MHz PLL Prescaler Select bits */

#define USB_PLL_PRESCALE_1          (0 << 12)   /* 1:1  */
#define USB_PLL_PRESCALE_2          (1 << 12)   /* 1:2  */
#define USB_PLL_PRESCALE_3          (2 << 12)   /* 1:3  */
#define USB_PLL_PRESCALE_4          (3 << 12)   /* 1:4  */
#define USB_PLL_PRESCALE_5          (4 << 12)   /* 1:5  */
#define USB_PLL_PRESCALE_6          (5 << 12)   /* 1:6  */
#define USB_PLL_PRESCALE_10         (6 << 12)   /* 1:10 */
#define USB_PLL_PRESCALE_12         (7 << 12)   /* 1:12 */

/* Oscillator Selection: */

#define OSC_STARTUP_FRC             (0 <<  8)  /* Fast RC oscillator                     */
#define OSC_STARTUP_FRC_PLL         (1 <<  8)  /* Fast RC oscillator w/ divide and PLL   */
#define OSC_STARTUP_PRIMARY         (2 <<  8)  /* Primary oscillator (XT, HS, EC)        */
#define OSC_STARTUP_PRIMARY_PLL     (3 <<  8)  /* Primary oscillator (XT, HS, EC) w/ PLL */
#define OSC_STARTUP_SECONDARY       (4 <<  8)  /* Secondary oscillator                   */
#define OSC_STARTUP_LPRC            (5 <<  8)  /* Low power RC oscillator                */
#define OSC_STARTUP_FRC_PS          (7 <<  8)  /* Fast RC oscillator with divide         */

/* Clock switching and clock montor: */

#define CLK_SW_EN_CLK_MON_EN        (0 <<  6)  /* Both enabled                 */
#define CLK_SW_EN_CLK_MON_DIS       (1 <<  6)  /* Only clock switching enabled */
#define CLK_SW_DIS_CLK_MON_DIS      (3 <<  6)  /* Both disabled                */

/* OSCO/RC15 function: */

#define OSCO_PIN_CLKO               (1 <<  5)  /* OSCO or Fosc/2 */
#define OSCO_PIN_GPIO               (0 <<  5)  /* RC15           */

/* RP Register Protection */

#define PPS_REG_PROTECT_DIS         (0 <<  4)  /* Unlimited Writes To RP Registers */
#define PPS_REG_PROTECT_EN          (1 <<  4)  /* Write RP Registers Once          */

/* USB regulator control */

#define USB_REG_EN                  (0 <<  3)
#define USB_REG_DIS                 (1 <<  3)

/* Oscillator Selection: */

#define PRIMARY_OSC_EC              (0 <<  0)  /* External clock   */
#define PRIMARY_OSC_XT              (1 <<  0)  /* XT oscillator    */
#define PRIMARY_OSC_HS              (2 <<  0)  /* HS oscillator    */
#define PRIMARY_OSC_DIS             (3 <<  0)  /* Primary disabled */



#define __CSP_CONFIG_1(x) __attribute__((section("__CONFIG1.sec,code"))) int __CSP_CONFIG_1 = ((x) | 0x8420)

/* JTAG: */

#define JTAG_EN                     (1 << 14)  /* Enabled  */
#define JTAG_DIS                    (0 << 14)  /* Disabled */

/* Code Protect: */

#define CODE_PROTECT_EN             (0 << 13)  /* Enabled  */
#define CODE_PROTECT_DIS            (1 << 13)  /* Disabled */

/* Write Protect: */

#define CODE_WRITE_EN               (1 << 12)  /* Enabled  */
#define CODE_WRITE_DIS              (0 << 12)  /* Disabled */

/* Background Debugger: */

#define BACKGROUND_DEBUG_EN         (0 << 11)  /* Enabled  */
#define BACKGROUND_DEBUG_DIS        (1 << 11)  /* Disabled */

/* Clip-on Emulation mode: */

#define EMULATION_EN                (0 << 10)  /* Enabled  */
#define EMULATION_DIS               (1 << 10)  /* Disabled */

/* ICD pins select: */

#define ICD_PIN_PGX3                (1 <<  8)  /* EMUC/EMUD share PGC3/PGD3 */
#define ICD_PIN_PGX2                (2 <<  8)  /* EMUC/EMUD share PGC2/PGD2 */
#define ICD_PIN_PGX1                (3 <<  8)  /* EMUC/EMUD share PGC1/PGD1 */

/* Watchdog Timer: */

#define WDT_EN                      (1 <<  7)  /* Enabled  */
#define WDT_DIS                     (0 <<  7)  /* Disabled */

/* Windowed WDT: */

#define WDT_WINDOW_EN               (0 <<  6)  /* Enabled  */
#define WDT_WINDOW_DIS              (1 <<  6)  /* Disabled */

/* Watchdog prescaler: */

#define WDT_PRESCALE_32             (0 <<  4)  /* 1:32  */
#define WDT_PRESCALE_128            (1 <<  4)  /* 1:128 */


/* Watchdog postscale: */

#define WDT_POSTSCALE_1             ( 0 << 0)  /* 1:1     */
#define WDT_POSTSCALE_2             ( 1 << 0)  /* 1:2     */
#define WDT_POSTSCALE_4             ( 2 << 0)  /* 1:4     */
#define WDT_POSTSCALE_8             ( 3 << 0)  /* 1:8     */
#define WDT_POSTSCALE_16            ( 4 << 0)  /* 1:16    */
#define WDT_POSTSCALE_32            ( 5 << 0)  /* 1:32    */
#define WDT_POSTSCALE_64            ( 6 << 0)  /* 1:64    */
#define WDT_POSTSCALE_128           ( 7 << 0)  /* 1:128   */
#define WDT_POSTSCALE_256           ( 8 << 0)  /* 1:256   */
#define WDT_POSTSCALE_512           ( 9 << 0)  /* 1:512   */
#define WDT_POSTSCALE_1024          (10 << 0)  /* 1:1024  */
#define WDT_POSTSCALE_2048          (11 << 0)  /* 1:2048  */
#define WDT_POSTSCALE_4096          (12 << 0)  /* 1:4096  */
#define WDT_POSTSCALE_8192          (13 << 0)  /* 1:8192  */
#define WDT_POSTSCALE_16384         (14 << 0)  /* 1:16384 */
#define WDT_POSTSCALE_32768         (15 << 0)  /* 1:32768 */

// }}}

//#pragma config CONFIG1H = 0x08;

__CSP_CONFIG_1(JTAG_DIS                     |   /* JTAG disabled                            */
#if defined(__DEBUG)
      CODE_PROTECT_DIS             |   /* Code Protect disabled                    */
#else
      CODE_PROTECT_EN              |   /* TODO:change Code Protect enabled                     */
#endif
      CODE_WRITE_EN                |   /* Code write enabled                       */
#if defined(__DEBUG)
      BACKGROUND_DEBUG_EN          |   /* Debug enabled                            */
      EMULATION_EN                 |   /* Emulation enabled                        */
#else
      BACKGROUND_DEBUG_DIS         |   /* Debug enabled                            */
      EMULATION_DIS                |   /* Emulation enabled                        */
#endif
      ICD_PIN_PGX2                 |   /* PGC2/PGD2 pins used for debug            */
#if defined(__DEBUG)
      WDT_DIS                      |   /* WDT disabled                             */
#else
      WDT_DIS                      |   /* WDT disabled                             */
#endif
      WDT_WINDOW_DIS               |   /* WDT mode is not-windowed                 */
WDT_PRESCALE_32              |   /* WDT prescale is 1:32                     */
WDT_POSTSCALE_1                  /* WDT postscale is 1:1                     */
);

__CSP_CONFIG_2(TWO_SPEED_STARTUP_DIS        |   /* Two-speed startup disabled               */
      USB_PLL_PRESCALE_4           |
      OSC_STARTUP_PRIMARY_PLL      |   /* Startup oscillator is Primary with PLL   */
      CLK_SW_DIS_CLK_MON_DIS       |   /* Clock switch disabled, monitor disabled  */
      OSCO_PIN_CLKO                |   /* OSCO pin is clockout                     */
      PPS_REG_PROTECT_DIS          |   /* PPS runtime change enabled               */
      USB_REG_DIS                  |
      PRIMARY_OSC_HS                   /* Primary oscillator mode is HS            */
      );

__CSP_CONFIG_3(CODE_WRITE_PROT_MEM_START    |   /* Code write protect block from start      */
      CODE_WRITE_CONF_MEM_DIS      |   /* Code config memory block not protected   */
      CODE_WRITE_BLOCK_DIS             /* Code block protect disabled              */
      );





/*******************************************************************************
 *    MACROS
 ******************************************************************************/

//-- instruction that causes debugger to halt
#define PIC24_SOFTWARE_BREAK()  {__asm__ volatile(".pword 0xDA4000"); __asm__ volatile ("nop");}

//-- system frequency
#define SYS_FREQ           32000000UL

//-- kernel ticks (system timer) frequency
#define SYS_TMR_FREQ       1000

//-- system timer prescaler
//#define SYS_TMR_PRESCALER           T5_PS_1_8
#define SYS_TMR_PRESCALER_VALUE     64

//-- system timer period (auto-calculated)
#define SYS_TMR_PERIOD              \
   (SYS_FREQ / SYS_TMR_PRESCALER_VALUE / SYS_TMR_FREQ)




//-- idle task stack size, in words
#define IDLE_TASK_STACK_SIZE          (TN_MIN_STACK_SIZE + 16 + 100/*TODO: remove*/)

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
//tn_soft_isr(_TIMER_5_VECTOR)

void __attribute__((__interrupt__, auto_psv)) _T1Interrupt(void)
{
   IFS0bits.T1IF   = 0;
   volatile int b;
   b = 1;
   b = _tn_arch_is_int_disabled();
   b = 1;
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
      //mPORTEToggleBits(BIT_0);
      LATE ^= (1 << 0);
      tn_task_sleep(50);
   }
}

void task_b_body(void *par)
{
   for(;;)
   {
      //mPORTEToggleBits(BIT_1);
      LATE ^= (1 << 1);
      tn_task_sleep(100);
   }
}

void task_c_body(void *par)
{
   for(;;)
   {
      //mPORTEToggleBits(BIT_2);
      LATE ^= (1 << 2);
      tn_task_sleep(150);
   }
}

/**
 * Hardware init: called from main() with interrupts disabled
 */
void hw_init(void)
{
   //SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

   //turn off ADC function for all pins
   //AD1PCFG = 0xffffffff;

   //-- enable timer5 interrupt
   T1CONbits.TCS   = 0;
   T1CONbits.TGATE = 0;
   T1CONbits.TSIDL = 1;

   T1CONbits.TCKPS = 2;        // 1:64
   PR1             = (SYS_TMR_PERIOD - 1);

   IFS0bits.T1IF   = 0;
   IEC0bits.T1IE   = 1;
   IPC0bits.T1IP   = 2;
   T1CONbits.TON   = 1;

   IEC0bits.INT0IE = 1;
   IPC0bits.INT0IP = 1;
}

/**
 * Application init: called from the first created application task
 */
void appl_init(void)
{
   //-- configure LED port pins
   //mPORTESetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2);
   //mPORTEClearBits(BIT_0 | BIT_1 | BIT_2);

   TRISEbits.TRISE0 = 0;
   TRISEbits.TRISE1 = 0;
   TRISEbits.TRISE2 = 0;

   //-- initialize various on-board peripherals, such as
   //   flash memory, displays, etc.
   //   (in this sample project there's nothing to init)

   //-- initialize various program modules
   //   (in this sample project there's nothing to init)


   //-- create all the rest application tasks
#if 1
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
#endif
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
#if 1
   tn_task_create(
         &task_a,                   //-- task structure
         task_a_body,               //-- task body function
         TASK_A_PRIORITY,           //-- task priority
         task_a_stack,              //-- task stack
         TASK_A_STK_SIZE,           //-- task stack size (in words)
         NULL,                      //-- task function parameter
         TN_TASK_CREATE_OPT_START   //-- creation option
         );

#endif
}

int main(void)
{
#ifndef PIC32_STARTER_KIT
   /*The JTAG is on by default on POR.  A PIC32 Starter Kit uses the JTAG, but
     for other debug tool use, like ICD 3 and Real ICE, the JTAG should be off
     to free up the JTAG I/O */
   //DDPCONbits.JTAGEN = 0;
#endif

#if 1
   //-- unconditionally disable interrupts
   tn_arch_int_dis();
#endif

   //-- init hardware
   hw_init();

#if 0
   for (;;){
      volatile int i;
      i++;
      if (i > 10){
         i = 0;
         IFS1bits.INT1IF = 1;
      }
   }
#endif

#if 1
   //-- call to tn_sys_start() never returns
   tn_sys_start(
         idle_task_stack,
         IDLE_TASK_STACK_SIZE,
         interrupt_stack,
         INTERRUPT_STACK_SIZE,
         init_task_create,
         idle_task_callback
         );
#endif

   //-- unreachable
   return 1;
}





void __attribute((__interrupt__, auto_psv)) _AddressError (void)
{
   PIC24_SOFTWARE_BREAK();
   //for(;;);
}

/**
 *  
 */
void __attribute((__interrupt__, auto_psv)) _StackError (void)
{
   PIC24_SOFTWARE_BREAK();
   //    for(;;);
}

/**
 *  
 */
void __attribute((__interrupt__, auto_psv)) _MathError (void)
{
   PIC24_SOFTWARE_BREAK();
   //for(;;);
}

/**
 *  
 */
void __attribute((__interrupt__, auto_psv)) _OscillatorFail (void)
{
   PIC24_SOFTWARE_BREAK();
   //for(;;);
}

/**
 * 
 *
 * 
 */
void __attribute((__interrupt__, auto_psv)) _DefaultInterrupt (void)
{
   PIC24_SOFTWARE_BREAK();
   //for(;;);
}
