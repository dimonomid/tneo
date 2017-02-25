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
 * Dispatch configuration: set predefined options, include user-provided cfg
 * file as well as default cfg file.
 *
 */

#ifndef _TN_CFG_DISPATCH_H
#define _TN_CFG_DISPATCH_H

#include "../arch/tn_arch_detect.h"

//-- Configuration constants
/**
 * In this case, you should use macro like this: `#TN_MAKE_ALIG(struct my_struct)`.
 * This way is used in the majority of TNKernel ports. (actually, in all ports
 * except the one by AlexB)
 */
#define  TN_API_MAKE_ALIG_ARG__TYPE       1

/**
 * In this case, you should use macro like this: `#TN_MAKE_ALIG(sizeof(struct
 * my_struct))`. This way is stated in TNKernel docs and used in the port for
 * dsPIC/PIC24/PIC32 by AlexB.
 */
#define  TN_API_MAKE_ALIG_ARG__SIZE       2


//--- As a starting point, you might want to copy tn_cfg_default.h -> tn_cfg.h,
//    and then edit it if you want to change default configuration.
//    NOTE: the file tn_cfg.h is specified in .hgignore file, in order to not include
//    project-specific configuration in the common TNKernel repository.
#include "tn_cfg.h"

//-- Probably a bit of hack, but anyway:
//   tn_cfg.h might just be a modified copy from existing tn_cfg_default.h, 
//   and then, _TN_CFG_DEFAULT_H is probably defined there. 
//   But we need to set some defaults, so, let's undef it.
//   Anyway, tn_cfg_default.h checks whether each particular option is already
//   defined, so it works nice.
#undef _TN_CFG_DEFAULT_H

//--- default cfg file is included too, so that you are free to not set
//    all available options in your tn_cfg.h file.
#include "tn_cfg_default.h"

//-- check that all options specified {{{

#if !defined(TN_CHECK_BUILD_CFG)
#  error TN_CHECK_BUILD_CFG is not defined
#endif

#if !defined(TN_PRIORITIES_CNT)
#  error TN_PRIORITIES_CNT is not defined
#endif


#if !defined(TN_CHECK_PARAM)
#  error TN_CHECK_PARAM is not defined
#endif

#if !defined(TN_DEBUG)
#  error TN_DEBUG is not defined
#endif

#if !defined(TN_OLD_TNKERNEL_NAMES)
#  error TN_OLD_TNKERNEL_NAMES is not defined
#endif

#if !defined(TN_USE_MUTEXES)
#  error TN_USE_MUTEXES is not defined
#endif

#if TN_USE_MUTEXES
#  if !defined(TN_MUTEX_REC)
#     error TN_MUTEX_REC is not defined
#  endif
#  if !defined(TN_MUTEX_DEADLOCK_DETECT)
#     error TN_MUTEX_DEADLOCK_DETECT is not defined
#  endif
#endif

#if !defined(TN_TICK_LISTS_CNT)
#  error TN_TICK_LISTS_CNT is not defined
#endif

#if !defined(TN_API_MAKE_ALIG_ARG)
#  error TN_API_MAKE_ALIG_ARG is not defined
#endif

#if !defined(TN_PROFILER)
#  error TN_PROFILER is not defined
#endif

#if !defined(TN_PROFILER_WAIT_TIME)
#  error TN_PROFILER_WAIT_TIME is not defined
#endif

#if !defined(TN_INIT_INTERRUPT_STACK_SPACE)
#  error TN_INIT_INTERRUPT_STACK_SPACE is not defined
#endif

#if !defined(TN_STACK_OVERFLOW_CHECK)
#  error TN_STACK_OVERFLOW_CHECK is not defined
#endif

#if defined (__TN_ARCH_PIC24_DSPIC__)
#  if !defined(TN_P24_SYS_IPL)
#     error TN_P24_SYS_IPL is not defined
#  endif
#endif

#if !defined(TN_DYNAMIC_TICK)
#  error TN_DYNAMIC_TICK is not defined
#endif

#if !defined(TN_OLD_EVENT_API)
#  error TN_OLD_EVENT_API is not defined
#endif

#if !defined(TN_FORCED_INLINE)
#  error TN_FORCED_INLINE is not defined
#endif

#if !defined(TN_MAX_INLINE)
#  error TN_MAX_INLINE is not defined
#endif


// }}}



//-- check TN_P24_SYS_IPL: should be 1 .. 6.
#if defined (__TN_ARCH_PIC24_DSPIC__)
#  if TN_P24_SYS_IPL >= 7
#     error TN_P24_SYS_IPL must be less than 7
#  endif
#  if TN_P24_SYS_IPL <= 0
#     error TN_P24_SYS_IPL must be more than 0
#  endif
#endif

//-- NOTE: TN_TICK_LISTS_CNT is checked in tn_timer_static.c
//-- NOTE: TN_PRIORITIES_CNT is checked in tn_sys.c
//-- NOTE: TN_API_MAKE_ALIG_ARG is checked in tn_common.h


/**
 * Internal kernel definition: set to non-zero if `_tn_sys_on_context_switch()`
 * should be called on context switch. 
 */
#if TN_PROFILER || TN_STACK_OVERFLOW_CHECK
#  define   _TN_ON_CONTEXT_SWITCH_HANDLER  1
#else
#  define   _TN_ON_CONTEXT_SWITCH_HANDLER  0
#endif

/**
 * If `#TN_STACK_OVERFLOW_CHECK` is set, we have 1-word overhead for each
 * task stack.
 */
#define _TN_STACK_OVERFLOW_SIZE_ADD    (TN_STACK_OVERFLOW_CHECK ? 1 : 0)

#endif // _TN_CFG_DISPATCH_H


