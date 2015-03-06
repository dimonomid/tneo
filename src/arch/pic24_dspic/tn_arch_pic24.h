/*******************************************************************************
 *
 * TNeo: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeo:                      copyright © 2014       Dmitry Frank.
 *
 *    TNeo was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeo.
 *
 *    Permission to use, copy, modify, and distribute this software in source
 *    and binary forms and its documentation for any purpose and without fee
 *    is hereby granted, provided that the above copyright notice appear
 *    in all copies and that both that copyright notice and this permission
 *    notice appear in supporting documentation.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE DMITRY FRANK AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DMITRY FRANK OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *    THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/**
 *
 * \file
 *
 * PIC24/dsPIC architecture-dependent routines
 *
 */

#ifndef  _TN_ARCH_PIC24_H
#define  _TN_ARCH_PIC24_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- Include `tn_common_macros.h` for `_TN_STRINGIZE_MACRO()`
#include "../../core/tn_common_macros.h"
#include "../../core/tn_cfg_dispatch.h"

//-- include macros for atomic assess to structure bit fields so that
//   application can use it too.
#include "tn_arch_pic24_bfa.h"


#ifdef __cplusplus
extern "C"  {     /*}*/
#endif



/*******************************************************************************
 *    ARCH-DEPENDENT DEFINITIONS
 ******************************************************************************/

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define  _TN_PIC24_INTSAVE_DATA_INVALID   0xffff

#if TN_DEBUG
#  define   _TN_PIC24_INTSAVE_CHECK()                          \
{                                                              \
   if (tn_save_status_reg == _TN_PIC24_INTSAVE_DATA_INVALID){  \
      _TN_FATAL_ERROR("");                                     \
   }                                                           \
}
#else
#  define   _TN_PIC24_INTSAVE_CHECK()  /* nothing */
#endif

/**
 * FFS - find first set bit. Used in `_find_next_task_to_run()` function.
 * Say, for `0xa8` it should return `3`.
 *
 * May be not defined: in this case, naive algorithm will be used.
 */
#define  _TN_FFS(x)     _tn_p24_ffs_asm(x)
int _tn_p24_ffs_asm(int x);

/**
 * Used by the kernel as a signal that something really bad happened.
 * Indicates TNeo bugs as well as illegal kernel usage
 * (e.g. sleeping in the idle task callback)
 *
 * Typically, set to assembler instruction that causes debugger to halt.
 */
#define  _TN_FATAL_ERROR(error_msg, ...)         \
   {__asm__ volatile(".pword 0xDA4000"); __asm__ volatile ("nop");}



/**
 * \def TN_ARCH_STK_ATTR_BEFORE
 *
 * Compiler-specific attribute that should be placed **before** declaration of
 * array used for stack. It is needed because there are often additional 
 * restrictions applied to alignment of stack, so, to meet them, stack arrays
 * need to be declared with these macros.
 *
 * @see TN_ARCH_STK_ATTR_AFTER
 */

/**
 * \def TN_ARCH_STK_ATTR_AFTER
 *
 * Compiler-specific attribute that should be placed **after** declaration of
 * array used for stack. It is needed because there are often additional 
 * restrictions applied to alignment of stack, so, to meet them, stack arrays
 * need to be declared with these macros.
 *
 * @see TN_ARCH_STK_ATTR_BEFORE
 */

#if defined (__C30__)
#  define TN_ARCH_STK_ATTR_BEFORE
#  define TN_ARCH_STK_ATTR_AFTER
#else
#  error "Unknown compiler"
#endif

/**
 * Minimum task's stack size, in words, not in bytes; includes a space for
 * context plus for parameters passed to task's body function.
 */
#define  TN_MIN_STACK_SIZE          (25            \
      + _TN_EDS_STACK_ADD                          \
      + _TN_STACK_OVERFLOW_SIZE_ADD                \
      )

/**
 * Some devices have two registers: DSRPAG and DSWPAG instead of PSVPAG.
 * If __HAS_EDS__ is defined, device has two these registers,
 * so we should take this in account.
 */
#ifdef __HAS_EDS__
#  define   _TN_EDS_STACK_ADD       1
#else
#  define   _TN_EDS_STACK_ADD       0
#endif


