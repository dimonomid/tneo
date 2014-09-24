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

   /* ver 2.7  */

/**
 * \file
 */


#ifndef  _TN_ARCH_PIC32_INTERNAL_H
#define  _TN_ARCH_PIC32_INTERNAL_H

  /* --------- PIC32 port -------- */

#define  USE_ASM_FFS

#define  ffs_asm(x) (32-__builtin_clz((x)&(0-(x))))



#define  TN_FATAL_ERROR(error_msg, ...)         {__asm__ volatile(" sdbbp 0"); __asm__ volatile ("nop");}



#endif   // _TN_ARCH_PIC32_INTERNAL_H
