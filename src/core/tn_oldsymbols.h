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
 * Compatibility layer for old projects that use old TNKernel names;
 * usage of them in new projects is discouraged.
 *
 * If you're porting your existing application written for TNKernel, it 
 * might be useful though.
 *
 * Included automatially if the option `#TN_OLD_TNKERNEL_NAMES` is set.
 *
 */

#ifndef _TN_OLDSYMBOLS_H
#define _TN_OLDSYMBOLS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_common.h"



#ifndef TN_OLD_TNKERNEL_NAMES
#  error TN_OLD_TNKERNEL_NAMES is not defined
#endif

#if TN_OLD_TNKERNEL_NAMES || DOXYGEN_ACTIVE


#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/// old TNKernel name of `TN_ListItem`
typedef struct TN_ListItem    CDLL_QUEUE;

/// old TNKernel name of `#TN_Mutex`
typedef struct TN_Mutex       TN_MUTEX;

/// old TNKernel name of `#TN_DQueue`
typedef struct TN_DQueue      TN_DQUE;

/// old TNKernel name of `#TN_Task` 
typedef struct TN_Task        TN_TCB;

/// old TNKernel name of `#TN_FMem` 
typedef struct TN_FMem        TN_FMP;

/// old TNKernel name of `#TN_Sem`
typedef struct TN_Sem         TN_SEM;


#if TN_OLD_EVENT_API
/// old TNKernel name of `#TN_EventGrp`
/// available if only `#TN_OLD_EVENT_API` is non-zero
typedef struct TN_EventGrp    TN_EVENT;
#endif




/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*
 * compatibility with old struct names
 */
/// old TNKernel struct name of `TN_ListItem`
#define  _CDLL_QUEUE    TN_ListItem

/// old TNKernel struct name of `#TN_Mutex`
#define  _TN_MUTEX      TN_Mutex

/// old TNKernel struct name of `#TN_DQueue`
#define  _TN_DQUE       TN_DQueue

/// old TNKernel struct name of `#TN_Task`
#define  _TN_TCB        TN_Task

/// old TNKernel struct name of `#TN_FMem`
#define  _TN_FMP        TN_FMem

/// old TNKernel struct name of `#TN_Sem`
#define  _TN_SEM        TN_Sem

#if TN_OLD_EVENT_API || defined(DOXYGEN_ACTIVE)
/// old TNKernel struct name of `#TN_EventGrp`,
/// available if only `#TN_OLD_EVENT_API` is non-zero
#  define  _TN_EVENT      TN_EventGrp
#endif

/// old TNKernel name of `#TN_MAKE_ALIG` macro
///
/// \attention it is recommended to use `#TN_MAKE_ALIG_SIZE` macro instead
/// of this one, in order to avoid confusion caused by various
/// TNKernel ports: refer to the section \ref tnkernel_diff_make_alig for details.
#define  MAKE_ALIG                     TN_MAKE_ALIG


/// old TNKernel name of `#TN_TASK_STATE_RUNNABLE`
#define  TSK_STATE_RUNNABLE            TN_TASK_STATE_RUNNABLE

/// old TNKernel name of `#TN_TASK_STATE_WAIT`
#define  TSK_STATE_WAIT                TN_TASK_STATE_WAIT

/// old TNKernel name of `#TN_TASK_STATE_SUSPEND`
#define  TSK_STATE_SUSPEND             TN_TASK_STATE_SUSPEND

/// old TNKernel name of `#TN_TASK_STATE_WAITSUSP`
#define  TSK_STATE_WAITSUSP            TN_TASK_STATE_WAITSUSP

/// old TNKernel name of `#TN_TASK_STATE_DORMANT`
#define  TSK_STATE_DORMANT             TN_TASK_STATE_DORMANT

/// old TNKernel name of `#TN_TASK_CREATE_OPT_START`
#define  TN_TASK_START_ON_CREATION     TN_TASK_CREATE_OPT_START

/// old TNKernel name of `#TN_TASK_EXIT_OPT_DELETE`
#define  TN_EXIT_AND_DELETE_TASK       TN_TASK_EXIT_OPT_DELETE



/// old TNKernel name of `#TN_EVENTGRP_WMODE_AND`
#define  TN_EVENT_WCOND_AND            TN_EVENTGRP_WMODE_AND

/// old TNKernel name of `#TN_EVENTGRP_WMODE_OR`
#define  TN_EVENT_WCOND_OR             TN_EVENTGRP_WMODE_OR


/// old TNKernel name of `#TN_WAIT_REASON_NONE`
#define  TSK_WAIT_REASON_NONE          TN_WAIT_REASON_NONE

/// old TNKernel name of `#TN_WAIT_REASON_SLEEP`
#define  TSK_WAIT_REASON_SLEEP         TN_WAIT_REASON_SLEEP

/// old TNKernel name of `#TN_WAIT_REASON_SEM`
#define  TSK_WAIT_REASON_SEM           TN_WAIT_REASON_SEM

/// old TNKernel name of `#TN_WAIT_REASON_EVENT`
#define  TSK_WAIT_REASON_EVENT         TN_WAIT_REASON_EVENT

