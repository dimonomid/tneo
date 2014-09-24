
/**
 * \file
 *
 * Architecture-dependent routines.
 */

#ifndef _TN_ARCH_H
#define _TN_ARCH_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_common.h"


#ifdef __cplusplus
extern "C"  {  /*}*/
#endif

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


/**
 * Unconditionally disable interrupts
 */
void tn_arch_int_dis(void);

/**
 * Unconditionally enable interrupts
 */
void tn_arch_int_en(void);

/**
 * Save current status register value and disable interrupts atomically
 *
 * @see `tn_arch_sr_restore()`
 */
unsigned int tn_arch_sr_save_int_dis(void);

/**
 * Restore previously saved status register
 *
 * @param sr   status register value previously from
 *             `tn_arch_sr_save_int_dis()`
 *
 * @see `tn_arch_sr_save_int_dis()`
 */
void tn_arch_sr_restore(unsigned int sr);





#ifdef __cplusplus
}  /* extern "C" */
#endif


/*******************************************************************************
 *    ACTUAL PORT IMPLEMENTATION
 ******************************************************************************/

#if defined(__PIC32MX__)
    #include "pic32/tn_arch_pic32.h"
#else
    #error "unknown platform"
#endif




#endif  /* _TN_ARCH_H */

