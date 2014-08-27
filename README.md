TNKernel
==============

A port of [TNKernel](http://www.tnkernel.com/ "TNKernel"), currently for PIC32 only.

---
PIC32
--------------

This port is a fork of ready-made PIC32 port by Anders Montonen: [TNKernel-PIC32](https://github.com/andersm/TNKernel-PIC32 "TNKernel-PIC32"). I don't like several design decisions of original TNKernel (and port by Anders Montonen, too), as well as some of the implementation details, so I decided to fork it and implement what I want.

For a full description of the kernel API, please see the [TNKernel project documentation](http://www.tnkernel.com/tn_description.html "TNKernel project documentation").

##Building the project
This project is intended to be built as a library, separately from main project (although nothing prevents you from bundle things together, if you want to).

There are various options available which affects API and behavior of TNKernel. But these options are specific for particular project, and aren't related to the TNKernel itself, so we need to keep them separately.

To this end, file `tn.h` includes `tn_cfg.h`, which isn't included in the repository (even more, it is added to `.hgignore` list actually). Instead, default configurate file `tn_cfg_default.h` is provided, and when you just cloned the repository, you might want to copy it as `tn_cfg.h`. Or even better, if your filesystem supports symbolic links, copy it somewhere to your main project's directory (so that you can add it to your VCS there), and create symlink to it named `tn_cfg.h` in the TNKernel directory.


##Context switch
The context switch is implemented using the core software 0 interrupt. It should be configured to use the lowest priority in the system:

    // set up the software interrupt 0 with a priority of 1, subpriority 0
    INTSetVectorPriority(INT_CORE_SOFTWARE_0_VECTOR, INT_PRIORITY_LEVEL_1);
    INTSetVectorSubPriority(INT_CORE_SOFTWARE_0_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_CS0, INT_ENABLED);

The interrupt priority level used by the context switch interrupt should not be configured to use shadow register sets.

**Note:** if tnkernel is built as a separate library, then the file `portable/pic32/tn_int_vec1.S` must be included in the main project itself, in order to dispatch vector1 (core software interrupt 0) correctly.

##Interrupts
TNKernel-PIC32 supports nested interrupts. The kernel provides C-language macros for calling C-language interrupt service routines, which can use either MIPS32 or MIPS16e mode. Both software and shadow register interrupt context saving is supported. Usage is as follows:

    /* Timer 1 interrupt handler using software interrupt context saving */
    tn_soft_isr(_TIMER_1_VECTOR)
    {
       /* here is your ISR code, including clearing of interrupt flag, and so on */
    }

    /* High-priority UART interrupt handler using shadow register set */
    tn_srs_isr(_UART_1_VECTOR)
    {
       /* here is your ISR code, including clearing of interrupt flag, and so on */
    }


Alternatively, the kernel provides assembly-language macros for calling C-language interrupt service routines:

    #include "tn_port_asm.h"
    
    #define _CORE_TIMER_VECTOR 0
    #define _TIMER1_VECTOR     4
    #define _INT_UART_1_VECTOR 24
    
    # Core timer interrupt handler using software interrupt context saving
    tn_soft_isr CoreTimerHandler _CORE_TIMER_VECTOR
    
    # High-priority UART interrupt handler using shadow register set
    tn_srs_isr UartHandler _INT_UART_1_VECTOR

##Interrupt stack
TNKernel-PIC32 uses a separate stack for interrupt handlers. Switching stack pointers is done automatically in the ISR handler wrapper macros. The default size of the interrupt stack in words is set in `tn_port.h`:

    #ifndef TN_INT_STACK_SIZE
    # define  TN_INT_STACK_SIZE        128
    #endif
 
---

The kernel is released under the BSD license as follows:

    TNKernel real-time kernel

    Copyright © 2004, 2013 Yuri Tiomkin
    PIC32 version modifications copyright © 2013 Anders Montonen
    Various API modifications and refactoring copyright © 2014 Dmitry Frank
    All rights reserved.

    Permission to use, copy, modify, and distribute this software in source
    and binary forms and its documentation for any purpose and without fee
    is hereby granted, provided that the above copyright notice appear
    in all copies and that both that copyright notice and this permission
    notice appear in supporting documentation.

    THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
