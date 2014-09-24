/*

  TNKernel real-time kernel

  Copyright © 2004, 2013 Yuri Tiomkin
  PIC32 version modifications copyright © 2013 Anders Montonen
  All rights reserved.

  ver. 2.7

  Assembler: GCC MIPS

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.
*/

#ifndef _TN_PORT_ASM_H_
#define _TN_PORT_ASM_H_


    .extern tn_user_sp
    .extern tn_int_sp
    .extern tn_int_nest_count
    .extern tn_curr_run_task
    .extern tn_next_task_to_run

    .equ IFS0SET, 0xBF881038

/*----------------------------------------------------------------------------
* Interrupt vector dispatch macro
*----------------------------------------------------------------------------*/
    .macro tn_vector_dispatch isr:req vec:req

    .section    .vector_\vec,code
    .set    mips32r2
    .set    nomips16
    .set    noreorder
    .align  2

    .global __vector_dispatch_\vec
    .global isr_wrapper_\isr

    .ent    __vector_dispatch_\vec
__vector_dispatch_\vec:
    j   isr_wrapper_\isr\()_\vec
    rdpgpr  $sp, $sp
    .end    __vector_dispatch_\vec
    .size   __vector_dispatch_\vec, .-__vector_dispatch_\vec

    .endm

/*----------------------------------------------------------------------------
* Interrupt handler wrapper macro for software context saving IPL
*----------------------------------------------------------------------------*/
    .macro tn_soft_isr isr:req vec:req

    .extern \isr

    tn_vector_dispatch \isr \vec

    .set mips32r2
    .set nomips16
    .set noreorder
    .set noat
    .text
    .align  2

    .ent    isr_wrapper_\isr\()_\vec
isr_wrapper_\isr\()_\vec:
    /* Increase interrupt nesting count */
    lui     $k0, %hi(tn_int_nest_count)
    lw      $k1, %lo(tn_int_nest_count)($k0)
    addiu   $k1, $k1, 1
    sw      $k1, %lo(tn_int_nest_count)($k0)
    ori     $k0, $zero, 1
    bne     $k1, $k0, 1f

    /* Swap stack pointers if nesting count is one */
    lui     $k0, %hi(tn_user_sp)
    sw      $sp, %lo(tn_user_sp)($k0)
    lui     $k0, %hi(tn_int_sp)
    lw      $sp, %lo(tn_int_sp)($k0)

1:
    /* Save context on stack */
    addiu   $sp, $sp, -92
    mfc0    $k1, $14                    # c0_epc
    mfc0    $k0, $12, 2                 # c0_srsctl
    sw      $k1, 84($sp)
    sw      $k0, 80($sp)
    mfc0    $k1, $12                    # c0_status
    sw      $k1, 88($sp)

    /* Enable nested interrupts */
    mfc0    $k0, $13                    # c0_cause
    ins     $k1, $zero, 1, 15
    ext     $k0, $k0, 10, 6
    ins     $k1, $k0, 10, 6
    mtc0    $k1, $12                    # c0_status

    /* Save caller-save registers on stack */
    sw      $ra, 76($sp)
    sw      $t9, 72($sp)
    sw      $t8, 68($sp)
    sw      $t7, 64($sp)
    sw      $t6, 60($sp)
    sw      $t5, 56($sp)
    sw      $t4, 52($sp)
    sw      $t3, 48($sp)
    sw      $t2, 44($sp)
    sw      $t1, 40($sp)
    sw      $t0, 36($sp)
    sw      $a3, 32($sp)
    sw      $a2, 28($sp)
    sw      $a1, 24($sp)
    sw      $a0, 20($sp)
    sw      $v1, 16($sp)
    sw      $v0, 12($sp)
    sw      $at, 8($sp)
    mfhi    $v0
    mflo    $v1
    sw      $v0, 4($sp)

    /* Call ISR */
    la      $t0, \isr
    jalr    $t0
    sw      $v1, 0($sp)

    /* Pend context switch if needed */
    lw      $t0, tn_curr_run_task
    lw      $t1, tn_next_task_to_run
    lw      $t0, 0($t0)
    lw      $t1, 0($t1)
    lui     $t2, %hi(IFS0SET)
    beq     $t0, $t1, 1f
    ori     $t1, $zero, 2
    sw      $t1, %lo(IFS0SET)($t2)

