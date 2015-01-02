/*******************************************************************************
 *
 * TNeo: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeo:                      copyright © 2014       Dmitry Frank.
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
 * Atomic bit-field access macros for PIC24/dsPIC.
 *
 * Initially, the code was taken from the [article by Alex Borisov
 * (russian)](http://www.pic24.ru/doku.php/articles/mchp/c30_atomic_access), 
 * and modified a bit.
 *
 * The kernel would not probably provide that kind of functionality, but 
 * the kernel itself needs it, so, it is made public so that application can
 * use it too.
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

///
/// Command for `TN_BFA()` macro: Set bits in the bit field by mask;
/// `...` macro param should be set to the bit mask to set.
#define TN_BFA_SET   0x1111
///
/// Command for `TN_BFA()` macro: Clear bits in the bit field by mask;
/// `...` macro param should be set to the bit mask to clear.
#define TN_BFA_CLR   0x2222         
///
/// Command for `TN_BFA()` macro: Invert bits in the bit field by mask;
/// `...` macro param should be set to the bit mask to invert.
#define TN_BFA_INV   0x3333
///
/// Command for `TN_BFA()` macro: Write bit field;
/// `...` macro param should be set to the value to write.
#define TN_BFA_WR    0xAAAA
///
/// Command for `TN_BFA()` macro: Read bit field;
/// `...` macro param ignored.
#define TN_BFA_RD    0xBBBB



/**
 * Macro for atomic access to the structure bit field. The `BFA` acronym 
 * means Bit Field Access.
 *
 * @param comm 
 *    command to execute:
 *    - `#TN_BFA_WR`  - write bit field
 *    - `#TN_BFA_RD`  - read bit field
 *    - `#TN_BFA_SET` - set bits by mask
 *    - `#TN_BFA_CLR` - clear bits by mask
 *    - `#TN_BFA_INV` - invert bits by mask
 *
 * @param reg_name
 *    register name (`PORTA`, `CMCON`, ...).
 *
 * @param field_name
 *    structure field name
 *
 * @param ...
 *    used if only `comm != #TN_BFA_RD`. Meaning depends on the `comm`, 
 *    see comments for specific command: `#TN_BFA_WR`, etc.
 *  
 *
 * Usage examples:
 * 
 * \code{.c}
 *
 *    int a = 0x02;
 *
 *    //-- Set third bit of the INT0IP field in the IPC0 register:
 *    //   IPC0bits.INT0IP |= (1 << 2);
 *    TN_BFA(TN_BFA_SET, IPC0, INT0IP, (1 << 2));
 *
 *    //-- Clear second bit of the INT0IP field in the IPC0 register:
 *    //   IPC0bits.INT0IP &= ~(1 << 1);
 *    TN_BFA(TN_BFA_CLR, IPC0, INT0IP, (1 << 1));
 *
 *    //-- Invert two less-significant bits of the INT0IP field 
 *    //   in the IPC0 register:
 *    //   IPC0bits.INT0IP ^= 0x03;
 *    TN_BFA(TN_BFA_INV, IPC0, INT0IP, 0x03);
 *
 *    //-- Write value 0x05 to the INT0IP field of the IPC0 register:
 *    //   IPC0bits.INT0IP = 0x05;
 *    TN_BFA(TN_BFA_WR, IPC0, INT0IP, 0x05);
 * 
 *    //-- Write value of the variable a to the INT0IP field of the IPC0 
 *    //   register:
 *    //   IPC0bits.INT0IP = a;
 *    TN_BFA(TN_BFA_WR, IPC0, INT0IP, a);
 *
 *    //-- Read the value that is stored in the INT0IP field of the IPC0 
 *    //   register, to the int variable a:
 *    //   int a = IPC0bits.INT0IP;
 *    a = TN_BFA(TN_BFA_RD, IPC0, INT0IP);
 *
 * \endcode
 */