/// old TNKernel name of `#TN_WAIT_REASON_DQUE_WSEND`
#define  TSK_WAIT_REASON_DQUE_WSEND    TN_WAIT_REASON_DQUE_WSEND

/// old TNKernel name of `#TN_WAIT_REASON_DQUE_WRECEIVE`
#define  TSK_WAIT_REASON_DQUE_WRECEIVE TN_WAIT_REASON_DQUE_WRECEIVE

/// old TNKernel name of `#TN_WAIT_REASON_MUTEX_C`
#define  TSK_WAIT_REASON_MUTEX_C       TN_WAIT_REASON_MUTEX_C

/// old TNKernel name of `#TN_WAIT_REASON_MUTEX_I`
#define  TSK_WAIT_REASON_MUTEX_I       TN_WAIT_REASON_MUTEX_I

/// old TNKernel name of `#TN_WAIT_REASON_WFIXMEM`
#define  TSK_WAIT_REASON_WFIXMEM       TN_WAIT_REASON_WFIXMEM



/// old TNKernel name of `#TN_RC_OK`
#define  TERR_NO_ERR                   TN_RC_OK

/// old TNKernel name of `#TN_RC_OVERFLOW`
#define  TERR_OVERFLOW                 TN_RC_OVERFLOW

/// old TNKernel name of `#TN_RC_WCONTEXT`
#define  TERR_WCONTEXT                 TN_RC_WCONTEXT

/// old TNKernel name of `#TN_RC_WSTATE`
#define  TERR_WSTATE                   TN_RC_WSTATE

/// old TNKernel name of `#TN_RC_TIMEOUT`
#define  TERR_TIMEOUT                  TN_RC_TIMEOUT

/// old TNKernel name of `#TN_RC_WPARAM`
#define  TERR_WRONG_PARAM              TN_RC_WPARAM

/// old TNKernel name of `#TN_RC_ILLEGAL_USE`
#define  TERR_ILUSE                    TN_RC_ILLEGAL_USE

/// old TNKernel name of `#TN_RC_INVALID_OBJ`
#define  TERR_NOEXS                    TN_RC_INVALID_OBJ

/// old TNKernel name of `#TN_RC_DELETED`
#define  TERR_DLT                      TN_RC_DELETED

/// old TNKernel name of `#TN_RC_FORCED`
#define  TERR_FORCED                   TN_RC_FORCED

/// old TNKernel name of `#TN_RC_INTERNAL`
#define  TERR_INTERNAL                 TN_RC_INTERNAL



/// old TNKernel name of `#TN_MUTEX_PROT_CEILING`
#define  TN_MUTEX_ATTR_CEILING         TN_MUTEX_PROT_CEILING

/// old TNKernel name of `#TN_MUTEX_PROT_INHERIT`
#define  TN_MUTEX_ATTR_INHERIT         TN_MUTEX_PROT_INHERIT




/// old TNKernel name of `#tn_sem_acquire_polling`
#define  tn_sem_polling                tn_sem_acquire_polling

/// old TNKernel name of `#tn_sem_iacquire_polling`
#define  tn_sem_ipolling               tn_sem_iacquire_polling


/// old name of `#tn_sem_wait`
#define  tn_sem_acquire                tn_sem_wait

/// old name of `#tn_sem_wait_polling`
#define  tn_sem_acquire_polling        tn_sem_wait_polling

/// old name of `#tn_sem_iwait_polling`
#define  tn_sem_iacquire_polling       tn_sem_iwait_polling



/// old TNKernel name of `#tn_fmem_iget_polling`
#define  tn_fmem_get_ipolling          tn_fmem_iget_polling


/// old TNKernel name of `#tn_queue_ireceive_polling`
#define  tn_queue_ireceive             tn_queue_ireceive_polling


/// old TNKernel name of `#tn_sys_start`
#define  tn_start_system               tn_sys_start

/// old TNKernel name of `#tn_sys_tslice_set`
#define  tn_sys_tslice_ticks           tn_sys_tslice_set



/// old TNKernel name of `#TN_ARCH_STK_ATTR_BEFORE`
#define  align_attr_start              TN_ARCH_STK_ATTR_BEFORE

/// old TNKernel name of `#TN_ARCH_STK_ATTR_AFTER`
#define  align_attr_end                TN_ARCH_STK_ATTR_AFTER


/// old TNKernel name of `#tn_arch_int_dis`
#define  tn_cpu_int_disable            tn_arch_int_dis

/// old TNKernel name of `#tn_arch_int_en`
#define  tn_cpu_int_enable             tn_arch_int_en


/// old TNKernel name of `#tn_arch_sr_save_int_dis`
#define  tn_cpu_save_sr                tn_arch_sr_save_int_dis

/// old TNKernel name of `#tn_arch_sr_restore`
#define  tn_cpu_restore_sr             tn_arch_sr_restore


/// old TNKernel name of `#TN_INT_DIS_SAVE`
#define  tn_disable_interrupt          TN_INT_DIS_SAVE

/// old TNKernel name of `#TN_INT_RESTORE`
#define  tn_enable_interrupt           TN_INT_RESTORE


