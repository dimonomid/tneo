/* ******************************************************************************
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
 * TNeo architecture-dependent routines for Cortex-M0/M0+/M1/M3/M4/M4F.
 *
 * Assemblers supported:
 *
 * - GCC
 * - clang
 * - ARMCC
 * - IAR
 *
 * NOTE: this file should be parsed by C preprocessor before assembling.
 * 
 * If your toolchain is unable to do it for you automatically (say, Keil
 * uVision + GCC seem unable), it is possible for you to issue cpp (C
 * preprocessor) manually. Say, for GNU assembler and Cortex-M3, the following
 * command does the trick:
 *
 *    cpp -P -undef tn_arch_cortex_m.S                         \
 *          -D __GNUC__ -D __ARM_ARCH -D __ARM_ARCH_7M__       \
 *          -I ../.. -I ../../core                             \
 *          > tn_arch_cortex_m3_gcc.s
 *
 * And then, you should include the output file tn_arch_cortex_m3_gcc.s to your
 * project, instead of this one (tn_arch_cortex_m.S).
 *
 * For other cores/compilers, you need to change predefined macros ( -D ... ),
 * you can check the file src/arch/tn_arch_detect.h to see what it expects for
 * each particular core/compiler.
 * 
 * Say, when GCC toolchain is used from Keil uVision IDE, for .S files it calls
 * arm-none-eabi-as, which does not call C preprocessor.
 *
 * Instead, arm-none-eabi-gcc should be used, but unfortunately I was unable to
 * make Keil uVision issue arm-none-eabi-gcc for .S files, the only way to use
 * GCC toolchain in Keil uVision that I'm aware of is to preprocess the file
 * manually.
 *
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "../tn_arch_detect.h"
#include "tn_cfg_dispatch.h"






/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#if defined(__TN_COMPILER_ARMCC__)

#  define   _TN_NAME(x)             x
#  define   _TN_LOCAL_NAME(x)       x
#  define   _TN_EXTERN(x)           IMPORT _TN_NAME(x)
#  define   _TN_GLOBAL(x)           EXPORT _TN_NAME(x)
#  define   _TN_EQU(symbol, value)  symbol EQU value
#  define   _TN_LABEL(label)        _TN_NAME(label)
#  define   _TN_LOCAL_LABEL(label)  _TN_LOCAL_NAME(label)
#  define   _TN_END()               END
#  define   _TN_THUMB_FUNC()        /* nothing */

#elif defined(__TN_COMPILER_IAR__)

#  define   _TN_NAME(x)             x
#  define   _TN_LOCAL_NAME(x)       x
#  define   _TN_EXTERN(x)           IMPORT x
#  define   _TN_GLOBAL(x)           EXPORT x
#  define   _TN_EQU(symbol, value)  symbol EQU value
#  define   _TN_LABEL(label)        label:
#  define   _TN_LOCAL_LABEL(label)  label:
#  define   _TN_END()               END
#  define   _TN_THUMB_FUNC()        /* nothing */

#elif defined(__TN_COMPILER_GCC__)

#  define   _TN_NAME(x)             x
//-- We need to declare local labels prepended with "L" so that they aren't
//   stripped by .subsections_via_symbols
#  define   _TN_LOCAL_NAME(x)       L ## x
#  define   _TN_EXTERN(x)           .extern _TN_NAME(x)
#  define   _TN_GLOBAL(x)           .global _TN_NAME(x)
#  define   _TN_EQU(symbol, value)  .equ symbol, value
#  define   _TN_LABEL(label)        _TN_NAME(label):
#  define   _TN_LOCAL_LABEL(label)  _TN_LOCAL_NAME(label):
#  define   _TN_END()               /* nothing */
#  define   _TN_THUMB_FUNC()        .thumb_func

#elif defined(__TN_COMPILER_CLANG__)

#  define   _TN_NAME(x)             _ ## x
//-- We need to declare local labels prepended with "L" so that they aren't
//   stripped by .subsections_via_symbols
#  define   _TN_LOCAL_NAME(x)       L ## x
#  define   _TN_EXTERN(x)           .extern _TN_NAME(x)
#  define   _TN_GLOBAL(x)           .global _TN_NAME(x)
#  define   _TN_EQU(symbol, value)  .equ symbol, value
#  define   _TN_LABEL(label)        _TN_NAME(label):
#  define   _TN_LOCAL_LABEL(label)  _TN_LOCAL_NAME(label):
#  define   _TN_END()               /* nothing */
#  define   _TN_THUMB_FUNC()        .thumb_func

