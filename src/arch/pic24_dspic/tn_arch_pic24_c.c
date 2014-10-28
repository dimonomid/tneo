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

#include "tn_tasks.h"

volatile unsigned int _tn_p24_dspic_inside_isr = 0;



TN_UWord *_tn_arch_stack_top_get(
      TN_UWord *stack_low_address,
      int stack_size
      )
{
   return stack_low_address;
}

TN_UWord *_tn_arch_stack_init(
      TN_TaskBody   *task_func,
      TN_UWord      *stack_top,
      int            stack_size,
      void          *param
      )
{
   TN_UWord *p_splim = stack_top + stack_size - 1 - 1;

   *(stack_top++) = (TN_UWord)task_func;
   *(stack_top++) = 0;
   *(stack_top++) = 0x0103;   // SR
   *(stack_top++) = 0x1414;   // W14
   *(stack_top++) = 0x1212;   // W12
   *(stack_top++) = 0x1313;   // W13
   *(stack_top++) = 0x1010;   // W10
   *(stack_top++) = 0x1111;   // W11
   *(stack_top++) = 0x0808;   // W08
   *(stack_top++) = 0x0909;   // W09
   *(stack_top++) = 0x0606;   // W06
   *(stack_top++) = 0x0707;   // W07
   *(stack_top++) = 0x0404;   // W04
   *(stack_top++) = 0x0505;   // W05
   *(stack_top++) = 0x0202;   // W02
   *(stack_top++) = 0x0303;   // W03
   *(stack_top++) = (TN_UWord)param;   // W00 - task func param
   *(stack_top++) = 0x0101;   // W01
   *(stack_top++) = 0;   // RCOUNT
   *(stack_top++) = 0;   // TBLPAG
   *(stack_top++) = 0x04;   // CORCON TODO: take from real CORCON value
   *(stack_top++) = 0;   // PSVPAG
   *(stack_top++) = (TN_UWord)p_splim;   // SPLIM

   return stack_top;
}

void tn_arch_int_dis(void)
{
    //__builtin_disable_interrupts();
    //TODO
   tn_arch_sr_save_int_dis();
}

void tn_arch_int_en(void)
{
   //__builtin_enable_interrupts();
   //TODO
}

int _tn_arch_inside_isr(void)
{
   return _tn_p24_dspic_inside_isr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
