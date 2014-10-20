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
#include "queue_example_arch.h"
#include "task_consumer.h"



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


//-- gpio register to work with, you might want to switch it to different port
//   NOTE: in real programs, it's better to use some peripheral library.
//   I strongly dislike plib as well as "brand new" Harmony by Microchip,
//   I use my own peripheral library, but for example such as this one,
//   I decided to write it as simple as possible.
#define  TRIS_REG          TRISE
#define  TRIS_REG_CLR      TRISECLR
#define  TRIS_REG_SET      TRISESET
#define  TRIS_REG_INV      TRISEINV

#define  PORT_REG          PORTE
#define  PORT_REG_CLR      PORTECLR
#define  PORT_REG_SET      PORTESET
#define  PORT_REG_INV      PORTEINV


#define TASK_CONS_PIN_MASK (0             \
      | (1 << TASK_CONS_PIN__0)           \
      | (1 << TASK_CONS_PIN__1)           \
      | (1 << TASK_CONS_PIN__2)           \
      )




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
   TRIS_REG_CLR = TASK_CONS_PIN_MASK;

   //-- clear current port value
   PORT_REG_CLR = TASK_CONS_PIN_MASK;
}

/**
 * See comments in the header file
 */
void queue_example_arch_pins_toggle(int pin_mask)
{
   PORT_REG_INV = pin_mask;
}


