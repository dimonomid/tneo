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

#include "tn_cfg_dispatch.h"

/*
 * Maximum system interrupt priority, shifted by 5 bits, as it is 
 * in the status register.
 *
 * Refer to the comments of TN_P24_SYS_IPL for details about what are
 * system interrupts.
 */
#define  _TN_IPL_SHIFTED    (TN_P24_SYS_IPL << 5)


      .text


      /* External references */

      .extern  _tn_curr_run_task
      .extern  _tn_next_task_to_run
      .extern  IFS0
      .extern  __tn_sys_on_context_switch


      /* Public functions declared in this file */

      .global  __INT0Interrupt
      .global  __tn_arch_context_switch_now_nosave
      .global  __tn_arch_context_switch_pend
      .global  __tn_arch_is_int_disabled
      .global  __tn_arch_inside_isr
      .global  __tn_p24_ffs_asm
      .global  _tn_arch_sr_save_int_dis
      .global  _tn_arch_sr_restore
      .global  _tn_arch_int_dis
      .global  _tn_arch_int_en



/*
 * See comments for _tn_arch_context_switch_now_nosave() in tn_arch.h
 */
__tn_arch_context_switch_now_nosave:

      /* NOTE: interrupts should be disabled here */


      /* call on-context-switch handler, if needed */
#if _TN_ON_CONTEXT_SWITCH_HANDLER
      mov      __tn_curr_run_task, w0         ; w0 = _tn_curr_run_task
      mov      __tn_next_task_to_run,   w1    ; w1 = _tn_next_task_to_run

      /* 
       * arguments are set:
       * - w0: _tn_curr_run_task
       * - w1: _tn_next_task_to_run
       */
      call     __tn_sys_on_context_switch
      nop
#endif

      /* set SPLIM to max: effectively, turn stack pointer limit off */
      mov      #0xffff, w0
      mov      w0, _SPLIM


      /* set pointer to new task and its stack */
      mov      __tn_next_task_to_run, w1   ; w1 = _tn_next_task_to_run
      mov      #__tn_curr_run_task, w0     ; w0 = &_tn_curr_run_task

      mov      w1, [w0]                ; _tn_curr_run_task = _tn_next_task_to_run
      mov      [w1], w15               ; SP = *_tn_next_task_to_run

      bra      __tn_context_restore



/*
 * See comments for _tn_arch_context_switch_pend() in tn_arch.h
 */
__tn_arch_context_switch_pend:

      mov      #_IFS0, w0
      bset     [w0], #0    ; IFS0bits.INT0IF = 1

      return



/*
 * ISR for INT0, which belongs to the kernel.
 * Performs context switch to the task pointed to by `_tn_next_task_to_run`.
 */
__INT0Interrupt:

      ;; Save current context

      push     _SR
      push     w14
      push.d   w12
      push.d   w10
      push.d   w8
      push.d   w6
      push.d   w4
      push.d   w2
      push.d   w0
      push     _RCOUNT
      push     _TBLPAG
      push     _CORCON
   
.ifdef __HAS_EDS
      push     _DSRPAG
      push     _DSWPAG
.else
      push     _PSVPAG
.endif

      push     _SPLIM

      /* store sp in preemted task's TCB */

      mov      __tn_curr_run_task, w0  ; w0 = _tn_curr_run_task;
      mov      w15, [w0]               ; _tn_curr_run_task->stack_top = SR;

      /* save SR in w10 */
      mov      _SR, w10

      /* disable interrupts */
      mov      _SR,              w0
      xor      #_TN_IPL_SHIFTED, w0
      and      #0xE0,            w0
      xor      _SR,            /*w0*/

      /* call on-context-switch handler, if needed */
#if _TN_ON_CONTEXT_SWITCH_HANDLER
      mov      __tn_curr_run_task,      w0    ; w0 = _tn_curr_run_task
      mov      __tn_next_task_to_run,   w1    ; w1 = _tn_next_task_to_run

      /* 
       * arguments are set:
       * - w0: _tn_curr_run_task
       * - w1: _tn_next_task_to_run
       */
      call     __tn_sys_on_context_switch
      nop