#define TN_BFA(comm, reg_name, field_name, ...)                            \
   ({                                                                      \
      ((comm) == TN_BFA_WR)                                                \
      ?({                                                                  \
         _TN_BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
        if (_TN_BFA_FIELD_DEF(reg_name, field_name, LENGTH) == 1) {        \
            if (v & 1) {                                                   \
                _TN_BFA_REG_DEF(reg_name, SET) =                           \
                    _TN_BFA_FIELD_DEF(reg_name, field_name, MASK);         \
            } else {                                                       \
                _TN_BFA_REG_DEF(reg_name, CLR) =                           \
                    _TN_BFA_FIELD_DEF(reg_name, field_name, MASK);         \
            }                                                              \
        } else {                                                           \
            _TN_BFA_REG_DEF(reg_name, INV) =                               \
                  _TN_BFA_FIELD_DEF(reg_name, field_name, MASK)            \
                   &                                                       \
                   (                                                       \
                    ((v) << _TN_BFA_FIELD_DEF(                             \
                       reg_name, field_name, POSITION                      \
                       ))                                                  \
                    ^                                                      \
                    reg_name                                               \
                   );                                                      \
        }                                                                  \
      }), 0                                                                \
                                                                           \
      :(((comm) == TN_BFA_INV)                                             \
      ?({                                                                  \
         _TN_BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
         _TN_BFA_REG_DEF(reg_name, INV) =                                  \
               (                                                           \
                _TN_BFA_FIELD_DEF(reg_name, field_name, MASK)              \
                &                                                          \
                ((v) << _TN_BFA_FIELD_DEF(reg_name, field_name, POSITION)) \
               );                                                          \
      }), 0                                                                \
                                                                           \
      :((((comm) == TN_BFA_SET)                                            \
      ?({                                                                  \
         _TN_BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
         _TN_BFA_REG_DEF(reg_name, SET) =                                  \
               (                                                           \
                _TN_BFA_FIELD_DEF(reg_name, field_name, MASK)              \
                &                                                          \
                ((v) << _TN_BFA_FIELD_DEF(reg_name, field_name, POSITION)) \
               );                                                          \
      }), 0                                                                \
                                                                           \
      :(((((comm) == TN_BFA_CLR)                                           \
      ?({                                                                  \
         _TN_BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
         _TN_BFA_REG_DEF(reg_name, CLR) =                                  \
               (                                                           \
                _TN_BFA_FIELD_DEF(reg_name, field_name, MASK)              \
                &                                                          \
                ((v) << _TN_BFA_FIELD_DEF(reg_name, field_name, POSITION)) \
               );                                                          \
      }), 0                                                                \
                                                                           \
      :({ /* TN_BFA_RD */                                                  \
          _TN_BFA_STRUCT_VAL(reg_name).field_name;                         \
      })))))));                                                            \
      })



/**
 * Macro for atomic access to the structure bit field specified as a range.
 *
 * @param comm
 *    command to execute:
 *    - `#TN_BFA_WR`  - write bit field
 *    - `#TN_BFA_RD`  - read bit field
 *    - `#TN_BFA_SET` - set bits by mask
 *    - `#TN_BFA_CLR` - clear bits by mask
 *    - `#TN_BFA_INV` - invert bits by mask
 *
 * @param reg_name
 *    variable name (`PORTA`, `CMCON`, ...). Variable should be in the near
 *    memory (first 8 KB)
 *
 * @param lower
 *    number of lowest affected bit of the field
 *
 * @param upper
 *    number of highest affected bit of the field
 *
 * @param ...
 *    used if only `comm != #TN_BFA_RD`. Meaning depends on the `comm`, 
 *    see comments for specific command: `#TN_BFA_WR`, etc.
 *  
 *
 * Usage examples:
 * 
 * \code{.c}
 *
 *    int a = 0x02;
 *
 *    //-- Write constant 0xaa to the least significant byte of the TRISB
 *    //   register:
 *    TN_BFAR(TN_BFA_WR, TRISB, 0, 7, 0xaa);
 *
 *    //-- Invert least significant nibble of the most significant byte 
 *    //   in the register TRISB:
 *    TN_BFAR(TN_BFA_INV, TRISB, 8, 15, 0x0f);
 *
 *    //-- Get 5 least significant bits from the register TRISB and store
 *    //   result to the variable a
 *    a = TN_BFAR(TN_BFA_RD, TRISB, 0, 4);
 *
 * \endcode
 *    
 */
