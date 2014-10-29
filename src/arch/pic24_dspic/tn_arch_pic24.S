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

    .text

    /* External references */

    .extern  tn_curr_run_task
    .extern  tn_next_task_to_run
    .extern  IFS0
    .extern  _tn_p24_dspic_inside_isr

    /* Public functions declared in this file */

    .global  __tn_arch_context_switch_now_nosave
    .global  __tn_arch_context_switch_pend
    .global  __tn_arch_is_int_disabled
    .global  __tn_arch_inside_isr
    .global  _tn_arch_sr_save_int_dis
    .global  _tn_arch_sr_restore
    .global  __INT0Interrupt

/*----------------------------------------------------------------------------
* Interrupts should be disabled here
*----------------------------------------------------------------------------*/

__tn_arch_context_switch_now_nosave:

    /* get new task's sp */

   /* set SPLIM to max: effectively, turn stack pointer limit off */

   mov    #0xffff, W0
   mov    W0,   _SPLIM

   /* ---- */

   mov     _tn_next_task_to_run,   W1    ; W1 = tn_next_task_to_run
   mov     #_tn_curr_run_task,     W0    ; W0 = &tn_curr_run_task

   mov     W1,             [W0]          ; tn_curr_run_task = tn_next_task_to_run

   mov     [W1],           W15           ; SP = *tn_next_task_to_run

   bra     __tn_sw_restore


/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/

__tn_arch_context_switch_pend:

   push W0

   mov #_IFS0, W0
   bset  [W0], #0  ; IFS0bits.INT0IF = 1

   pop W0

   return


/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/

__INT0Interrupt:

        ;; Save current context

        push    _SR
        push    W14
        push.d  W12
        push.d  W10
        push.d  W8
        push.d  W6
        push.d  W4
        push.d  W2
        push.d  W0
        push    _RCOUNT
        push    _TBLPAG
        push    _CORCON
        push    _PSVPAG
        push    _SPLIM

        /* store sp in preemted task's TCB */

        mov     _tn_curr_run_task,      W0             ; move current task stack pointer to W0
        mov     W15,          [W0]

        /* save SR in W2 */
        mov     _SR,   W2

        /* set SPLIM to max: effectively, turn stack pointer limit off */

        mov    #0xffff, W0
        mov    W0,   _SPLIM

        /* disable interrupts */
        mov   _SR,     W0
        xor   #0xE0,   W0
        xor   _SR

        /* tn_curr_run_task = tn_next_task_to_run */

        mov     _tn_next_task_to_run,  W1    ; W1 = tn_next_task_to_run
        mov     #_tn_curr_run_task,     W0    ; W0 = &tn_curr_run_task

        mov     W1,             [W0]          ; tn_curr_run_task = tn_next_task_to_run


        /* Set new stack pointer (from newly activated task) */

        mov     [W1],           W15           ; SP = *tn_next_task_to_run


        /* clear INT0 pending bit */

        mov #_IFS0, W0
        bclr  [W0], #0  ; IFS0bits.INT0IF = 0

        /* restore SR from W2 (and therefore enable interrupts) */

        mov     W2,             _SR

__tn_sw_restore:

        ; Restore context on stack pointer

        pop     _SPLIM
        pop     _PSVPAG
        pop     _CORCON
        pop     _TBLPAG
        pop     _RCOUNT
        pop.d   W0
        pop.d   W2
        pop.d   W4
        pop.d   W6
        pop.d   W8
        pop.d   W10
        pop.d   W12
        pop     W14
        pop     _SR

        retfie

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/

_tn_arch_sr_save_int_dis:

   push    _SR

   ;mov     _SR,   W0
   ;ior     #0xE0, W0
   ;mov     W0,    _SR

   mov   _SR,     W0
   xor   #0xE0,   W0
   xor   _SR

   pop     W0

   return


/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/

_tn_arch_sr_restore:

   push  W0
   push  W1

   mov   W0,      W1
   and   #0xE0,   W1
   mov   _SR,     W0
   xor   W0,      W1,      W0
   and   #0xE0,   W0
   xor   _SR

   pop   W1
   pop   W0

   return

/*----------------------------------------------------------------------------
*
*----------------------------------------------------------------------------*/

__tn_arch_is_int_disabled:
   /* TODO: optimize */

   mov   #0xE0,   W0
   and   _SR,     WREG
   lsr   W0,      #5,      W0
   cp    W0,      #0x7
   bra   Z,       1f
   mov   #0,      W0
   bra            2f
1:
   mov   #1,      W0
2:
   return




__tn_arch_inside_isr:

   //-- here, we check if SPLIM is equal to __tn_p24_int_splim.
   //   If it is, we are inside ISR.

   mov __tn_p24_int_splim, w0
   cp SPLIM
   mov   #0,      W0
   bra nz, 1f
   mov   #1,      W0
1:
   return

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
