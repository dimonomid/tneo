/**
 * \file
 *
 * Data queue usage example.
 *
 * For general information about the pattern, refer to the top of
 * queue_example.h file
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include <xc.h>
#include "tn.h"
#include "queue_example_arch.h"
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
   TN_BFA(TN_BFA_WR, TRISE, TRISE0, 0);   
   TN_BFA(TN_BFA_WR, TRISE, TRISE1, 0);   
   TN_BFA(TN_BFA_WR, TRISE, TRISE2, 0);   

   //-- clear current port value
   TN_BFA(TN_BFA_WR, LATE, LATE0, 0);   
   TN_BFA(TN_BFA_WR, LATE, LATE1, 0);   
   TN_BFA(TN_BFA_WR, LATE, LATE2, 0);   
}

/**
 * See comments in the header file
 */
void queue_example_arch_pins_toggle(int pin_mask)
{
   TN_BFAR(TN_BFA_INV, LATE, 0, 2, pin_mask);
}


