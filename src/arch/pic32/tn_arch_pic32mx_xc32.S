/*******************************************************************************
 *
 * TNeoKernel: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeoKernel:                copyright © 2014       Dmitry Frank.
 *
 *    TNeoKernel was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeoKernel.
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

    .set mips32r2
    .set nomips16
    .text
    .align 4

    /* External references */

    .extern  tn_curr_run_task, 4
    .extern  tn_next_task_to_run, 4
    .extern  tn_sys_state, 4
    .extern  _tn_sys_on_context_switch, 4

    /* Public functions declared in this file */

    .global  _tn_arch_context_switch_now_nosave
    .global  _tn_arch_context_switch_pend
    .global  _tn_arch_inside_isr
    .global  tn_arch_sr_save_int_dis
    .global  tn_arch_sr_restore
    .global  cs0_int_handler

    /* Constants */

    .equ IFS0CLR, 0xBF881034
    .equ IFS0SET, 0xBF881038
    .equ context_size, 128
    .equ buildmode, not_simulator           /* workaround for buggy simulator */

/*----------------------------------------------------------------------------
* Interrupts should be disabled here
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent _tn_arch_context_switch_now_nosave

_tn_arch_context_switch_now_nosave:

    /* get new task's sp */

    la      $s0, tn_curr_run_task       /* s0 = &tn_curr_run_task */
    lw      $a0, 0($s0)                 /* a0 =  tn_curr_run_task */
    lw      $a1, tn_next_task_to_run    /* a1 =  tn_next_task_to_run */

#if _TN_ON_CONTEXT_SWITCH_HANDLER
    /* NOTE: next instruction is executed BEFORE call, so,
       SP is updated to tn_next_task_to_run->stack_top before call */
    jal     _tn_sys_on_context_switch
#endif

    lw      $sp, 0($a1)                 /* sp = *tn_next_task_to_run */
    j       tn_restore_context
    sw      $a1, 0($s0)                 /* tn_curr_run_task = tn_next_task_to_run */

    .end _tn_arch_context_switch_now_nosave

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent _tn_arch_context_switch_pend

_tn_arch_context_switch_pend:

    /* pend CS0 interrupt */
    lui     $t0, %hi(IFS0SET)
    ori     $t1, $zero, 2
    sw      $t1, %lo(IFS0SET)($t0)
    nop

    jr      $ra
    nop

    .end _tn_arch_context_switch_pend

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent cs0_int_handler

cs0_int_handler:

    mfc0    $k1, $14                    /* c0_epc */
    addiu   $sp, $sp, -context_size     /* allocate context on stack */
    sw      $k1, 124($sp)               /* save return address */
    mfc0    $k1, $12                    /* c0_status */
    mfc0    $k0, $13                    /* c0_cause */

    sw      $k1, 120($sp)

    /* enable nested interrupts */
    ins     $k1, $zero, 1, 15           /* clear IPL, UM, ERL, EXL */
    ext     $k0, $k0, 10, 6             /* extract RIPL */
    ins     $k1, $k0, 10, 6             /* set current IPL */
    mtc0    $k1, $12                    /* c0_status */

    sw      $31, 116($sp)               /* save current task registers */
    sw      $30, 112($sp)
    sw      $28, 108($sp)
    sw      $25, 104($sp)
    sw      $24, 100($sp)
    sw      $23, 96($sp)
    sw      $22, 92($sp)
    sw      $21, 88($sp)
    sw      $20, 84($sp)
    sw      $19, 80($sp)
    sw      $18, 76($sp)
    sw      $17, 72($sp)
    sw      $16, 68($sp)
    sw      $15, 64($sp)
    sw      $14, 60($sp)
    sw      $13, 56($sp)
    sw      $12, 52($sp)
    sw      $11, 48($sp)
    sw      $10, 44($sp)
    sw      $9, 40($sp)
    sw      $8, 36($sp)
    sw      $7, 32($sp)
    sw      $6, 28($sp)
    sw      $5, 24($sp)
    sw      $4, 20($sp)
    sw      $3, 16($sp)
    sw      $2, 12($sp)
    sw      $1, 8($sp)
    mfhi    $t0
    mflo    $t1
    sw      $t0, 4($sp)
    sw      $t1, 0($sp)

    la      $s0, tn_curr_run_task       /* s0 = &tn_curr_run_task */
    lw      $a0, 0($s0)                 /* a0 =  tn_curr_run_task */
    sw      $sp, 0($a0)                 /* store sp in preempted task's TCB */



    di      $t2                         /* disable interrupts */
    ehb

    la      $a1, tn_next_task_to_run    /* a1 = &tn_next_task_to_run */
    lw      $a1, 0($a1)                 /* a1 =  tn_next_task_to_run */