#define TN_BFAR(comm, reg_name, lower, upper, ...)                         \
   ({                                                                      \
      unsigned int mask, maskl, masku;                                     \
      maskl = (1 << (lower));                                              \
      masku = (1 << (upper));                                              \
      mask  = ((maskl-1) ^ (masku-1)) | maskl | masku;                     \
                                                                           \
      ((comm) == TN_BFA_WR)                                                \
      ?({                                                                  \
         _TN_BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
         if ((lower) == (upper)) {                                         \
            if (v & 1) {                                                   \
               _TN_BFA_REG_DEF(reg_name, SET) = mask;                      \
            } else {                                                       \
               _TN_BFA_REG_DEF(reg_name, CLR) = mask;                      \
            }                                                              \
         } else {                                                          \
            _TN_BFA_REG_DEF(reg_name, INV) =                               \
                  mask                                                     \
                  &                                                        \
                  (((v) << _TN_BFA_MIN((lower), (upper))) ^ reg_name);     \
        }                                                                  \
      }), 0                                                                \
                                                                           \
      :(((comm) == TN_BFA_INV)                                             \
      ?({                                                                  \
         _TN_BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
         _TN_BFA_REG_DEF(reg_name, INV) =                                  \
               mask & ((v) << _TN_BFA_MIN((lower), (upper)));              \
      }), 0                                                                \
                                                                           \
      :((((comm) == TN_BFA_SET)                                            \
      ?({                                                                  \
         _TN_BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
         _TN_BFA_REG_DEF(reg_name, SET) =                                  \
               mask & ((v) << _TN_BFA_MIN((lower), (upper)));              \
      }), 0                                                                \
                                                                           \
      :(((((comm) == TN_BFA_CLR)                                           \
      ?({                                                                  \
         _TN_BFA_COMM_GET(comm) v = __VA_ARGS__+0;                         \
         _TN_BFA_REG_DEF(reg_name, CLR) =                                  \
               mask & ((v) << _TN_BFA_MIN((lower), (upper)));              \
      }), 0                                                                \
                                                                           \
      :({ /* TN_BFA_RD */                                                  \
         ((reg_name) & mask) >> _TN_BFA_MIN((lower), (upper));             \
      })))))));                                                            \
   })



//-- internal kernel macros {{{
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define _TN_BFA_COMM_ERR(a)               _TN_BFA_COMMAND_ERROR_##a
#define _TN_BFA_COMM_GET(a)               _TN_BFA_COMM_ERR(a)

typedef unsigned int _TN_BFA_COMM_GET(TN_BFA_SET);
typedef unsigned int _TN_BFA_COMM_GET(TN_BFA_CLR);
typedef unsigned int _TN_BFA_COMM_GET(TN_BFA_INV);

typedef unsigned int _TN_BFA_COMM_GET(TN_BFA_WR);
typedef unsigned int _TN_BFA_COMM_GET(TN_BFA_RD);

#define _TN_BFA_STRUCT_VAL(a)             a##bits
#define _TN_BFA_FIELD_DEF(reg_name, field_name, def)  \
                                          _##reg_name##_##field_name##_##def
#define _TN_BFA_REG_DEF(reg_name, def)    reg_name##def
#define _TN_BFA_MIN(a, b)                 ((a) < (b) ? (a) : (b))


#endif
// }}}


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _TN_ARCH_PIC24_BFA_H

