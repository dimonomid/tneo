/*******************************************************************************
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

/*
 * Cortex-M context layout
 *
 * First of all, there is a stack frame created by hardware:
 *
 *    - xPSR
 *    - Return Address
 *    - LR
 *    - R12
 *    - R3
 *    - R2
 *    - R1
 *    - R0
 *
 *    - {Probably, "caller-saved" floating point registers}
 *
 * Then, all the "callee-saved" registers:
 *
 *    - {Probably, "callee-saved" floating-point registers S16-S31}
 *
 *    - EXC_RETURN (i.e. value of LR when ISR is called, saved on M3/M4/M4F only)
 *
 *          Actually, saving of EXC_RETURN in task context is necessary for M4F
 *          only, because usage of FPU is the only thing that can differ in
 *          EXC_RETURN for different tasks, but I don't want to complicate
 *          things even more, so let it be. 
 *
 *          But on Cortex-M0/M0+ we anyway have different code for
 *          saving/restoring context.
 *
 *    - R11
 *    - R10
 *    - R9
 *    - R8
 *    - R7
 *    - R6
 *    - R5
 *    - R4
 *
 *
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "_tn_tasks.h"



/*******************************************************************************
 *    EXTERNAL DATA
 ******************************************************************************/




/*******************************************************************************
 *    EXTERNAL FUNCTION PROTOTYPES
 ******************************************************************************/



/*******************************************************************************
 *    PROTECTED DATA
 ******************************************************************************/


/*******************************************************************************
 *    CORTEX-M SPECIFIC FUNCTIONS
 ******************************************************************************/


/*******************************************************************************
 *    IMPLEMENTATION
 ******************************************************************************/




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
   TN_UWord *cur_stack_pt = stack_high_addr + 1/*'full desc stack' model*/;

   //-- xPSR register: the bit "T" (Thumb) should be set,
   //   its offset is 24.
   *(--cur_stack_pt) = (1 << 24);

   //-- Return address: this is address of the task body function.
   *(--cur_stack_pt) = (TN_UWord)task_func;

   //-- LR: where to go if task body function returns
   *(--cur_stack_pt) = (TN_UWord)_tn_task_exit_nodelete;

   *(--cur_stack_pt) = 0x12121212;           //-- R12
   *(--cur_stack_pt) = 0x03030303;           //-- R3
   *(--cur_stack_pt) = 0x02020202;           //-- R2
   *(--cur_stack_pt) = 0x01010101;           //-- R1
   *(--cur_stack_pt) = (TN_UWord)param;      //-- R0: argument for task body func


#if defined(__TN_ARCHFEAT_CORTEX_M_FPU__)
   //-- NOTE: at this point, there are floating-point registers S16-S31
   //   if bit 0x00000010 of EXC_RETURN is cleared.
   //   Initially, it is of course set (see below), so that we don't have
   //   these registers in initial stack.
#endif

#if defined(__TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
   //-- EXC_RETURN:
   //    - floating point is not used by the task at the moment;
   //    - return to Thread mode;
   //    - use PSP.
   //
   //   It is saved in task context for M3/M4/M4F only, see comments
   //   about context layout above for details.
   *(--cur_stack_pt) = 0xFFFFFFFD;
#endif

   *(--cur_stack_pt) = 0x11111111;           //-- R11
   *(--cur_stack_pt) = 0x10101010;           //-- R10
   *(--cur_stack_pt) = 0x09090909;           //-- R9
   *(--cur_stack_pt) = 0x08080808;           //-- R8
   *(--cur_stack_pt) = 0x07070707;           //-- R7
   *(--cur_stack_pt) = 0x06060606;           //-- R6
   *(--cur_stack_pt) = 0x05050505;           //-- R5
   *(--cur_stack_pt) = 0x04040404;           //-- R4

   _TN_UNUSED(stack_low_addr);

   return cur_stack_pt;
}