#if _TN_ON_CONTEXT_SWITCH_HANDLER
    /* NOTE: next instruction is executed BEFORE call, so,
       SP is updated to tn_next_task_to_run->stack_top before call */
    jal     _tn_sys_on_context_switch
#endif

    lw      $sp, 0($a1)                 /* sp = *tn_next_task_to_run */
    sw      $a1, 0($s0)                 /* tn_curr_run_task = tn_next_task_to_run */

    lui     $k0, %hi(IFS0CLR)           /* clear cs0 pending bit */
    ori     $k1, $zero, 2
    sw      $k1, %lo(IFS0CLR)($k0)

    mtc0    $t2, $12                    /* re-enable interrupts */

tn_restore_context:

    lw      $t0, 0($sp)
    lw      $t1, 4($sp)
    mtlo    $t0
    mthi    $t1

    lw      $1, 8($sp)
    lw      $2, 12($sp)
    lw      $3, 16($sp)
    lw      $4, 20($sp)
    lw      $5, 24($sp)
    lw      $6, 28($sp)
    lw      $7, 32($sp)
    lw      $8, 36($sp)
    lw      $9, 40($sp)
    lw      $10, 44($sp)
    lw      $11, 48($sp)
    lw      $12, 52($sp)
    lw      $13, 56($sp)
    lw      $14, 60($sp)
    lw      $15, 64($sp)
    lw      $16, 68($sp)
    lw      $17, 72($sp)
    lw      $18, 76($sp)
    lw      $19, 80($sp)
    lw      $20, 84($sp)
    lw      $21, 88($sp)
    lw      $22, 92($sp)
    lw      $23, 96($sp)
    lw      $24, 100($sp)
    lw      $25, 104($sp)
    lw      $28, 108($sp)
    lw      $30, 112($sp)
    lw      $31, 116($sp)

    di      $zero
    ehb

    lw      $k1, 124($sp)
    lw      $k0, 120($sp)
    mtc0    $k1, $14                    /* c0_epc */
    addiu   $sp, $sp, context_size      /* pop stack frame */
    mtc0    $k0, $12                    /* c0_status */
    eret

    .end cs0_int_handler

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_arch_sr_save_int_dis

tn_arch_sr_save_int_dis:

    di      $v0                         /* Disable interrupts */
.if buildmode != simulator
    jr.hb   $ra                         /* Return with hazard barrier */
    nop
.else
    jr   $ra                            /* Return with hazard barrier */
    ehb
.endif

    .end tn_arch_sr_save_int_dis

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent tn_arch_sr_restore

tn_arch_sr_restore:

    mtc0    $a0, $12                    /* c0_status */
.if buildmode != simulator
    jr.hb   $ra                         /* Return with hazard barrier */
    nop
.else
    jr   $ra                            /* Return with hazard barrier */
    ehb
.endif

    .end tn_arch_sr_restore

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/
    .set noreorder
    .set noat
    .ent _tn_arch_inside_isr

_tn_arch_inside_isr:

    mfc0    $v0, $12                    /* c0_status */
    ins     $v0, $zero, 2, 8            /* zero everything between EXL and IPL */
    ext     $v0, $v0, 1, 15             /* extract EXL...IPL */
    sltiu   $v0, $v0, 1                 /* resulting value < 1 ? */
    jr      $ra
    xori    $v0, $v0, 1                 /* flip result */

    .end _tn_arch_inside_isr

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
