/*******************************************************************************
 *   Description:   TODO
 *
 ******************************************************************************/

#ifndef _EXAMPLE_ARCH_H
#define _EXAMPLE_ARCH_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

//-- instruction that causes debugger to halt
#define SOFTWARE_BREAK()  {__asm__ volatile(".pword 0xDA4000"); __asm__ volatile ("nop");}


/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#endif // _EXAMPLE_ARCH_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


