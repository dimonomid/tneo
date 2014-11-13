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

/*
 * Cortex-M context layout
 *
 * First of all, there is a stack frame created by hardware:
 *
 *    xPSR
 *    Return Address
 *    LR
 *    R12
 *    R3
 *    R2
 *    R1
 *    R0
 *
 *    {Probably, "callee-saved" floating point registers}
 *
 * Then, all the "caller-saved" registers:
 *
 *    R11
 *    R10
 *    R9
 *    R8
 *    R7
 *    R6
 *    R5
 *    R4
 *
 *    {TODO: "caller-saved" floating point registers, if TN_CORTEX_M_FPU is set}
 *
 *
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "_tn_tasks.h"

//-- include app-specific header for CMSIS.
//   Say, if application uses STM32F407VG CPU, the file is "stm32f4xx.h"
#include "../../tn_app_c.h"



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
 *    IMPLEMENTATION
 ******************************************************************************/

#if 0
void _tn_arch_sys_start(
      TN_UWord            *int_stack,
      unsigned int         int_stack_size
      )
{
   //-- Set PendSV to the minimum priority
   NVIC_SetPriority(PendSV_IRQn, 0xff);


}
#endif




/*
 * See comments in the `tn_arch.h` file
 */
TN_UWord *_tn_arch_stack_top_get(
      TN_UWord *stack_low_address,
      int stack_size
      )
{
   //-- on Cortex-M, stack is "full descending stack", so
   //   we return highest stack address plus one.
   return stack_low_address + stack_size;
}


/*
 * See comments in the file `tn_arch.h`
 */
TN_UWord *_tn_arch_stack_init(
      TN_TaskBody   *task_func,
      TN_UWord      *stack_top,
      int            stack_size,
      void          *param
      )
{
   //-- xPSR register: the bit "T" (Thumb) should be set,
   //   its offset is 24.
   *(--stack_top) = (1 << 24);

   //-- Return address: this is address of the task body function.
   *(--stack_top) = (TN_UWord)task_func;

   //-- LR: where to go if task body function returns
   *(--stack_top) = (TN_UWord)_tn_task_exit_nodelete;

   *(--stack_top) = 0x12121212;           //-- R12
   *(--stack_top) = 0x03030303;           //-- R3
   *(--stack_top) = 0x02020202;           //-- R2
   *(--stack_top) = 0x01010101;           //-- R1
   *(--stack_top) = 0xfefefefe;           //-- R0

   *(--stack_top) = 0x11111111;           //-- R11
   *(--stack_top) = 0x10101010;           //-- R10
   *(--stack_top) = 0x09090909;           //-- R9
   *(--stack_top) = 0x08080808;           //-- R8
   *(--stack_top) = 0x07070707;           //-- R7
   *(--stack_top) = 0x06060606;           //-- R6
   *(--stack_top) = 0x05050505;           //-- R5
   *(--stack_top) = 0x04040404;           //-- R4

   // TODO: "caller-saved" floating point registers, if TN_CORTEX_M_FPU is set

   return stack_top;
}


/*
 * See comments in the file `tn_arch.h`
 */
void tn_arch_int_dis(void)
{
   __disable_irq();
}


/*
 * See comments in the file `tn_arch.h`
 */
void tn_arch_int_en(void)
{
   __enable_irq();
}


/*
 * See comments in the file `tn_arch.h`
 */
TN_UWord tn_arch_sr_save_int_dis(void)
{
   TN_UWord primask = __get_PRIMASK();
   __disable_irq();
   return primask;
}


/*
 * See comments in the file `tn_arch.h`
 */
void tn_arch_sr_restore(TN_UWord primask)
{
   __set_PRIMASK(primask);
}

int _tn_arch_is_int_disabled(void)
{
   return (__get_PRIMASK() != 0);
}

int _tn_arch_inside_isr(void)
{
   return !(__get_CONTROL() & 0x02);
}

void _tn_arch_context_switch_pend(void)
{
   //-- activate PendSV
   *((volatile unsigned long *)0xE000ED04) = 0x10000000;
}

void _tn_arch_context_switch_now_nosave(void)
{
   _tn_arch_context_switch_pend();
   __enable_irq();
}


