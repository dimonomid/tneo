

#ifndef _TN_PORT_H
#define _TN_PORT_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//--- Port (NOTE: tn_cfg.h must be included before including tn_port.h,
//    and all configuration constants should be defined as well)
#include "tn_common.h"




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

unsigned int *tn_stack_init(void * task_func,
                            void * stack_start,
                            void * param);

void tn_cpu_int_enable(void);
void tn_cpu_int_disable(void);
int  tn_inside_int(void);





/*******************************************************************************
 *    ACTUAL PORT IMPLEMENTATION
 ******************************************************************************/

#if defined(__PIC32MX__)
    #include "pic32/tn_port_pic32.h"
#else
    #error "unknown platform"
#endif




#endif  /* _TN_PORT_H */