#else
#  error unknown compiler for Cortex-M
#endif


/*
 * Since we have on-context-switch handler (see _TN_ON_CONTEXT_SWITCH_HANDLER),
 * LR is corrupted during context switch. So, we need to save it.
 * On Cortex M3/M4/M4F we have EXC_RETURN right in the task context,
 * so it is already saved and restored just fine.
 *
 * (actually saving of EXC_RETURN in task context is necessary for M4F only,
 * because usage of FPU is the only thing that can differ in EXC_RETURN for
 * different tasks, but I don't want to complicate things even more)
 *
 * On M0/M0+ there's no need to store EXC_RETURN in the task context, but if
 * there is some on-context-switch handler, then we should save/restore LR on
 * MSP during context switch.
 */
#define     _TN_NEED_SAVE_LR()      (                                         \
      !defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)                           \
      && _TN_ON_CONTEXT_SWITCH_HANDLER                                        \
      )




/*******************************************************************************
 *    ASM PROLOGUE
 ******************************************************************************/

#if defined(__TN_COMPILER_ARMCC__)

   AREA    |.text|, CODE, READONLY, ALIGN=3
   THUMB
   REQUIRE8
   PRESERVE8

#elif defined(__TN_COMPILER_IAR__)

   SECTION  .text:CODE(2)
   THUMB
   REQUIRE8
   PRESERVE8

#elif defined(__TN_COMPILER_GCC__)

   .text
   .syntax unified
   .thumb

#elif defined(__TN_COMPILER_CLANG__)

   .text
   .syntax unified
   .thumb

#else
#  error unknown compiler for Cortex-M
#endif




/*******************************************************************************
 *    EXTERN SYMBOLS
 ******************************************************************************/

   _TN_EXTERN(_tn_curr_run_task)
   _TN_EXTERN(_tn_next_task_to_run)
#if _TN_ON_CONTEXT_SWITCH_HANDLER
   _TN_EXTERN(_tn_sys_on_context_switch)
#endif



/*******************************************************************************
 *    PUBLIC SYMBOLS
 ******************************************************************************/

#if defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
   _TN_GLOBAL(ffs_asm)
#endif

   _TN_GLOBAL(PendSV_Handler)
   _TN_GLOBAL(SVC_Handler)
   _TN_GLOBAL(_tn_arch_sys_start)
   _TN_GLOBAL(_tn_arch_context_switch_now_nosave)
   _TN_GLOBAL(tn_arch_int_dis)
   _TN_GLOBAL(tn_arch_int_en)

   _TN_GLOBAL(tn_arch_sr_save_int_dis)
   _TN_GLOBAL(tn_arch_sr_restore)
   _TN_GLOBAL(_tn_arch_is_int_disabled)
   _TN_GLOBAL(_tn_arch_inside_isr)
   _TN_GLOBAL(_tn_arch_context_switch_pend)
   _TN_GLOBAL(tn_arch_sched_dis_save)
   _TN_GLOBAL(tn_arch_sched_restore)



/*******************************************************************************
 *    CONSTANTS
 ******************************************************************************/

//-- Interrupt Control State Register Address
_TN_EQU(ICSR_ADDR, 0xE000ED04)

//-- PendSV bit in the Interrupt Control State Register
_TN_EQU(PENDSVSET, 0x10000000)

//-- System Handlers 12-15 Priority Register Address
_TN_EQU(PR_08_11_ADDR, 0xE000ED1C)

//-- System Handlers 12-15 Priority Register Address
_TN_EQU(PR_12_15_ADDR, 0xE000ED20)

//-- PRI_14 (PendSV) priority in the System Handlers 12-15 Priority Register Address
//   PendSV priority is minimal (0xFF)
_TN_EQU(PENDS_VPRIORITY, 0x00FF0000)

//-- PRI_11 (SVC) priority in the System Handlers 08-11 Priority Register Address
//   SVC priority is minimal (0xFF)
_TN_EQU(SVC_VPRIORITY, 0xFF000000)

