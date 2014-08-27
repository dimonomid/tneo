/*

  TNKernel real-time kernel

  Copyright © 2004, 2013 Yuri Tiomkin
  PIC32 version modifications copyright © 2013 Anders Montonen
  All rights reserved.

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

  /* ver 2.7 */

#include "tn.h"

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


//----------------------------------------------------------------------------
//   Processor specific routine - here for MIPS4K
//
//   sizeof(void*) = sizeof(int)
//----------------------------------------------------------------------------
unsigned int * tn_stack_init(void * task_func,
                             void * stack_start,
                             void * param)
{
    unsigned int * stk;

 //-- filling register's position in the stack - for debugging only

    stk  = (unsigned int *)stack_start;     //-- Load stack pointer
    *stk-- = 0;                             //-- ABI argument area
    *stk-- = 0;
    *stk-- = 0;
    *stk-- = 0;
    *stk-- = (unsigned int)task_func;       //-- EPC
 //-- CU0 = 0
 //-- RP = 0
 //-- RE = 0
 //-- BEV = 0
 //-- SR = 0
 //-- NMI = 0
 //-- IPL<2:0> = 0
 //-- UM = 0
 //-- ERL = 0
 //-- EXL = 0
 //-- IE = 1
    *stk-- = 1;                             //-- Status
    *stk-- = (unsigned int)tn_task_exit;    //-- ra
    *stk-- = 0x30303030L;                   //-- fp
    *stk-- = (unsigned int)&_gp;            //-- gp - provided by linker
    *stk-- = 0x25252525L;                   //-- t9
    *stk-- = 0x24242424L;                   //-- t8
    *stk-- = 0x23232323L;                   //-- s7
    *stk-- = 0x22222222L;                   //-- s6
    *stk-- = 0x21212121L;                   //-- s5
    *stk-- = 0x20202020L;                   //-- s4
    *stk-- = 0x19191919L;                   //-- s3
    *stk-- = 0x18181818L;                   //-- s2
    *stk-- = 0x17171717L;                   //-- s1
    *stk-- = 0x16161616L;                   //-- s0
    *stk-- = 0x15151515L;                   //-- t7
    *stk-- = 0x14141414L;                   //-- t6
    *stk-- = 0x13131313L;                   //-- t5
    *stk-- = 0x12121212L;                   //-- t4
    *stk-- = 0x11111111L;                   //-- t3
    *stk-- = 0x10101010L;                   //-- t2
    *stk-- = 0x09090909L;                   //-- t1
    *stk-- = 0x08080808L;                   //-- t0
    *stk-- = 0x07070707L;                   //-- a3
    *stk-- = 0x06060606L;                   //-- a2
    *stk-- = 0x05050505L;                   //-- a1
    *stk-- = (unsigned int)param;           //-- a0 - task's function argument
    *stk-- = 0x03030303L;                   //-- v1
    *stk-- = 0x02020202L;                   //-- v0
    *stk-- = 0x01010101L;                   //-- at
    *stk-- = 0x33333333L;                   //-- hi
    *stk = 0x32323232L;                     //-- lo

    return stk;
}

//_____________________________________________________________________________
//
void tn_cpu_int_enable(void)
{
    __builtin_enable_interrupts();
}

void tn_cpu_int_disable(void)
{
    __builtin_disable_interrupts();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
