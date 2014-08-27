

#ifndef _TN_PORT_H
#define _TN_PORT_H




#if defined(__PIC32MX__)
    #include "pic32/tn_port_pic32.h"
#else
    #error "unknown platform"
#endif




#endif  /* _TN_PORT_H */