//-- We need to clear LSPEN bit in FPU->FPCCR, so that here's
//   address and mask
_TN_EQU(FPU_FPCCR_ADDR, 0xE000EF34)
_TN_EQU(FPU_FPCCR_LSPEN, 0xBFFFFFFF)




/*******************************************************************************
 *    CODE
 ******************************************************************************/

#if defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
_TN_THUMB_FUNC()
_TN_LABEL(ffs_asm)

      mov      r1, r0                  //-- tmp = in
      rsbs     r0, r1, #0              //-- in = -in
      ands     r0, r0, r1              //-- in = in & tmp
      clz      r0, r0
      rsb      r0, r0, #0x20           //-- 32 - in
      bx       lr
#endif



_TN_THUMB_FUNC()
_TN_LABEL(PendSV_Handler)

      //-- Save LR if needed (see comment for macro _TN_NEED_SAVE_LR())
#if _TN_NEED_SAVE_LR()
      push     {lr}
#endif

      cpsid    i                       //-- Disable core int

      //-- Now, PSP contains task's stack pointer.
      //   We need to get it and save callee-saved registers to stack.
      mrs      r2, PSP

      //-- save FPU callee-saved registers, if needed {{{
#if defined(__TN_ARCHFEAT_CORTEX_M_FPU__)

      //-- if FPU is active, save S16-S31
      tst      lr, #0x10      //-- test bit 4 (Stack Frame type) of EXC_RETURN
      it       eq
      vstmdbeq r2!, {s16-s31}

#endif
      // }}}

      //-- save callee-saved registers {{{
#if defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
      stmdb    r2!, {r4-r11, lr}
#else
      subs     r2, #32     //-- allocate space for r4-r11
      stmia    r2!, {r4-r7}
      mov      r4, r8
      mov      r5, r9
      mov      r6, r10
      mov      r7, r11
      stmia    r2!, {r4-r7}
      subs     r2, #32     //-- increment r2 again
#endif
      // }}}

      //-- get stack pointer of preempted task and save it in the
      //   _tn_curr_run_task->stack_top
      ldr      r5, =_TN_NAME(_tn_curr_run_task)   //-- r5 = &_tn_curr_run_task
      ldr      r6, =_TN_NAME(_tn_next_task_to_run)//-- r6 = &_tn_next_task_to_run
      ldr      r0, [r5]                //-- r0 =  _tn_curr_run_task
      ldr      r1, [r6]                //-- r1 =  _tn_next_task_to_run
      str      r2, [r0]                //-- _tn_curr_run_task->stack_top = SP


#if _TN_ON_CONTEXT_SWITCH_HANDLER
      /*
       * arguments are set:
       * - r0: _tn_curr_run_task
       * - r1: _tn_next_task_to_run
       */
      bl       _TN_NAME(_tn_sys_on_context_switch)
#endif

      //-- _tn_curr_run_task = _tn_next_task_to_run
      ldr      r4, [r6]                //-- r4 =  _tn_next_task_to_run
      str      r4, [r5]                //-- _tn_curr_run_task = _tn_next_task_to_run

      //-- r4 is now _tn_curr_run_task

_TN_LOCAL_LABEL(__context_restore)  //-- if you branch here, r4 should be _tn_curr_run_task

      //-- load stack pointer of newly activated task to r0
      ldr      r0, [r4]       //-- r0 = _tn_curr_run_task->stack_top

      //-- restore callee-saved registers {{{
#if defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
      ldmia    r0!, {r4-r11, lr}  //-- load callee-saved registers, plus lr
#else
      adds     r0, #16
      ldmia    r0!, {r4-r7}
      mov      r8, r4
      mov      r9, r5
      mov      r10, r6
      mov      r11, r7
      subs     r0, #32
      ldmia    r0!, {r4-r7}
      adds     r0, #16
#endif
      // }}}

      //-- restore FPU callee-saved registers, if needed {{{
#if defined(__TN_ARCHFEAT_CORTEX_M_FPU__)

      //if FPU was active in the newly activated task, restore S16-S31
      tst      lr, #0x10      //-- test bit 4 (Stack Frame type) of EXC_RETURN
      it       eq
      vldmiaeq r0!, {s16-s31}