/**
 * Width of `int` type.
 */
#define  TN_INT_WIDTH               16

/**
 * Unsigned integer type whose size is equal to the size of CPU register.
 * Typically it's plain `unsigned int`.
 */
typedef  unsigned int               TN_UWord;

/**
 * Unsigned integer type that is able to store pointers.
 * We need it because some platforms don't define `uintptr_t`.
 * Typically it's `unsigned int`.
 */
typedef  unsigned int               TN_UIntPtr;


/**
 * Maximum number of priorities available, this value usually matches
 * `#TN_INT_WIDTH`.
 *
 * @see TN_PRIORITIES_CNT
 */
#define  TN_PRIORITIES_MAX_CNT      TN_INT_WIDTH

/**
 * Value for infinite waiting, usually matches `ULONG_MAX`,
 * because `#TN_TickCnt` is declared as `unsigned long`.
 */
#define  TN_WAIT_INFINITE           (TN_TickCnt)0xFFFFFFFF

/**
 * Value for initializing the task's stack
 */
#define  TN_FILL_STACK_VAL          0xFEED




/**
 * Declares variable that is used by macros `TN_INT_DIS_SAVE()` and
 * `TN_INT_RESTORE()` for storing status register value.
 *
 * It is good idea to initially set it to some invalid value,
 * and if TN_DEBUG is non-zero, check it in TN_INT_RESTORE().
 * Then, we can catch bugs if someone tries to restore interrupts status
 * without saving it first.
 *
 * @see `TN_INT_DIS_SAVE()`
 * @see `TN_INT_RESTORE()`
 */
#define  TN_INTSAVE_DATA            \
   int tn_save_status_reg = _TN_PIC24_INTSAVE_DATA_INVALID;

/**
 * The same as `#TN_INTSAVE_DATA` but for using in ISR together with
 * `TN_INT_IDIS_SAVE()`, `TN_INT_IRESTORE()`.
 *
 * @see `TN_INT_IDIS_SAVE()`
 * @see `TN_INT_IRESTORE()`
 */
#define  TN_INTSAVE_DATA_INT        TN_INTSAVE_DATA

/**
 * \def TN_INT_DIS_SAVE()
 *
 * Disable interrupts and return previous value of status register,
 * atomically. Similar `tn_arch_sr_save_int_dis()`, but implemented
 * as a macro, so it is potentially faster.
 *
 * Uses `#TN_INTSAVE_DATA` as a temporary storage.
 *
 * @see `#TN_INTSAVE_DATA`
 * @see `tn_arch_sr_save_int_dis()`
 */

/**
 * \def TN_INT_RESTORE()
 *
 * Restore previously saved status register.
 * Similar to `tn_arch_sr_restore()`, but implemented as a macro,
 * so it is potentially faster.
 *
 * Uses `#TN_INTSAVE_DATA` as a temporary storage.
 *
 * @see `#TN_INTSAVE_DATA`
 * @see `tn_arch_sr_save_int_dis()`
 */

#  define TN_INT_DIS_SAVE()   tn_save_status_reg = tn_arch_sr_save_int_dis()
#  define TN_INT_RESTORE()    _TN_PIC24_INTSAVE_CHECK();                      \
                              tn_arch_sr_restore(tn_save_status_reg)

/**
 * The same as `TN_INT_DIS_SAVE()` but for using in ISR.
 *
 * Uses `#TN_INTSAVE_DATA_INT` as a temporary storage.
 *
 * @see `#TN_INTSAVE_DATA_INT`
 */
#define TN_INT_IDIS_SAVE()       TN_INT_DIS_SAVE()

/**
 * The same as `TN_INT_RESTORE()` but for using in ISR.
 *
 * Uses `#TN_INTSAVE_DATA_INT` as a temporary storage.
 *
 * @see `#TN_INTSAVE_DATA_INT`
 */
#define TN_INT_IRESTORE()        TN_INT_RESTORE()

/**
 * Returns nonzero if interrupts are disabled, zero otherwise.
 */
#define TN_IS_INT_DISABLED()     (_tn_arch_is_int_disabled())

/**
 * Pend context switch from interrupt.
 */
