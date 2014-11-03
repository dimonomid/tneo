/**
 * \file
 *
 * Usage example of connecting event group to other kernel objects.
 * Refer to the readme.txt file for details.
 *
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include <xc.h>
#include "tn.h"
#include "example_queue_eventgrp_conn_arch.h"
#include "task_consumer.h"



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * See comments in the header file
 */
void queue_example_arch_init(void)
{
   //-- set output for needed pins
   TN_BFAR(TN_BFA_CLR, TRISE, 0, 15, 0x03);   

   //-- clear current port value
   TN_BFAR(TN_BFA_WR, LATE, 0, 15, 0x03);   
}

/**
 * See comments in the header file
 */
void queue_example_arch_pins_toggle(int pin_mask)
{
   TN_BFAR(TN_BFA_INV, LATE, 0, 2, pin_mask);
}

/**
 * See comments in the header file
 */
void queue_example_arch_pins_set(int pin_mask)
{
   TN_BFAR(TN_BFA_SET, LATE, 0, 2, pin_mask);
}

/**
 * See comments in the header file
 */
void queue_example_arch_pins_clear(int pin_mask)
{
   TN_BFAR(TN_BFA_CLR, LATE, 0, 2, pin_mask);
}


