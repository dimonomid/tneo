/*******************************************************************************
 *
 * TNeo: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright 2013, 2014 Anders Montonen.
 *    TNeo:                      copyright 2014       Dmitry Frank.
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
 * Cortex-M0/M0+/M3/M4/M4F architecture-dependent routines
 *
 */

#ifndef  _TN_ARCH_CORTEX_M_H
#define  _TN_ARCH_CORTEX_M_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "../tn_arch_detect.h"
#include "../../core/tn_cfg_dispatch.h"




#ifdef __cplusplus
extern "C"  {     /*}*/
#endif


/*******************************************************************************
 *    ARCH-DEPENDENT DEFINITIONS
 ******************************************************************************/







#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define  _TN_PIC32_INTSAVE_DATA_INVALID   0xffffffff

#if TN_DEBUG
#  define   _TN_PIC32_INTSAVE_CHECK()                          \
{                                                              \
   if (TN_INTSAVE_VAR == _TN_PIC32_INTSAVE_DATA_INVALID){      \
      _TN_FATAL_ERROR("");                                     \
   }                                                           \
}
#else
#  define   _TN_PIC32_INTSAVE_CHECK()  /* nothing */
#endif

#if defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
/**
 * FFS - find first set bit. Used in `_find_next_task_to_run()` function.
 * Say, for `0xa8` it should return `3`.
 *
 * May be not defined: in this case, naive algorithm will be used.
 */
#define  _TN_FFS(x)     ffs_asm(x)
int ffs_asm(int x);
#endif

/**
 * Used by the kernel as a signal that something really bad happened.
 * Indicates TNeo bugs as well as illegal kernel usage
 * (e.g. sleeping in the idle task callback)
 *
 * Typically, set to assembler instruction that causes debugger to halt.
 */

#if defined(__TN_COMPILER_IAR__)
#  define  _TN_FATAL_ERROR(error_msg, ...)         \
      {asm("bkpt #0");}
#else
#  define  _TN_FATAL_ERROR(error_msg, ...)         \
      {__asm__ volatile("bkpt #0");}
#endif



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

#if defined(__TN_COMPILER_ARMCC__)

#  define TN_ARCH_STK_ATTR_BEFORE      __align(8)
#  define TN_ARCH_STK_ATTR_AFTER

#elif defined(__TN_COMPILER_GCC__) || defined(__TN_COMPILER_CLANG__)

#  define TN_ARCH_STK_ATTR_BEFORE
#  define TN_ARCH_STK_ATTR_AFTER       __attribute__((aligned(0x08)))

#elif defined(__TN_COMPILER_IAR__)

#  define TN_ARCH_STK_ATTR_BEFORE
#  define TN_ARCH_STK_ATTR_AFTER

#endif


#if defined(__TN_ARCHFEAT_CORTEX_M_FPU__)
#  define _TN_CORTEX_FPU_CONTEXT_SIZE 32 /* FPU registers: S0 .. S31 */
#else
#  define _TN_CORTEX_FPU_CONTEXT_SIZE 0  /* no FPU registers */
#endif


/**
 * Minimum task's stack size, in words, not in bytes; includes a space for
 * context plus for parameters passed to task's body function.
 */
#define  TN_MIN_STACK_SIZE          (17 /* context: 17 words */   \
      + _TN_STACK_OVERFLOW_SIZE_ADD                               \
      + _TN_CORTEX_FPU_CONTEXT_SIZE                               \
      )

/**
 * Width of `int` type.
 */
#define  TN_INT_WIDTH               32

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
#define  TN_FILL_STACK_VAL          0xFEEDFACE




/**
 * Variable name that is used for storing interrupts state
 * by macros TN_INTSAVE_DATA and friends
 */
#define TN_INTSAVE_VAR              tn_save_status_reg

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
   TN_UWord TN_INTSAVE_VAR = _TN_PIC32_INTSAVE_DATA_INVALID;

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

#define TN_INT_DIS_SAVE()   TN_INTSAVE_VAR = tn_arch_sr_save_int_dis()
#define TN_INT_RESTORE()    _TN_PIC32_INTSAVE_CHECK();                      \
                            tn_arch_sr_restore(TN_INTSAVE_VAR)

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
#define _TN_SIZE_BYTES_TO_UWORDS(size_in_bytes)    ((size_in_bytes) >> 2)

#if defined(__TN_COMPILER_ARMCC__)
#  if TN_FORCED_INLINE
#     define _TN_INLINE             __forceinline
#  else
#     define _TN_INLINE             __inline
#  endif
#  define _TN_STATIC_INLINE         static _TN_INLINE
#  define _TN_VOLATILE_WORKAROUND   /* nothing */
#elif defined(__TN_COMPILER_GCC__) || defined(__TN_COMPILER_CLANG__)
#  if TN_FORCED_INLINE
#     define _TN_INLINE             inline __attribute__ ((always_inline))
#  else
#     define _TN_INLINE             inline
#  endif
#  define _TN_STATIC_INLINE         static _TN_INLINE
#  define _TN_VOLATILE_WORKAROUND   /* nothing */
#elif defined(__TN_COMPILER_IAR__)
#  if TN_FORCED_INLINE
#     define _TN_INLINE             _Pragma("inline=forced")
#  else
#     define _TN_INLINE             inline
#  endif
/*
 * NOTE: for IAR, `_TN_INLINE` should go before `static`, because
 * when we use forced inline by _Pragma, and `static` is before _Pragma,
 * then IAR compiler generates a warning that pragma should immediately
 * precede the declaration.
 */
#  define _TN_STATIC_INLINE         _TN_INLINE static
#  define _TN_VOLATILE_WORKAROUND   volatile
#else
#  error unknown Cortex compiler
#endif

#define _TN_ARCH_STACK_PT_TYPE   _TN_ARCH_STACK_PT_TYPE__FULL
#define _TN_ARCH_STACK_DIR       _TN_ARCH_STACK_DIR__DESC

#endif   //-- DOXYGEN_SHOULD_SKIP_THIS











#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _TN_ARCH_CORTEX_M_H

