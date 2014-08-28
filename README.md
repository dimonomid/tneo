TNKernel
==============

A port of [TNKernel](http://www.tnkernel.com/ "TNKernel"), currently for PIC32 only.

---
PIC32
--------------

This port is a fork of ready-made PIC32 port by Anders Montonen: [TNKernel-PIC32](https://github.com/andersm/TNKernel-PIC32 "TNKernel-PIC32"). I don't like several design decisions of original TNKernel, as well as some of the implementation details, but Anders wants to keep his port as close to original TNKernel as possible. So I decided to fork it and have fun implementing what I want.

Note that almost all PIC32-dependent routines (such as context switch and so on) are implemented by Anders Montonen; I just examined them in detail and changed a couple of things which I believe should be implemented differently. Anders, great thanks for sharing your job.

Another existing PIC32 port, [the one by Alex Borisov](http://www.tnkernel.com/tn_port_pic24_dsPIC_PIC32.html), definitely also affected my job. In fact, I used to use Alex's port for a long time, but it has several concepts that I don't like, so I had to move. Nevertheless, Alex's port has several nice ideas and solutions, so I didn't hesitate to take what I like from his port. Alex, great thanks for you too.

For a full description of the kernel API, please see the [TNKernel project documentation](http://www.tnkernel.com/tn_description.html "TNKernel project documentation"). Note though that this port has several differences from original TNKernel, they are explained below.

##Building the project
This project is intended to be built as a library, separately from main project (although nothing prevents you from bundle things together, if you want to).

There are various options available which affects API and behavior of TNKernel. But these options are specific for particular project, and aren't related to the TNKernel itself, so we need to keep them separately.

To this end, file `tn.h` includes `tn_cfg.h`, which isn't included in the repository (even more, it is added to `.hgignore` list actually). Instead, default configurate file `tn_cfg_default.h` is provided, and when you just cloned the repository, you might want to copy it as `tn_cfg.h`. Or even better, if your filesystem supports symbolic links, copy it somewhere to your main project's directory (so that you can add it to your VCS there), and create symlink to it named `tn_cfg.h` in the TNKernel directory, like this:

```bash
$ cd /path/to/tnkernel/core
$ cp ./tn_cfg_default.h /path/to/main/project/lib_cfg/tn_cfg.h
$ ln -s /path/to/main/project/lib_cfg/tn_cfg.h ./tn_cfg.h
```
Default configuration file contains detailed comments, so you can read them and configure behavior as you like.

##PIC32 port implementation details
###Context switch
The context switch is implemented using the core software 0 interrupt. It should be configured to use the lowest priority in the system:

```C
// set up the software interrupt 0 with a priority of 1, subpriority 0
INTSetVectorPriority(INT_CORE_SOFTWARE_0_VECTOR, INT_PRIORITY_LEVEL_1);
INTSetVectorSubPriority(INT_CORE_SOFTWARE_0_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
INTEnable(INT_CS0, INT_ENABLED);
```

The interrupt priority level used by the context switch interrupt should not be configured to use shadow register sets.

**Note:** if tnkernel is built as a separate library, then the file `portable/pic32/tn_port_pic32_int_vec1.S` must be included in the main project itself, in order to dispatch vector1 (core software interrupt 0) correctly.

###Interrupts
TNKernel-PIC32 supports nested interrupts. The kernel provides C-language macros for calling C-language interrupt service routines, which can use either MIPS32 or MIPS16e mode. Both software and shadow register interrupt context saving is supported. Usage is as follows:

```C
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
```
Every ISR in your system should use either `tn_soft_isr` or `tn_srs_isr` macro, instead of `__ISR` macro.

###Interrupt stack
TNKernel-PIC32 uses a separate stack for interrupt handlers. Switching stack pointers is done automatically in the ISR handler wrapper macros. User should allocate array for interrupt stack and pass it as an argument to `tn_start_system()`.


##Differences from original TNKernel

###System start
Original TNKernel code designed to be built together with main project only, there's no way to build as a separate library: at least, arrays for timer-task and idle-task stacks are allocated statically, so size of them is defined at tnkernel compile time.

It's much better if we could pass these things to tnkernel at runtime, so, `tn_start_system()` now has the following prototype:

```C
void tn_start_system(
      unsigned int  *timer_task_stack,       //-- pointer to array for timer task stack
      unsigned int   timer_task_stack_size,  //-- size of timer task stack
      unsigned int  *idle_task_stack,        //-- pointer to array for idle task stack
      unsigned int   idle_task_stack_size,   //-- size of idle task stack
      unsigned int  *int_stack,              //-- pointer to array for interrupt stack
      unsigned int   int_stack_size,         //-- size of interrupt stack
      void          (*app_in_cb)(void),      //-- callback function used for setup user tasks etc.
      void          (*cpu_int_en)(void),     //-- callback function used to enable interrupts
      void          (*idle_user_cb)(void)    //-- callback function repeatedly called from idle task
      );
```
Besides stacks for idle and timer tasks, there is interrupt stack: it should be also provided.

The port by Alex Borisov has `tn_start_system()` with similar prototype (a bit different though).

###System time
Added two functions for getting and setting system tick count:

```C
unsigned int  tn_sys_time_get  (void);
void          tn_sys_time_set  (unsigned int value);
```

The port by Alex Borisov also contains them.

###Mutex lock polling
In original TNKernel, `tn_mutex_lock()` returned `TERR_WRONG_PARAM` if `timeout` is 0, but in my opinion this function should behave exactly like `tn_mutex_lock_polling()` in the case of zero timeout. I've made this change. (Well in fact, I've rewritten mutex locking implementation completely, so that now there's just one "worker" function that does the job, and `tn_mutex_lock_polling()` is just a kind of wrapper that always provides zero timeout)

###Option for recursive mutexes
Sometimes I feel lack of mutexes that allow recursive locking. Yeah I know there are developers who believe that recursive locking leads to the code of lower quality, and I understand it. Even mutexes in the Linux kernel aren't recursive. Sometimes they are really useful though, so I decided to implement it optionally, see option `TN_MUTEX_REC` below.

###Task creation API
In original TNKernel, one should give bottom address of the task stack to `tn_task_create()`, like this: 

```C
#define MY_STACK_SIZE   0x100
static unsigned int my_stack[ MY_STACK_SIZE ];

tn_task_create(/* ... several arguments omitted ... */
               &(my_stack[ MY_STACK_SIZE - 1]),
               /* ... several arguments omitted ... */);
```
Alex Borisov implemented it more conveniently: one should give just array address, like this:
```C
tn_task_create(/* ... several arguments omitted ... */
               my_stack,
               /* ... several arguments omitted ... */);
```
This port has an option `TN_API_TASK_CREATE` with two possible values:

  * `TN_API_TASK_CREATE__NATIVE` - for behavior of original TNKernel;

  * `TN_API_TASK_CREATE__CONVENIENT` - for behavior of port by Alex.

###Macro `MAKE_ALIG()`

There is a terrible mess with `MAKE_ALIG()` macro: TNKernel docs specify that the argument of it should be the size to align, but almost all ports, including original one, defined it so that it takes type, not size.
                                                                       
But the port by AlexB implemented it differently (i.e. accordingly to the docs) : it takes size as an argument.

When I was moving from the port by AlexB to another one, do you have any idea how much time it took me to figure out why do I have rare weird bug? :)

So, there is an option `TN_API_MAKE_ALIG_ARG` with two possible values;

  * `TN_API_MAKE_ALIG_ARG__SIZE` - use macro like this: `MAKE_ALIG(sizeof(struct my_struct))`, like in the port by Alex.

  * `TN_API_MAKE_ALIG_ARG__TYPE` - use macro like this: `MAKE_ALIG(struct my_struct)`, like in any other port.

By the way, I wrote to the author of TNKernel (Yuri Tiomkin) about this mess, but he didn't answer anything. It's a pity of course, but we have what we have.

##Why refactor?
I'm not a huge fan of refactoring, but sometimes I **really** want to rewrite some part of code which I believe is not currently perfect.

I really don't like code like this:
```C
int my_function(void)
{
   tn_disable_interrupt();
   //-- do something

   if (error()){
      //-- do something

      tn_enable_interrupt();
      return ERROR;
   }
   //-- do something

   tn_enable_interrupt();
   return SUCCESS;
}
```

If you have multiple `return` statements or, even more, if you have to perform some action before return (`tn_enable_interrupt()` in the example above), it's great job for `goto`:

```C
int my_function(void)
{
   int ret = SUCCESS;
   tn_disable_interrupt();
   //-- do something

   if (error()){
      //-- do something
      ret = ERROR;
      goto out;
   }
   //-- do something

out:
   tn_enable_interrupt();
   return ret;
}
```
I understand there are a lot of people that don't agree with me on this (mostly because they believe `goto` is unconditionally evil), but anyway I decided to explain it. And yeah, original TNKernel code contains zero `goto` statement occurences, but a lot of code like the one in first example above. So, if I look at the code and I want to rewrite it, I don't deny myself the pleasure.

---

The kernel is released under the BSD license as follows:

```
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
```
