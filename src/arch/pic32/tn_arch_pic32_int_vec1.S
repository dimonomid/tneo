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

/**
 * \file
 *
 * This file contains ISR for core software interrupt 0 (which is used by the
 * kernel internally).
 *
 * **NOTE:** This file must be included in the main project itself, not in the
 * library project.
 *
 */



/*
 * Self-check for the application that uses TNeo:
 *
 *  PIC32 application must include the file tn_arch_pic32_int_vec1.S
 * in order to dispatch vector_1 correctly, but if it is forgotten,
 * no error is generated at the build time: we just get to the 
 * _DefaultInterrupt when we should switch context.
 *
 * Note that we can't include that file to the TNeo library
 * project: it doesn't work.
 *
 * So, dummy function was invented, and if we forgot to 
 * include that file, we got an error at the link time.
 *
 * That function merely returns 0.
 */
    .global  _you_should_add_file___tn_arch_pic32_int_vec1_S___to_the_project
    .ent _you_should_add_file___tn_arch_pic32_int_vec1_S___to_the_project
_you_should_add_file___tn_arch_pic32_int_vec1_S___to_the_project:

    li      $v0, 0
    jr      $ra
    nop

    .end _you_should_add_file___tn_arch_pic32_int_vec1_S___to_the_project





    .section    .vector_1,code
    .align  2
    .set    nomips16

    .extern cs0_int_handler
    .global __vector_dispatch_1

    .ent    __vector_dispatch_1

__vector_dispatch_1:

    j       cs0_int_handler
    .end    __vector_dispatch_1
    .size   __vector_dispatch_1, .-__vector_dispatch_1