#endif

      /* set SPLIM to max: effectively, turn stack pointer limit off */

      mov      #0xffff, w2
      mov      w2,      _SPLIM

      /* _tn_curr_run_task = _tn_next_task_to_run */

      mov      __tn_next_task_to_run, w9    ; w9 = _tn_next_task_to_run
      mov      #__tn_curr_run_task,   w2    ; w2 = &_tn_curr_run_task

      mov      w9, [w2]          ; _tn_curr_run_task = _tn_next_task_to_run


      /* Set new stack pointer (from newly activated task) */

      mov      [w9], w15         ; SP = *_tn_next_task_to_run


      /* clear INT0 pending bit */

      mov      #_IFS0, w2
      bclr     [w2],   #0        ; IFS0bits.INT0IF = 0

      /* restore SR from w10 (and therefore enable interrupts) */

      mov      w10, _SR

__tn_context_restore:

      ; Restore context on stack pointer

      pop      _SPLIM

.ifdef __HAS_EDS
      pop      _DSWPAG
      pop      _DSRPAG
.else
      pop      _PSVPAG
.endif

      pop      _CORCON
      pop      _TBLPAG
      pop      _RCOUNT
      pop.d    w0
      pop.d    w2
      pop.d    w4
      pop.d    w6
      pop.d    w8
      pop.d    w10
      pop.d    w12
      pop      w14
      pop      _SR

      retfie


/*
 * See comments for tn_arch_sr_save_int_dis() in tn_arch.h
 */
_tn_arch_sr_save_int_dis:

      mov      _SR, w1

      mov      _SR,              w0
      xor      #_TN_IPL_SHIFTED, w0
      and      #0xE0,            w0
      xor      _SR             /*w0*/

      mov      w1, w0

      return


/*
 * See comments for tn_arch_sr_restore() in tn_arch.h
 */
_tn_arch_sr_restore:

      ; w0  - saved SR

      mov      w0, w1
      and      #0xE0, w1
      mov      _SR, w0
      xor      w0, w1, w0
      and      #0xE0, w0
      xor      _SR

      return


/*
 * See comments for tn_arch_int_dis() in tn_arch.h
 */
_tn_arch_int_dis:

      goto     _tn_arch_sr_save_int_dis


/*
 * See comments for tn_arch_int_en() in tn_arch.h
 */
_tn_arch_int_en:

      mov      #0xff1f, w0
      and      _SR, WREG/*w0*/

      return

/*
 * See comments for _tn_arch_is_int_disabled() in tn_arch.h
 */
__tn_arch_is_int_disabled:

      mov      #0xE0, w0
      and      _SR, WREG/*w0*/
      lsr      w0, #5, w0
      cp       w0, #TN_P24_SYS_IPL
      bra      geu, 1f
      mov      #0, w0
      bra      2f
1:
      mov      #1, w0
2:
      return




/*
 * See comments for _tn_arch_inside_isr() in tn_arch.h
 */
__tn_arch_inside_isr:

      /*
       * Here, we check if SPLIM is equal to __tn_p24_int_splim.  If it is, we
       * are inside ISR.
       *
       * NOTE: we can't check IPL from SR, because when someone disables
       * interrupts by calling tn_arch_sr_save_int_dis(), IPL is modified. So,
       * if we disable interrupts from task, then _tn_arch_inside_isr() would
       * return true, but it is actually should be false.
       *
       * Instead, on PIC24/dsPIC, system interrupts **must** use
       * kernel-provided macro tn_p32_soft_isr() for interrupt definition, and
       * we modify SPLIM there. So, here we check SPLIM, and if it points to
       * _tn_p24_int_splim, we are inside interrupt.
       */

      mov      __tn_p24_int_splim, w0
      cp       SPLIM
      mov      #0, w0
      bra      nz, 1f
      mov      #1, w0
1:
      return


/**
 * find-first-set bit
 */
__tn_p24_ffs_asm:

      ff1r     w0, w0
      return