1:
    /* Restore registers */
    lw      $v1, 0($sp)
    lw      $v0, 4($sp)
    mtlo    $v1
    mthi    $v0
    lw      $at, 8($sp)
    lw      $v0, 12($sp)
    lw      $v1, 16($sp)
    lw      $a0, 20($sp)
    lw      $a1, 24($sp)
    lw      $a2, 28($sp)
    lw      $a3, 32($sp)
    lw      $t0, 36($sp)
    lw      $t1, 40($sp)
    lw      $t2, 44($sp)
    lw      $t3, 48($sp)
    lw      $t4, 52($sp)
    lw      $t5, 56($sp)
    lw      $t6, 60($sp)
    lw      $t7, 64($sp)
    lw      $t8, 68($sp)
    lw      $t9, 72($sp)
    lw      $ra, 76($sp)

    di
    ehb

    /* Restore context */
    lw      $k0, 84($sp)
    mtc0    $k0, $14                # c0_epc
    lw      $k0, 80($sp)
    mtc0    $k0, $12, 2             # c0_srsctl
    addiu   $sp, $sp, 92

    /* Decrease interrupt nesting count */
    lui     $k0, %hi(tn_int_nest_count)
    lw      $k1, %lo(tn_int_nest_count)($k0)
    addiu   $k1, $k1, -1
    sw      $k1, %lo(tn_int_nest_count)($k0)
    bne     $k1, $zero, 1f
    lw      $k1, -4($sp)

    /* Swap stack pointers if nesting count is zero */
    lui     $k0, %hi(tn_int_sp)
    sw      $sp, %lo(tn_int_sp)($k0)
    lui     $k0, %hi(tn_user_sp)
    lw      $sp, %lo(tn_user_sp)($k0)

1:
    wrpgpr  $sp, $sp
    mtc0    $k1, $12                # c0_status
    eret

    .end    isr_wrapper_\isr\()_\vec
    .size   isr_wrapper_\isr\()_\vec, .-isr_wrapper_\isr\()_\vec

    .endm

/*----------------------------------------------------------------------------
* Interrupt handler wrapper macro for shadow register context saving IPL
*----------------------------------------------------------------------------*/
    .macro tn_srs_isr isr:req vec:req

    .extern \isr

    tn_vector_dispatch \isr \vec

    .set mips32r2
    .set nomips16
    .set noreorder
    .set noat
    .text
    .align  2

    .ent    isr_wrapper_\isr\()_\vec
isr_wrapper_\isr\()_\vec:
    /* Increase interrupt nesting count */
    lui     $k0, %hi(tn_int_nest_count)
    lw      $k1, %lo(tn_int_nest_count)($k0)
    addiu   $k1, $k1, 1
    sw      $k1, %lo(tn_int_nest_count)($k0)
    ori     $k0, $zero, 1
    bne     $k1, $k0, 1f

    /* Swap stack pointers if nesting count is one */
    lui     $k0, %hi(tn_user_sp)
    sw      $sp, %lo(tn_user_sp)($k0)
    lui     $k0, %hi(tn_int_sp)
    lw      $sp, %lo(tn_int_sp)($k0)

1:
    /* Save context on stack */
    addiu   $sp, $sp, -20
    mfc0    $k1, $14                    # c0_epc
    mfc0    $k0, $12, 2                 # c0_srsctl
    sw      $k1, 12($sp)
    sw      $k0, 8($sp)
    mfc0    $k1, $12                    # c0_status
    sw      $k1, 16($sp)

    /* Enable nested interrupts */
    mfc0    $k0, $13                    # c0_cause
    ins     $k1, $zero, 1, 15
    ext     $k0, $k0, 10, 6
    ins     $k1, $k0, 10, 6
    mtc0    $k1, $12                    # c0_status

    /* Save caller-save registers on stack */
    mfhi    $v0
    mflo    $v1
    sw      $v0, 4($sp)

    /* Call ISR */
    la      $t0, \isr
    jalr    $t0
    sw      $v1, 0($sp)

    /* Pend context switch if needed */
    lw      $t0, tn_curr_run_task
    lw      $t1, tn_next_task_to_run
    lw      $t0, 0($t0)
    lw      $t1, 0($t1)
    lui     $t2, %hi(IFS0SET)
    beq     $t0, $t1, 1f
    ori     $t1, $zero, 2
    sw      $t1, %lo(IFS0SET)($t2)

1:
    /* Restore registers */
    lw      $v1, 0($sp)
    lw      $v0, 4($sp)
    mtlo    $v1
    mthi    $v0

    di
    ehb

    /* Restore context */
    lw      $k0, 12($sp)
    mtc0    $k0, $14                # c0_epc
    lw      $k0, 8($sp)
    mtc0    $k0, $12, 2             # c0_srsctl
    addiu   $sp, $sp, 20

    /* Decrease interrupt nesting count */
    lui     $k0, %hi(tn_int_nest_count)
    lw      $k1, %lo(tn_int_nest_count)($k0)
    addiu   $k1, $k1, -1
    sw      $k1, %lo(tn_int_nest_count)($k0)
    bne     $k1, $zero, 1f
    lw      $k1, -4($sp)

    /* Swap stack pointers if nesting count is zero */
    lui     $k0, %hi(tn_int_sp)
    sw      $sp, %lo(tn_int_sp)($k0)
    lui     $k0, %hi(tn_user_sp)
    lw      $sp, %lo(tn_user_sp)($k0)

1:
    wrpgpr  $sp, $sp
    mtc0    $k1, $12                # c0_status
    eret

    .end    isr_wrapper_\isr\()_\vec
    .size   isr_wrapper_\isr\()_\vec, .-isr_wrapper_\isr\()_\vec

    .endm

#endif
