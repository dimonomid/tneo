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

extern unsigned long _gp;

//----------------------------------------------------------------------------
//  Context layout
//
//      SP + 7C EPC
//      SP + 78 Status
//      SP + 74 r31/ra
//      SP + 70 r30/s8/fp
//      SP + 6C r28/gp
//      SP + 68 r25/t9
//      SP + 64 r24/t8
//      SP + 60 r23/s7
//      SP + 5C r22/s6
//      SP + 58 r21/s5
//      SP + 54 r20/s4
//      SP + 50 r19/s3
//      SP + 4C r18/s2
//      SP + 48 r17/s1
//      SP + 44 r16/s0
//      SP + 40 r15/t7
//      SP + 3C r14/t6
//      SP + 38 r13/t5
//      SP + 34 r12/t4
//      SP + 30 r11/t3
//      SP + 2C r10/t2
//      SP + 28 r9/t1
//      SP + 24 r8/t0
//      SP + 20 r7/a3
//      SP + 1C r6/a2
//      SP + 18 r5/a1
//      SP + 14 r4/a0
//      SP + 10 r3/v1
//      SP + 0C r2/v0
//      SP + 08 r1/at
//      SP + 04 hi
//      SP + 00 lo
//----------------------------------------------------------------------------

TN_UWord *_tn_arch_stack_start_get(
      TN_UWord *stack_low_address,
      int stack_size
      )
{
   return stack_low_address + stack_size - 1;
}


//----------------------------------------------------------------------------
//   Processor specific routine - here for MIPS4K
//
//   sizeof(void*) = sizeof(int)
//----------------------------------------------------------------------------
unsigned int *_tn_arch_stack_init(
      TN_TaskBody *task_func,
      TN_UWord *stack_start,
      void *param
      )
{
   unsigned int *p_stk;

   //-- filling register's position in the stack - for debugging only

   p_stk  = (unsigned int *)stack_start;     //-- Load stack pointer
   *p_stk-- = 0;                             //-- ABI argument area
   *p_stk-- = 0;
   *p_stk-- = 0;
   *p_stk-- = 0;
   *p_stk-- = (unsigned int)task_func;       //-- EPC
   *p_stk-- = 3;                             //-- Status: EXL and IE bits are set
   *p_stk-- = (unsigned int)tn_task_exit;    //-- ra
   *p_stk-- = 0x30303030L;                   //-- fp
   *p_stk-- = (unsigned int)&_gp;            //-- gp - provided by linker
   *p_stk-- = 0x25252525L;                   //-- t9
   *p_stk-- = 0x24242424L;                   //-- t8
   *p_stk-- = 0x23232323L;                   //-- s7
   *p_stk-- = 0x22222222L;                   //-- s6
   *p_stk-- = 0x21212121L;                   //-- s5
   *p_stk-- = 0x20202020L;                   //-- s4
   *p_stk-- = 0x19191919L;                   //-- s3
   *p_stk-- = 0x18181818L;                   //-- s2
   *p_stk-- = 0x17171717L;                   //-- s1
   *p_stk-- = 0x16161616L;                   //-- s0
   *p_stk-- = 0x15151515L;                   //-- t7
   *p_stk-- = 0x14141414L;                   //-- t6
   *p_stk-- = 0x13131313L;                   //-- t5
   *p_stk-- = 0x12121212L;                   //-- t4
   *p_stk-- = 0x11111111L;                   //-- t3
   *p_stk-- = 0x10101010L;                   //-- t2
   *p_stk-- = 0x09090909L;                   //-- t1
   *p_stk-- = 0x08080808L;                   //-- t0
   *p_stk-- = 0x07070707L;                   //-- a3
   *p_stk-- = 0x06060606L;                   //-- a2
   *p_stk-- = 0x05050505L;                   //-- a1
   *p_stk-- = (unsigned int)param;           //-- a0 - task's function argument
   *p_stk-- = 0x03030303L;                   //-- v1
   *p_stk-- = 0x02020202L;                   //-- v0
   *p_stk-- = 0x01010101L;                   //-- at
   *p_stk-- = 0x33333333L;                   //-- hi
   *p_stk = 0x32323232L;                     //-- lo

   return p_stk;
}

//_____________________________________________________________________________
//
void tn_arch_int_dis(void)
{
    __builtin_disable_interrupts();
}

void tn_arch_int_en(void)
{
   __builtin_enable_interrupts();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