#endif
      // }}}

      msr      PSP, r0        //-- update PSP to stack of newly activated task

      cpsie    i              //-- enable core int

      //-- Restore LR if needed (see comment for macro _TN_NEED_SAVE_LR())
#if _TN_NEED_SAVE_LR()
      pop      {r0}
      mov      lr, r0
#endif

      bx       lr


_TN_THUMB_FUNC()
_TN_LABEL(_tn_arch_sys_start)
      //-- arguments:
      //     r0:  int_stack
      //     r1:  int_stack_size   (in TN_UWord)

      //-- we need to initialize MSP, but before we can do that, we should
      //   switch to PSP, so that MSP will stay untouched after we
      //   initialize it.

      //-- set PSP = MSP
      mrs      r2, MSP
      msr      PSP, r2

      //-- modify CONTROL register so that PSP becomes active

      mrs      r2, CONTROL    //-- read current CONTROL
      movs     r3, #0x02      //-- used intermediary register r3 in order to
                              //   work on M0 too
      orrs     r2, r2, r3
      msr      CONTROL, r2    //-- write to CONTROL

      //-- after modifying CONTROL, we need for instruction barrier
      isb

      //-- now, MSP isn't used, so we can set it to interrupt stack.

      //-- init MSP to int_stack + int_stack_size (given as arguments)
      movs     r2, #4
      muls     r1, r2, r1     //-- int_stack_size *= 4;
      adds     r0, r0, r1     //-- r0 = (int_stack + int_stack_size)
      msr      MSP, r0        //-- MSP = r0

      //-- set priority of PendSV to minimum value.
      ldr      r1, =PR_12_15_ADDR      //-- Load the System 12-15 Priority Register
      ldr      r0, [r1]
      ldr      r2, =PENDS_VPRIORITY
      orrs     r0, r0, r2              //-- set PRI_14 (PendSV) to 0xFF - minimal
      str      r0, [r1]

      //-- set priority of SVC to minimum value.
      ldr      r1, =PR_08_11_ADDR      //-- Load the System 8-11 Priority Register
      ldr      r0, [r1]
      ldr      r2, =SVC_VPRIORITY
      orrs     r0, r0, r2              //-- set PRI_11 (SVC) to 0xFF - minimal
      str      r0, [r1]

#if defined(__TN_ARCHFEAT_CORTEX_M_FPU__)
      //-- disable lazy stacking (so that if interrupted task uses FPU
      //   at the moment, registers S0-S16 are certainly saved by the hardware)
      ldr      r1, =FPU_FPCCR_ADDR               //-- Load the System 8-11 Priority Register
      ldr      r0, [r1]
      and      r0, r0, #FPU_FPCCR_LSPEN          //-- set PRI_11 (SVC) to 0xFF - minimal
      str      r0, [r1]
#endif




      //-- proceed to _tn_arch_context_switch_now_nosave() ..

_TN_THUMB_FUNC()
_TN_LABEL(_tn_arch_context_switch_now_nosave)

      //-- we should enable core int because we're going to
      //   call SVC. If interrupts are disabled,
      //   a call to SVC causes HardFault exception.
      cpsie    i

      //-- perform SVC
      svc      #0x00


      //-- SVC is used for context_switch_now_nosave.
_TN_THUMB_FUNC()
_TN_LABEL(SVC_Handler)

      //-- there is just one service possible: context_switch_now_nosave,
      //   so, we don't check params.

      //-- Save LR if needed (see comment for macro _TN_NEED_SAVE_LR())
#if _TN_NEED_SAVE_LR()
      push     {lr}
#endif

      cpsid    i              //-- Disable core int

      ldr      r5, =_TN_NAME(_tn_curr_run_task)    //-- r5 = &_tn_curr_run_task
      ldr      r6, =_TN_NAME(_tn_next_task_to_run) //-- r6 = &_tn_next_task_to_run
      ldr      r0, [r5]                            //-- r0 =  _tn_curr_run_task
      ldr      r1, [r6]                            //-- r1 =  _tn_next_task_to_run

#if _TN_ON_CONTEXT_SWITCH_HANDLER
      /*
       * arguments are set:
       * - r0: _tn_curr_run_task
       * - r1: _tn_next_task_to_run
       */
      bl       _TN_NAME(_tn_sys_on_context_switch)
