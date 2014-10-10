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

/**
 * \file
 *
 * TNeoKernel default configuration file, to be copied as `tn_cfg.h`.
 *
 * This project is intended to be built as a library, separately from main
 * project (although nothing prevents you from bundling things together, if you
 * want to).
 *
 * There are various options available which affects API and behavior of the
 * kernel. But these options are specific for particular project, and aren't
 * related to the kernel itself, so we need to keep them separately.
 *
 * To this end, file `tn.h` (the main kernel header file) includes `tn_cfg.h`,
 * which isn't included in the repository (even more, it is added to
 * `.hgignore` list actually). Instead, default configuration file
 * `tn_cfg_default.h` is provided, and when you just cloned the repository, you
 * might want to copy it as `tn_cfg.h`. Or even better, if your filesystem
 * supports symbolic links, copy it somewhere to your main project's directory
 * (so that you can add it to your VCS there), and create symlink to it named
 * `tn_cfg.h` in the TNeoKernel source directory, like this:
 *
 *     $ cd /path/to/tneokernel/src
 *     $ cp ./tn_cfg_default.h /path/to/main/project/lib_cfg/tn_cfg.h
 *     $ ln -s /path/to/main/project/lib_cfg/tn_cfg.h ./tn_cfg.h
 *
 * Default configuration file contains detailed comments, so you can read them
 * and configure behavior as you like.
 */

#ifndef _TN_CFG_DEFAULT_H
#define _TN_CFG_DEFAULT_H


/*******************************************************************************
 *    USER-DEFINED OPTIONS
 ******************************************************************************/

/**
 * Enables additional param checking for most of the system functions.
 * It's surely useful for debug, but probably better to remove in release.
 * If it is set, most of the system functions are able to return two additional
 * codes:
 *
 *    * `#TN_RC_WPARAM` if wrong params were given;
 *    * `#TN_RC_INVALID_OBJ` if given pointer doesn't point to a valid object.
 *      Object validity is checked by means of the special ID field of type
 *      `enum #TN_ObjId`.
 *
 * @see `enum #TN_ObjId`
 */
#ifndef TN_CHECK_PARAM
#  define TN_CHECK_PARAM         1
#endif

/**
 * Allows additional internal self-checking, useful to catch internal
 * TNeoKernel bugs as well as illegal kernel usage (e.g. sleeping in the idle 
 * task callback). Produces a couple of extra instructions which usually just
 * causes debugger to stop if something goes wrong.
 */
#ifndef TN_DEBUG
#  define TN_DEBUG               0
#endif

/**
 * Whether old TNKernel names (definitions, functions, etc) should be
 * available.  If you're porting your existing application written for
 * TNKernel, it is definitely worth enabling.  If you start new project with
 * TNeoKernel from scratch, it's better to avoid old names.
 */
#ifndef TN_OLD_TNKERNEL_NAMES
#  define TN_OLD_TNKERNEL_NAMES  1
#endif

/**
 * Whether mutexes API should be available
 */
#ifndef TN_USE_MUTEXES
#  define TN_USE_MUTEXES         1
#endif

/**
 * Whether mutexes should allow recursive locking/unlocking
 */
#ifndef TN_MUTEX_REC
#  define TN_MUTEX_REC           1
#endif

/**
 * Whether RTOS should detect deadlocks and notify user about them
 * via callback
 *
 * @see see `tn_callback_deadlock_set()`
 */
#ifndef TN_MUTEX_DEADLOCK_DETECT
#  define TN_MUTEX_DEADLOCK_DETECT  1
#endif

/**
 * Number of "tick" lists of timers, must be a power or two; minimum value:
 * `2`; typical values: `4`, `8` or `16`.
 *
 * Refer to the \ref timers_implementation for details.
 *
 * Shortly: this value represents number of elements in the array of 
 * `struct TN_ListItem`, on 32-bit system each element takes 8 bytes.
 *
 * The larger value, the more memory is needed, and the faster
 * $(TN_SYS_TIMER_LINK) ISR works. If your application has a lot of timers
 * and/or sleeping tasks, consider incrementing this value; otherwise,
 * default value should work for you.
 */
#ifndef TN_TICK_LISTS_CNT
#  define TN_TICK_LISTS_CNT    4
#endif


/**
 * API option for `MAKE_ALIG()` macro.
 *
 * There is a terrible mess with `MAKE_ALIG()` macro: original TNKernel docs
 * specify that the argument of it should be the size to align, but almost all
 * ports, including "original" one, defined it so that it takes type, not size.
 *
 * But the port by AlexB implemented it differently
 * (i.e. accordingly to the docs)
 *
 * When I was moving from the port by AlexB to another one, do you have any
 * idea how much time it took me to figure out why do I have rare weird bug? :)
 *
 * So, available options:
 *
 *  * `#TN_API_MAKE_ALIG_ARG__TYPE`: 
 *             In this case, you should use macro like this: 
 *                `TN_MAKE_ALIG(struct my_struct)`
 *             This way is used in the majority of TNKernel ports.
 *             (actually, in all ports except the one by AlexB)
 *
 *  * `#TN_API_MAKE_ALIG_ARG__SIZE`:
 *             In this case, you should use macro like this: 
 *                `TN_MAKE_ALIG(sizeof(struct my_struct))`
 *             This way is stated in TNKernel docs
 *             and used in the port for dsPIC/PIC24/PIC32 by AlexB.
 */
#ifndef TN_API_MAKE_ALIG_ARG
#  define TN_API_MAKE_ALIG_ARG     TN_API_MAKE_ALIG_ARG__SIZE
#endif


#endif // _TN_CFG_DEFAULT_H