#define _TN_CONTEXT_SWITCH_IPEND_IF_NEEDED()          \
   _tn_context_switch_pend_if_needed()

/**
 * Converts size in bytes to size in `#TN_UWord`.
 * For 32-bit platforms, we should shift it by 2 bit to the right;
 * for 16-bit platforms, we should shift it by 1 bit to the right.
 */
#define _TN_SIZE_BYTES_TO_UWORDS(size_in_bytes)    ((size_in_bytes) >> 1)

#define _TN_INLINE   inline

#define _TN_VOLATILE_WORKAROUND   /* nothing */

//-- internal interrupt macro stuff {{{

#if TN_CHECK_PARAM

/**
 * Check whether priority is too high. On PIC24 port, we have a range of
 * interrupt priorities (let's call this range as "system priority"), and
 * kernel functions are allowed to call only from ISRs with priority in this
 * range. 
 *
 * As a result, the kernel (almost) never disables ALL interrupts: when it
 * modifies critical data, it disables just interrupts with system priority.
 *
 * The "almost" is because it disables interrupts for 4-8 cycles when it modifies
 * stack pointer and SPLIM, because they should always correspond.
 */
#  define _TN_SOFT_ISR_PRIORITY_CHECK()                                       \
   "                                                                    \n"   \
   "   mov     #0xE0,   W0                                              \n"   \
   "   and     _SR,     WREG                                            \n"   \
   "   lsr     W0,      #5,      W0                                     \n"   \
   "   cp      W0,      #" _TN_STRINGIZE_MACRO(TN_P24_SYS_IPL) "        \n"   \
   "   bra     leu,     1f                                              \n"   \
                                                                              \
   /* Interrupt priority is too high. Halt the debugger here.           */    \
   "   .pword  0xDA4000                                                 \n"   \
   "   nop                                                              \n"   \
   "1:                                                                  \n"   \
   /* Interrupt priority is ok, go on now.                              */    \

#else
#  define _TN_SOFT_ISR_PRIORITY_CHECK()  /* nothing */
#endif


#define _TN_SOFT_ISR_PROLOGUE                                                 \
   "                                                                    \n"   \
                                                                              \
   /* we need to use a couple of registers, so, save them.              */    \
   "   push   w0;                                                       \n"   \
   "   push   w1;                                                       \n"   \
                                                                              \
   /* if TN_CHECK_PARAM is enabled, check if interrupt priority is      */    \
   /* too high. See macro _TN_SOFT_ISR_PRIORITY_CHECK() above for       */    \
   /* details.                                                          */    \
   _TN_SOFT_ISR_PRIORITY_CHECK()                                              \
                                                                              \
   /* before playing with SP and SPLIM, we need to disable system       */    \
   /* interrupts. NOTE that 'disi' instruction does not disable         */    \
   /* interrupts with priority 7, so, system interrupt priority should  */    \
   /* never be 7.                                                       */    \
                                                                              \
   "   disi      #8;                                                    \n"   \
                                                                              \
   /* check if SP is already inside the interrupt stack:                */    \
   /* we check it by checking if SPLIM is set to _tn_p24_int_splim      */    \
   "   mov __tn_p24_int_splim, w0;                                      \n"   \
   "   cp SPLIM;                                                        \n"   \
   "   bra z, 1f;                                                       \n"   \
                                                                              \
   /* SP is not inside the interrupt stack. We should set it.           */    \
   "                                                                    \n"   \
   "   mov     SPLIM,   w1;                                             \n"   \
   "   mov     w0,      SPLIM;                                          \n"   \
   "   mov     w15,     w0;                                             \n"   \
   "   mov     __tn_p24_int_stack_low_addr, w15;                        \n"   \
                                                                              \
   /* Interrupts should be re-enabled here.                             */    \
   /* We just switched to interrupt stack.                              */    \
   /* we need to push previous stack pointer and SPLIM there.           */    \
   "   push    w0;                  \n"   /* push task's SP             */    \
   "   push    w1;                  \n"   /* push task's SPLIM          */    \
   "   bra     2f;                                                      \n"   \
   "1:                                                                  \n"   \
   /* Interrupt stack is already active (it happens when interrupts     */    \
   /* nest)                                                             */    \
   /* Just push SP and SPLIM so that stack contents will be compatible  */    \
   /* with the case of non-nested interrupt                             */    \
   "   push    w15;                 \n"   /* push SP                    */    \
   "   push    SPLIM;               \n"   /* push SPLIM                 */    \
   "2:                                                                  \n"   \
   "                                                                    \n"   \


