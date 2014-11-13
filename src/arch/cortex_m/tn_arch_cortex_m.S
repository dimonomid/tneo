 ;TODO: how to preprocess this file?
 ;/* ******************************************************************************
 ;*
 ;* TNeoKernel: real-time kernel initially based on TNKernel
 ;*
 ;*    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 ;*    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 ;*    TNeoKernel:                copyright © 2014       Dmitry Frank.
 ;*
 ;*    TNeoKernel was born as a thorough review and re-implementation of
 ;*    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 ;*    as well as new features being added, and it is tested carefully with
 ;*    unit-tests.
 ;*
 ;*    API is changed somewhat, so it's not 100% compatible with TNKernel,
 ;*    hence the new name: TNeoKernel.
 ;*
 ;*    Permission to use, copy, modify, and distribute this software in source
 ;*    and binary forms and its documentation for any purpose and without fee
 ;*    is hereby granted, provided that the above copyright notice appear
 ;*    in all copies and that both that copyright notice and this permission
 ;*    notice appear in supporting documentation.
 ;*
 ;*    THIS SOFTWARE IS PROVIDED BY THE DMITRY FRANK AND CONTRIBUTORS "AS IS"
 ;*    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 ;*    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 ;*    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DMITRY FRANK OR CONTRIBUTORS BE
 ;*    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 ;*    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 ;*    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 ;*    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 ;*    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ;*    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 ;*    THE POSSIBILITY OF SUCH DAMAGE.
 ;*
 ;******************************************************************************/

   PRESERVE8

   AREA    |.text|, CODE, READONLY, ALIGN=3
   THUMB
   REQUIRE8
   PRESERVE8


   IMPORT tn_curr_run_task
   IMPORT tn_next_task_to_run
   IMPORT _tn_cortex_m_sys_init

   EXPORT ffs_asm
   EXPORT PendSV_Handler
   EXPORT SVC_Handler
   EXPORT _tn_arch_sys_start
   EXPORT _tn_arch_context_switch_now_nosave



ffs_asm

       mov    r1, r0                    ;-- tmp = in
       rsbs   r0, r1, #0                ;-- in = -in
       ands   r0, r0, r1                ;-- in = in & tmp
       CLZ.W  r0, r0
       rsb    r0, r0, #0x20             ;-- 32 - in
       bx     lr



PendSV_Handler

       cpsid  I                         ;-- Disable core int

       mrs    r0, PSP                   ;-- in PSP - process(task) stack pointer
       stmdb  r0!, {r4-r11}

       ldr    r3, =tn_curr_run_task
       mov    r1,  r3
       ldr    r1, [r1]
       str    r0, [r1]                  ;-- save own SP in TCB
	

       ldr    r1, =tn_next_task_to_run
       ldr    r2, [r1]
       str    r2, [r3]                  ;-- in r3 - =tn_curr_run_task


       ldr    r0, [r2]                  ;-- in r0 - new task SP

       ldmia  r0!, {r4-r11}
       msr    PSP, r0
       orr    lr, lr, #0x04             ;-- Force to new process PSP

       cpsie  I                         ;-- enable core int
       bx     lr


_tn_arch_sys_start
       ;-- arguments:
       ;     r0:  int_stack
       ;     r1:  int_stack_size   (in TN_UWord)

       ;-- set PSP = MSP
       mrs    r2, MSP
       msr    PSP, r2

       ;-- modify CONTROL register so that PSP becomes active

       mrs    r2, CONTROL     ;-- read current CONTROL
       orrs   r2, r2, #0x02   ;-- set SPSEL
       msr    CONTROL, r2     ;-- write to CONTROL

       ;-- after modifying CONTROL, we need for instruction barrier
       ;   (architectural recommendation)
       isb

       ;-- now, MSP isn't used, so we can set it to interrupt stack.

       ;-- init MSP to int_stack + int_stack_size (given as arguments)
       mov    r2, #4
       mul    r1, r1, r2                        ;-- int_stack_size *= 4;
       adds   r0, r0, r1                        ;-- r0 = int_stack + int_stack_size
       msr    MSP, r0                           ;-- MSP = r0

       ;-- it's more convenient to take advantage of CMSIS in C, so,
       ;   here we just call C function that performs needed hardware
       ;   initialization.
       bl     _tn_cortex_m_sys_init

       cpsie  I                         ;-- enable core int

       ;-- proceed to _tn_arch_context_switch_now_nosave() ..

_tn_arch_context_switch_now_nosave

       ;-- perform SVC
       svc    #0x00


       ;-- SVC is used for context_switch_now_nosave.
SVC_Handler

       ;-- there is just one service possible: context_switch_now_nosave,
       ;   so, we don't check params here.

       cpsid  I                         ;-- Disable core int
       ldr    r3, =tn_curr_run_task

       ldr    r1, =tn_next_task_to_run
       ldr    r2, [r1]
       str    r2, [r3]                  ;-- in r3 - =tn_curr_run_task

       ldr    r0, [r2]                  ;-- in r0 - new task SP

       ldmia  r0!, {r4-r11}
       msr    PSP, r0
       orr    lr, lr, #0x04             ;-- Force to new process PSP

       cpsie  I                         ;-- enable core int
       bx     lr

       


   END