/// old TNKernel name of `#TN_INT_IDIS_SAVE`
#define  tn_idisable_interrupt         TN_INT_IDIS_SAVE

/// old TNKernel name of `#TN_INT_IRESTORE`
#define  tn_ienable_interrupt          TN_INT_IRESTORE


/// old TNKernel name of `#TN_IS_INT_DISABLED`
#define  tn_chk_irq_disabled           TN_IS_INT_DISABLED

/// old TNKernel name of `#TN_PRIORITIES_CNT`
#define  TN_NUM_PRIORITY               TN_PRIORITIES_CNT

/// old TNKernel name of `#TN_INT_WIDTH`
#define  _TN_BITS_IN_INT                TN_INT_WIDTH

/// old TNKernel name for `sizeof(#TN_UWord)`
#define  TN_ALIG                       sizeof(TN_UWord)

/// old TNKernel name for `#TN_NO_TIME_SLICE`
#define  NO_TIME_SLICE                 TN_NO_TIME_SLICE

/// old TNKernel name for `#TN_MAX_TIME_SLICE`
#define  MAX_TIME_SLICE                TN_MAX_TIME_SLICE




/// old name for `#TN_STACK_ARR_DEF`
#define  TN_TASK_STACK_DEF             TN_STACK_ARR_DEF

/// old name for `#TN_TickCnt`
#define  TN_Timeout                    TN_TickCnt



//-- event stuff {{{

#if TN_OLD_EVENT_API || DOXYGEN_ACTIVE

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#TN_EVENTGRP_ATTR_SINGLE`,
#define  TN_EVENT_ATTR_SINGLE          TN_EVENTGRP_ATTR_SINGLE

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#TN_EVENTGRP_ATTR_MULTI`,
#define  TN_EVENT_ATTR_MULTI           TN_EVENTGRP_ATTR_MULTI

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#TN_EVENTGRP_ATTR_CLR`,
#define  TN_EVENT_ATTR_CLR             TN_EVENTGRP_ATTR_CLR

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#tn_eventgrp_create_wattr()`,
#define  tn_event_create(ev, attr, pattern)  \
         tn_eventgrp_create_wattr((ev), (enum TN_EGrpAttr)(attr), (pattern))

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#tn_eventgrp_delete()`,
#define  tn_event_delete               tn_eventgrp_delete

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#tn_eventgrp_wait()`,
#define  tn_event_wait                 tn_eventgrp_wait

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#tn_eventgrp_wait_polling()`,
#define  tn_event_wait_polling         tn_eventgrp_wait_polling

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#tn_eventgrp_iwait_polling()`,
#define  tn_event_iwait                tn_eventgrp_iwait_polling

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old TNKernel-compatible way of calling `#tn_eventgrp_modify (event,
/// #TN_EVENTGRP_OP_SET, pattern)`
#define  tn_event_set(ev, pattern)     tn_eventgrp_modify ((ev), TN_EVENTGRP_OP_SET, (pattern))

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old TNKernel-compatible way of calling `#tn_eventgrp_imodify (event,
/// #TN_EVENTGRP_OP_SET, pattern)`
#define  tn_event_iset(ev, pattern)    tn_eventgrp_imodify((ev), TN_EVENTGRP_OP_SET, (pattern))

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old TNKernel-compatible way of calling `#tn_eventgrp_modify (event,
/// #TN_EVENTGRP_OP_CLEAR, (~pattern))`
///
/// \attention Unlike `#tn_eventgrp_modify()`, the pattern should be inverted!
#define  tn_event_clear(ev, pattern)   tn_eventgrp_modify ((ev), TN_EVENTGRP_OP_CLEAR, (~(pattern)))

/// \attention Deprecated. Available if only `#TN_OLD_EVENT_API` option is
/// non-zero.
///
/// Old TNKernel-compatible way of calling `#tn_eventgrp_imodify (event,
/// #TN_EVENTGRP_OP_CLEAR, (~pattern))`
///
/// \attention Unlike `#tn_eventgrp_modify()`, the pattern should be inverted!
#define  tn_event_iclear(ev, pattern)  tn_eventgrp_imodify((ev), TN_EVENTGRP_OP_CLEAR, (~(pattern)))

#else
//-- no compatibility with event API
#endif

//}}}


//-- Internal implementation details {{{

//-- Exclude it from doxygen-generated docs
#if !DOXYGEN_SHOULD_SKIP_THIS

#define tn_ready_list         _tn_tasks_ready_list
#define tn_create_queue       _tn_tasks_created_list
#define tn_created_tasks_cnt  _tn_tasks_created_cnt

#define tn_tslice_ticks       _tn_tslice_ticks

#define tn_sys_state          _tn_sys_state

#define tn_next_task_to_run   _tn_next_task_to_run
#define tn_curr_run_task      _tn_curr_run_task

#define tn_ready_to_run_bmp   _tn_ready_to_run_bmp

#define tn_idle_task          _tn_idle_task

#endif

//}}}

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // TN_OLD_TNKERNEL_NAMES

#endif // _TN_OLDSYMBOLS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


