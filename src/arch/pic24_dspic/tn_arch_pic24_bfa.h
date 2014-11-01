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
 * Atomic bit-field access macros.
 *
 * Author: Alex Borisov, his article in russian could be found here: 
 * http://www.pic24.ru/doku.php/articles/mchp/c30_atomic_access
 *
 */

#ifndef _TN_ARCH_PIC24_BFA_H
#define _TN_ARCH_PIC24_BFA_H



/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/



#ifdef __cplusplus
extern "C"  {  /*}*/
#endif


/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/* Access params */

#define BFA_SET   0x1111
#define BFA_CLR   0x2222         
#define BFA_INV   0x3333         /* invert bit field */

#define BFA_WR    0xAAAA         /* write bit field */ 
#define BFA_RD    0xBBBB         /* read bit field */

#define __BFA_COMM_ERR(a)               __BFA_COMMAND_ERROR_##a
#define __BFA_COMM_GET(a)               __BFA_COMM_ERR(a)

typedef unsigned int __BFA_COMM_GET(BFA_SET);
typedef unsigned int __BFA_COMM_GET(BFA_CLR);
typedef unsigned int __BFA_COMM_GET(BFA_INV);

typedef unsigned int __BFA_COMM_GET(BFA_WR);
typedef unsigned int __BFA_COMM_GET(BFA_RD);

#define __BFA_STRUCT_TYPE(a)            a##BITS
#define __BFA_STRUCT_VAL(a)             a##bits

/**
 * Macro for atomic access to structure field. 
 *
 * comm       - command to execute
 *                  - BFA_WR  - write bit field
 *                  - BFA_RD  - read bit field
 *                  - BFA_SET - set bit fields by mask
 *                  - BFA_CLR - clear bit fields by mask
 *                  - BFA_INV - invert bit fields by mask
 * reg_name   - register name (PORTA, CMCON, ...).
 * field_name - structure field name
 * ...        - used if only comm = BFA_WR. Should be equal to the value
 *              to write to field. You can use variable here.
 *  
 * Usage examples: 
 *              BFA(BFA_WR,  IPC0, INT0IP, 0);
 *              BFA(BFA_INV, IPC0, INT0IP, 1);
 *              a = BFA(BFA_RD, IPC0, INT0IP);
 *              BFA(BFA_WR,  IPC0, INT0IP, a);
 *              BFA(BFA_SET, IPC0, INT0IP, (1 << 2));
 */
#define BFA(comm, reg_name, field_name, ...)                                \
    ({                                                                      \
        union                                                               \
        {                                                                   \
            __BFA_STRUCT_TYPE(reg_name) o;                                  \
            unsigned int                i;                                  \
        } lcur, lmask={}, lval={};                                          \
                                                                            \
        lmask.o.field_name = -1;                                            \
                                                                            \
        ((comm) == BFA_WR)                                                  \
        ?({                                                                 \
            __BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
            if (lmask.i & (lmask.i-1))                                      \
            {                                                               \
                lval.o.field_name = v;                                      \
                lcur.o = __BFA_STRUCT_VAL(reg_name);                        \
                __asm__ __volatile__                                        \
                ("xor %0":"=U"(reg_name):"a"(lmask.i & (lval.i^lcur.i)));   \
            }                                                               \
            else                                                            \
            {                                                               \
                if (v & 1)                                                  \
                {                                                           \
                    __BFA_STRUCT_VAL(reg_name).field_name = 1;              \
                }                                                           \
                else                                                        \
                {                                                           \
                    __BFA_STRUCT_VAL(reg_name).field_name = 0;              \
                }                                                           \
            }                                                               \
        }), 0                                                               \
                                                                            \
        :(((comm) == BFA_INV)                                               \
        ?({                                                                 \
            __BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
                lval.o.field_name = v;                                      \
                __asm__ __volatile__(" xor %0"                              \
                :"=U"(reg_name):"a"(lmask.i & lval.i));                     \
        }), 0                                                               \
                                                                            \
        :((((comm) == BFA_SET)                                              \
        ?({                                                                 \
            __BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
           if (lmask.i & (lmask.i-1))                                       \
           {                                                                \
               lval.o.field_name = v;                                       \
               __asm__ __volatile__                                         \
               ("ior %0":"=U"(reg_name):"a"(lmask.i & lval.i));             \
           }                                                                \
           else                                                             \
           {                                                                \
               if (v & 1)                                                   \
                   __BFA_STRUCT_VAL(reg_name).field_name = 1;               \
           }                                                                \
        }), 0                                                               \
                                                                            \
        :(((((comm) == BFA_CLR)                                             \
        ?({                                                                 \
            __BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
            if (lmask.i & (lmask.i-1))                                      \
            {                                                               \
                lval.o.field_name = v;                                      \
                __asm__ __volatile__                                        \
                ("and %0":"=U"(reg_name):"a"(~(lmask.i & lval.i)));         \
            }                                                               \
            else                                                            \
            {                                                               \
                if (v & 1)                                                  \
                    __BFA_STRUCT_VAL(reg_name).field_name = 0;              \
            }                                                               \
        }), 0                                                               \
                                                                            \
        :({ /* BFA_RD */                                                    \
             __BFA_STRUCT_VAL(reg_name).field_name;                         \
        })))))));                                                           \
        })






#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _TN_ARCH_PIC24_BFA_H

