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

#include "_tn_tasks.h"
#include "tn_arch_pic24_bfa.h"
#include <xc.h>


/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

//-- pointer to interrupt stack
TN_UWord *_tn_p24_int_stack_low_addr   = TN_NULL;

//-- pointer to interrupt stack limit (the value to set to SPLIM register)
TN_UWord *_tn_p24_int_splim            = TN_NULL;



/*******************************************************************************
 *    ARCHITECTURE-DEPENDENT IMPLEMENTATION
 ******************************************************************************/

/*
 * See comments in the file `tn_arch.h`
 */
void _tn_arch_sys_start(
      TN_UWord      *int_stack,
      TN_UWord       int_stack_size
      )
{
   //-- set interrupt stack pointers.
   //   We need to subtract additional 1 from SPLIM because
   //   PIC24/dsPIC hardware works this way.
   _tn_p24_int_stack_low_addr = int_stack;
   _tn_p24_int_splim = int_stack + int_stack_size - 1 - 1;


   //-- set up software interrupt for context switching
   TN_BFA(TN_BFA_WR, IPC0, INT0IP, 1); //-- set lowest interrupt priority
                                       //   for context switch interrupt
   TN_BFA(TN_BFA_WR, IFS0, INT0IF, 0); //-- clear interrupt flag
   TN_BFA(TN_BFA_WR, IEC0, INT0IE, 1); //-- enable interrupt

   //-- perform first context switch
   _tn_arch_context_switch_now_nosave();
}



/*
 * See comments in the file `tn_arch.h`
 */
TN_UWord *_tn_arch_stack_init(
      TN_TaskBody   *task_func,
      TN_UWord      *stack_low_addr,
      TN_UWord      *stack_high_addr,
      void          *param
      )
{
   TN_UWord *cur_stack_pt = stack_low_addr/*'empty asc stack' model*/;

   //-- calculate stack pointer limit: we need to subtract additional 1 from
   //   it because PIC24/dsPIC hardware works this way.
   TN_UWord *p_splim = stack_high_addr - 1;

   //-- set return address that is used when task body function returns:
   //   we set it to _tn_task_exit_nodelete, so that returning from task
   //   body function is equivalent to calling tn_task_exit(0)
   *(cur_stack_pt++) = (TN_UWord)_tn_task_exit_nodelete;
   *(cur_stack_pt++) = 0;

   //-- set return address that is used when context is switched to the task
   //   for the first time. We set it to the task body function.
   *(cur_stack_pt++) = (TN_UWord)task_func;
   *(cur_stack_pt++) = 0;

   //-- save usual context for PIC24/dsPIC.
   *(cur_stack_pt++) = 0x0103;               // SR
   *(cur_stack_pt++) = 0x1414;               // W14
   *(cur_stack_pt++) = 0x1212;               // W12
   *(cur_stack_pt++) = 0x1313;               // W13
   *(cur_stack_pt++) = 0x1010;               // W10
   *(cur_stack_pt++) = 0x1111;               // W11
   *(cur_stack_pt++) = 0x0808;               // W08
   *(cur_stack_pt++) = 0x0909;               // W09
   *(cur_stack_pt++) = 0x0606;               // W06
   *(cur_stack_pt++) = 0x0707;               // W07
   *(cur_stack_pt++) = 0x0404;               // W04
   *(cur_stack_pt++) = 0x0505;               // W05
   *(cur_stack_pt++) = 0x0202;               // W02
   *(cur_stack_pt++) = 0x0303;               // W03
   *(cur_stack_pt++) = (TN_UWord)param;      // W00 - task func param
   *(cur_stack_pt++) = 0x0101;               // W01
   *(cur_stack_pt++) = 0;                    // RCOUNT
   *(cur_stack_pt++) = 0;                    // TBLPAG
   *(cur_stack_pt++) = 0x04;                 // CORCON: the PSV bit is set.
#ifdef __HAS_EDS__
   *(cur_stack_pt++) = 0;                    // DSRPAG
   *(cur_stack_pt++) = 0;                    // DSWPAG
#else
   *(cur_stack_pt++) = 0;                    // PSVPAG
#endif

   *(cur_stack_pt++) = (TN_UWord)p_splim;    // SPLIM

   return cur_stack_pt;
}


/*
 * See comments in the file `tn_arch.h`
 */
TN_UWord tn_arch_sched_dis_save(void)
{
   TN_UWord ret = TN_BFA(TN_BFA_RD, IEC0, INT0IE);
   TN_BFA(TN_BFA_WR, IEC0, INT0IE, 0);
   return ret;
}



/*
 * See comments in the file `tn_arch.h`
 */
void tn_arch_sched_restore(TN_UWord sched_state)
{
   TN_BFA(TN_BFA_WR, IEC0, INT0IE, !!sched_state);
}