#define _TN_SOFT_ISR_CALL                                                     \
   /* before we call user-provided ISR, we need to imitate interrupt    */    \
   /* call, i.e. store SR to the stack. It is done below,               */    \
   /* at the label 1:                                                   */    \
   "   rcall   1f;                                                      \n"

#define _TN_SOFT_ISR_EPILOGUE                                                 \
   /* we got here when we just returned from user-provided ISR.         */    \
   /* now, we need to restore previous SPLIM and SP, and we should      */    \
   /* disable interrupts because they could nest, and if SPLIM and SP   */    \
   /* don't correspond, system crashes.                                 */    \
   "   disi    #4;                                                      \n"   \
   "   pop     w1;                  \n"   /* pop SPLIM                  */    \
   "   pop     w0;                  \n"   /* pop SP                     */    \
   "   mov     w1,   SPLIM;                                             \n"   \
   "   mov     w0,   w15;                                               \n"   \
                                                                              \
   /* now, interrupts should be enabled back.                           */    \
   /* here we just need to restore w0 and w1 that we saved and used in  */    \
   /* _TN_SOFT_ISR_PROLOGUE                                             */    \
   "   pop      w1;                                                     \n"   \
   "   pop      w0;                                                     \n"   \
                                                                              \
   /* finally, return from the ISR.                                     */    \
   "   retfie;                                                          \n"   \
   "1:                                                                  \n"   \
   /* we got here by rcall when we are about to call user-provided ISR. */    \
   /* 'rcall' saves program counter to the stack, but we need to        */    \
   /* imitate interrupt, so, we manually save SR there.                 */    \
   "   mov      SR, w0;                                                 \n"   \
   "   mov.b    w0, [w15-1];                                            \n"   \
                                                                              \
   /* now, we eventually proceed to user-provided ISR.                  */    \
   /* When it returns, we get above to the macro _TN_SOFT_ISR_EPILOGUE  */    \
   /* (because 'rcall' saved this address to the stack)                 */    \



#define _tn_soft_isr_internal(_func, _psv, _shadow)                           \
   void __attribute__((                                                       \
            __interrupt__(                                                    \
               __preprologue__(                                               \
                   _TN_SOFT_ISR_PROLOGUE                                      \
                   _TN_SOFT_ISR_CALL                                          \
                   _TN_SOFT_ISR_EPILOGUE                                      \
               )                                                              \
            ),                                                                \
            _psv,                                                             \
            _shadow                                                           \
         ))                                                                   \
      _func(void)


// }}}

#define _TN_ARCH_STACK_PT_TYPE   _TN_ARCH_STACK_PT_TYPE__EMPTY
#define _TN_ARCH_STACK_DIR       _TN_ARCH_STACK_DIR__ASC

#endif   //-- DOXYGEN_SHOULD_SKIP_THIS










/**
 * ISR wrapper macro for software context saving.
 *
 * Usage looks like the following:
 *
 * \code{.c}
 * tn_p24_soft_isr(_T1Interrupt, auto_psv)
 * {
 *    //-- clear interrupt flag
 *    IFS0bits.T1IF = 0;
 *
 *    //-- do something useful
 * }
 * \endcode
 *
 * Which should be used for system interrupts, instead of standard way:
 *
 * \code{.c}
 * void __attribute__((__interrupt__, auto_psv)) _T1Interrupt(void)
 * \endcode
 *
 * Where `_T1Interrupt` is the usual PIC24/dsPIC ISR name,
 * and `auto_psv` (or `no_auto_psv`) is the usual attribute argument for 
 * interrupt.
 *
 *
 */
#define  tn_p24_soft_isr(_func, _psv)  _tn_soft_isr_internal(_func, _psv, )




#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _TN_ARCH_PIC24_H