#endif

      ldr      r4, [r6]                   //-- r4 =  _tn_next_task_to_run
      str      r4, [r5]                   //-- _tn_curr_run_task = _tn_next_task_to_run

      //-- r4 is now _tn_curr_run_task

      //-- proceed to context restore (r4 must be set to _tn_curr_run_task)
      b        _TN_LOCAL_NAME(__context_restore)



_TN_THUMB_FUNC()
_TN_LABEL(tn_arch_int_dis)

      cpsid    i
      bx       lr



_TN_THUMB_FUNC()
_TN_LABEL(tn_arch_int_en)

      cpsie    i
      bx       lr


_TN_THUMB_FUNC()
_TN_LABEL(tn_arch_sr_save_int_dis)

      mrs      r0, PRIMASK
      cpsid    i
      bx       lr


_TN_THUMB_FUNC()
_TN_LABEL(tn_arch_sr_restore)

      msr      PRIMASK, r0
      bx       lr


_TN_THUMB_FUNC()
_TN_LABEL(_tn_arch_is_int_disabled)

      mrs      r0, PRIMASK
      bx       lr


_TN_THUMB_FUNC()
_TN_LABEL(_tn_arch_inside_isr)

      mrs      r0, CONTROL

#if defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
      //-- Code for Cortex-M3/M4/M4F
      tst      r0, #0x02   //-- test SPSEL bit (0: MSP, 1: PSP)
      ite      eq
      moveq    r0, #1      //-- SPSEL bit is clear: return true
      movne    r0, #0      //-- SPSEL bit is set: return false
      bx       lr
#else
      //-- Code for Cortex-M0/M0+
      movs     r1, #0x02
      tst      r0, r1      //-- test SPSEL bit (0: MSP, 1: PSP)
      bne      _TN_LOCAL_NAME(__ne)
      movs     r0, #1      //-- SPSEL bit is clear: return true
      bx       lr
_TN_LOCAL_LABEL(__ne)
      movs     r0, #0      //-- SPSEL bit is set: return false
      bx       lr
#endif


_TN_THUMB_FUNC()
_TN_LABEL(_tn_arch_context_switch_pend)

      ldr      r1, =ICSR_ADDR
      ldr      r0, =PENDSVSET
      str      r0, [r1]

      bx       lr


/*
 * Disable kernel scheduler and return previous state.
 * See comments in `tn_arch.h` for details.
 *
 * On Cortex-M architecture, we can't precisely disable PendSV interrupt,
 * but we should disable interrupts based on their priorities. So,
 * here we disable all interrupts with lowest priority (0xff)
 *
 * @return
 *    Scheduler state to be restored later by `#tn_arch_sched_restore()`.
 */
_TN_THUMB_FUNC()
_TN_LABEL(tn_arch_sched_dis_save)

#if defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
      //-- Code for Cortex-M3/M4/M4F

      //-- Actual number of interrupt priority levels is
      //   implementation-dependent, so, we can't assume any exact value of the
      //   lowest priority to be set in BASEPRI. The portable way is to read
      //   actual priority level of PendSV and put it to BASEPRI.

      //-- Get current BASEPRI value (to be returned)
      mrs      r0, BASEPRI

      //-- Load the System 8-11 Priority Register
      ldr      r1, =PR_08_11_ADDR
      ldr      r1, [r1]

      //-- Shift it by 24 bits, i.e. get the MSB
      lsr      r1, r1, #24

      //-- Store that value to BASEPRI, if current value isn't already
      //   equal or lower
      msr      BASEPRI_MAX, r1

      bx       lr

#else
      //-- Code for Cortex-M0/M0+
      //   Cortex-M0/M0+ don't have BASEPRI register, so, we have to disable
      //   all interrupts
      b        _TN_NAME(tn_arch_sr_save_int_dis)
#endif

_TN_THUMB_FUNC()
_TN_LABEL(tn_arch_sched_restore)

#if defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
      //-- Code for Cortex-M3/M4/M4F
      msr      BASEPRI, r0
      bx       lr
#else
      //-- Code for Cortex-M0/M0+
      //   Cortex-M0/M0+ don't have BASEPRI register, so, here we just jump
      //   to tn_arch_sr_restore, which enables all interrupts back
      b        _TN_NAME(tn_arch_sr_restore)
#endif


   _TN_END()

