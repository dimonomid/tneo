
#ifndef _QUEUE_EXAMPLE_ARCH_H
#define _QUEUE_EXAMPLE_ARCH_H



/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- Include common pic32 header for all examples
#include "../../../common/arch/pic32/example_arch.h"

/**
 * At least, we need to initialize GPIO pins here
 */
void queue_example_arch_init(void);

/**
 * Toggle pins specified by pin_mask
 */
void queue_example_arch_pins_toggle(int pin_mask);

/**
 * Set pins specified by pin_mask
 */
void queue_example_arch_pins_set(int pin_mask);

/**
 * Clear pins specified by pin_mask
 */
void queue_example_arch_pins_clear(int pin_mask);


#endif // _QUEUE_EXAMPLE_ARCH_H

