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
 * Macros that may be useful for any part of the kernel.
 * Note: only preprocessor macros allowed here, so that the file can be
 * included in any source file (C, assembler, or whatever)
 */

#ifndef _TN_COMMON_MACROS_H
#define _TN_COMMON_MACROS_H


/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


/**
 * Macro that expands to string representation of its argument:
 * for example, 
 *
 * \code{.c}
 *    _TN_STRINGIFY_LITERAL(5)
 * \endcode
 *
 * expands to:
 *
 * \code{.c}
 *    "5"
 * \endcode
 *
 * See also _TN_STRINGIFY_MACRO()
 */
#define _TN_STRINGIFY_LITERAL(x)    #x

/**
 * Macro that expands to string representation of its argument, which is
 * allowed to be a macro:
 * for example, 
 *
 * \code{.c}
 *    #define MY_VALUE     10
 *    _TN_STRINGIFY_MACRO(MY_VALUE)
 * \endcode
 *
 * expands to:
 *
 * \code{.c}
 *    "10"
 * \endcode
 */
#define _TN_STRINGIFY_MACRO(x)      _TN_STRINGIFY_LITERAL(x)




#endif // _TN_COMMON_MACROS_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


