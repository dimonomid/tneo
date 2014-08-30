Modified TNKernel-PIC32
==============

A port of [TNKernel real-time system](http://www.tnkernel.com/ "TNKernel"). Tested on PIC32MX.

--------------

This port is a fork of another PIC32 port by Anders Montonen: [TNKernel-PIC32](https://github.com/andersm/TNKernel-PIC32 "TNKernel-PIC32"). I don't like several design decisions of original TNKernel, as well as some of the implementation details, but Anders wants to keep his port as close to original TNKernel as possible. So I decided to fork it and have fun implementing what I want.

Note that almost all PIC32-dependent routines (such as context switch and so on) are implemented by Anders Montonen; I just examined them in detail and changed a couple of things which I believe should be implemented differently. Anders, great thanks for sharing your job.

Another existing PIC32 port, [the one by Alex Borisov](http://www.tnkernel.com/tn_port_pic24_dsPIC_PIC32.html), also affected my port. In fact, I used to use Alex's port for a long time, but it has several concepts that I don't like, so I had to move eventually. Nevertheless, Alex's port has several nice ideas and solutions, so I didn't hesitate to take what I like from his port. Alex, great thanks to you too.

For a full description of the kernel API, please see the [TNKernel project documentation](http://www.tnkernel.com/tn_description.html "TNKernel project documentation"). Note though that this port has several differences from original TNKernel, they are explained below.

##Building the project
###Configuration file
This project is intended to be built as a library, separately from main project (although nothing prevents you from bundling things together, if you want to).

There are various options available which affects API and behavior of TNKernel. But these options are specific for particular project, and aren't related to the TNKernel itself, so we need to keep them separately.

To this end, file `tn.h` includes `tn_cfg.h`, which isn't included in the repository (even more, it is added to `.hgignore` list actually). Instead, default configuration file `tn_cfg_default.h` is provided, and when you just cloned the repository, you might want to copy it as `tn_cfg.h`. Or even better, if your filesystem supports symbolic links, copy it somewhere to your main project's directory (so that you can add it to your VCS there), and create symlink to it named `tn_cfg.h` in the TNKernel directory, like this:

```bash
$ cd /path/to/tnkernel/core
$ cp ./tn_cfg_default.h /path/to/main/project/lib_cfg/tn_cfg.h
$ ln -s /path/to/main/project/lib_cfg/tn_cfg.h ./tn_cfg.h
```
Default configuration file contains detailed comments, so you can read them and configure behavior as you like.

###MPLABX project

MPLABX project resides in the `portable/pic32/tnkernel_df.X` directory. This is a "*library project*" in terms of MPLABX, so if you use MPLABX you can easily add it to your main project by right-clicking `Libraries -> Add Library Project ...`. Alternatively, of course you can just build it and use resulting `tnkernel_df.X.a` file in whatever way you like.

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
**Note**: every ISR in your system should use kernel-provided macro, either `tn_soft_isr` or `tn_srs_isr`, instead of `__ISR` macro.

###Interrupt stack
TNKernel-PIC32 uses a separate stack for interrupt handlers. Switching stack pointers is done automatically in the ISR handler wrapper macros. User should allocate array for interrupt stack and pass it as an argument to `tn_start_system()` (see details below).


##Differences from original TNKernel

###No timer task (by default)
Yeah, timer task's job is important: it manages `tn_wait_timeout_list`, i.e. it wakes up tasks whose timeout is expired. But it's actually better to do it right in `tn_tick_int_processing()` that is called from timer ISR, because presence of the special task provides significant overhead.
Look at what happens when timer interrupt is fired (assume we don't use shadow register set for that, which is almost always the case):

  * Current context (23 words) is saved to the interrupt stack;
  * ISR called: particularly, `tn_tick_int_processing()` is called;
  * `tn_tick_int_processing()` disables interrupts, manages round-robin (if needed), then it wakes up `tn_timer_task`, sets `tn_next_task_to_run`, and enables interrupts back;
  * `tn_tick_int_processing()` finishes, so ISR macro checks that `tn_next_task_to_run` is different from `tn_curr_run_task`, and sets `CS0` interrupt bit, so that context should be switched as soon as possible;
  * Context (23 words) gets restored to whatever task we interrupted;
  * `CS0` ISR is immediately called, so full context (32 words) gets saved on task's stack, and context of `tn_timer_task` is restored;
  * `tn_timer_task` disables interrupts, performs its not so big job (manages `tn_wait_timeout_list`), puts itself to wait, enables interrupts and pends context switching again;
  * `CS0` ISR is immediately called, so full context of `tn_timer_task` gets saved in its stack, and then, after all, context of my own interrupted task gets restored and my task continues to run.

I've measured with MPLABX's stopwatch how much time it takes: with just three tasks (idle task, timer task, my own task with priority 6), i.e. without any sleeping tasks, all this routine takes **682 cycles**.
So I tried to get rid of `tn_timer_task` and perform its job right in the `tn_tick_int_processing()`. When system starts, application callback is therefore called from idle task. Now, the following steps are performed when timer interrupt is fired:

  * Current context (23 words) is saved to the interrupt stack;
  * ISR called: particularly, `tn_tick_int_processing()` is called;
  * `tn_tick_int_processing()` disables interrupts, manages round-robin (if needed), manages `tn_wait_timeout_list`, and enables interrupts back;
  * `tn_tick_int_processing()` finishes, ISR macro checks that `tn_next_task_to_run` is the same as `tn_curr_run_task`
  * Context (23 words) gets restored to whatever task we interrupted;

That's all. It takes **251 cycles**: 2.7 times less.

Well, timer task could make sense if we don't use separate interrupt stack: then it might be needed to save stack RAM, because `task_wait_complete()` gets called, which calls other functions as well. So, for the case this TNKernel implementation will be ported to another platform without ability to use separate interrupt task, I just left an option for that: `TN_USE_TIMER_TASK`, which default value is, of course, `0`.

But if we have separate interrupt stack, it's worth to get rid of timer task, and just make sure that interrupt stack size is enough for this job. As a result, RAM is saved (since you don't need to allocate stack for timer task) and things work much faster.

Well, to be honest, I don't think that anyone will ever need an option to use timer task, so I'll probably remove it eventually. But let it be for now.

###System start
Original TNKernel code designed to be built together with main project only, there's no way to build as a separate library: at least, arrays for idle-task stacks are allocated statically, so size of them is defined at tnkernel compile time.

It's much better if we could pass these things to tnkernel at runtime, so, `tn_start_system()` now has the following prototype:

```C
void tn_start_system(
      unsigned int  *idle_task_stack,        //-- pointer to array for idle task stack
      unsigned int   idle_task_stack_size,   //-- size of idle task stack
      unsigned int  *int_stack,              //-- pointer to array for interrupt stack
      unsigned int   int_stack_size,         //-- size of interrupt stack
      void          (*app_in_cb)(void),      //-- callback function used for setup user tasks etc.
      void          (*idle_user_cb)(void)    //-- callback function repeatedly called from idle task
      );
```
Besides stack for idle task, there is interrupt stack: it should be also provided. If the option `TN_USE_TIMER_TASK` is set to `1`, there are two additional arguments for timer task's stack pointer and size.

The port by Alex Borisov has similar prototype of `tn_start_system()` (a bit different though).

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
Sometimes I feel lack of mutexes that allow recursive locking. Yeah I know there are developers who believe that recursive locking leads to the code of lower quality, and I understand it. Even Linux kernel doesn't have recursive mutexes.

Sometimes they are really useful though (say, if you want to use some third-party library that requires locking primitives to be recursive), so I decided to implement an option for that: `TN_MUTEX_REC`. If it is non-zero, mutexes allow recursive locking; otherwise you get `TERR_ILUSE` when you try to lock mutex that is already locked by this task. Default value: `0`.

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

  * `TN_API_TASK_CREATE__NATIVE` - default value, for behavior of original TNKernel;

  * `TN_API_TASK_CREATE__CONVENIENT` - for behavior of port by Alex.

###Macro `MAKE_ALIG()`

There is a terrible mess with `MAKE_ALIG()` macro: TNKernel docs specify that the argument of it should be the size to align, but almost all ports, including original one, defined it so that it takes type, not size.
                                                                       
But the port by AlexB implemented it differently (i.e. accordingly to the docs) : it takes size as an argument.

When I was moving from the port by AlexB to another one, do you have any idea how much time it took me to figure out why do I have rare weird bug? :)

So, there is an option `TN_API_MAKE_ALIG_ARG` with two possible values;

  * `TN_API_MAKE_ALIG_ARG__SIZE` - use macro like this: `MAKE_ALIG(sizeof(struct my_struct))`, like in the port by Alex.

  * `TN_API_MAKE_ALIG_ARG__TYPE` - default value, use macro like this: `MAKE_ALIG(struct my_struct)`, like in any other port.

By the way, I wrote to the author of TNKernel (Yuri Tiomkin) about this mess, but he didn't answer anything. It's a pity of course, but we have what we have.

##Differences from the port by Alex Borisov

  * Nested interrupts are supported, so, no need to make all interrupts to have the same priority
  * There are no "*system interrupts*" (the ones that call some system services) or "*user interrupts*" (the ones that don't) : every interrupt should use kernel-provided macro, either `tn_soft_isr` or `tn_srs_isr`, instead of `__ISR` macro;
  * There is separate stack for interrupts. This has a bunch of consequenses:
    1. Less RAM usage: there's no need to add interrupt's stack size to every task's stack. Say, if your interrupt needs at most 64 words of stack, and you have 5 tasks, then in the Alex's port you should add 64 words to each tasks's stack: total 320 words. In the port by andersm, 64 words are allocated just once;
    1. Impossible to have the dreaded situation when interrupt caused full context (34 words) to be saved on the task's stack twice (that happens on Alex's port if system interrupt happened while `tn_switch_context` is saving/restoring context);
    1. When interrupt happens, there's no need to store all 32 words (well, actually Alex's port saves even 34 words) to the stack, because callee-saved registers `$s0-$s9` are saved automatically by compiler. Only 23 words are saved to the interrupt stack (one additional word is `SRSCtl` register);
    1. It's possible to implement interrupts that use shadow register set. In this case, only 5 registers are saved to the interrupt stack: `EPC`, `SRSCtl`, `Status`, `hi`, `lo`. This works faster and takes even less memory.
    1. If some low-priority task is preempted by interrupt, and ISR wakes up some high-priority task, context is switched to new high-priority task about 1.5 times slower.
  * Core software interrupt 0 is used for context switch routine. You can't use this interrupt in your project;
  * There is an option for recursive mutexes;
  * There are no `tn_sys_enter_critical()` / `tn_sys_exit_critical()` functions. You can use `tn_cpu_int_disable()` / `tn_cpu_int_enable()` instead, but keep in mind they aren't totally equivalent: Alex added special "*critical section context*" to its port, and it refuses to enter critical section from non-task context.
  * Slightly different API of `tn_start_system()`: at least we should provide interrupt stack which isn't present in Alex's port at all. One more thing: in Alex's port, `tn_start_system()` takes two callback functions `app_in_cb` and `cpu_int_en` that are actually called from timer task one-by-one, immediately. I believe there's no need to keep two callback functions, the single `app_in_cb` is fairly enough.
  * Latest (for now) TNKernel 2.7; at least, Alex's port has buggy ceiling mutexes. In 2.7, these issues are fixed.

So, to summarize:
###Cons
  * Context switch is slower when you wake up high-priority task from ISR;
  * It uses core software interrupt 0, so you can't use it in your project.

###Pros
  * Context switch is faster if ISR doesn't wake up new task. If you use shadow register set, even more faster;
  * It uses less RAM, which is always insufficient in the embedded world;
  * It is safer (because context is saved to the task's stack exactly once, no way for tnkernel to save it several times);
  * It allows you to use nested interrupts;
  * It allows you to use recursive mutexes, if you want;
  * Latest TNKernel 2.7: at least, ceiling mutexes are fixed.

##Why refactor?
I'm not a huge fan of refactoring, but sometimes I **really** want to rewrite some part of code. The most common example that happens across all TNKernel sources is code like the following:
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
The code above is simplified; in the real TNKernel code things are usually more complicated.

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
