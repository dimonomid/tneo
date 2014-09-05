Modified TNKernel-PIC32
==============

A port of [TNKernel real-time system](http://www.tnkernel.com/ "TNKernel"). Tested on PIC32MX.

--------------

#Overview

  * Separate stack for interrupts
  * Nested interrupts are supported
  * Shadow register set interrupts are supported
  * Recursive mutexes (optionally)
  * No timer task, so, system tick processing works much faster and takes less RAM

This port is a fork of another PIC32 port by Anders Montonen: [TNKernel-PIC32](https://github.com/andersm/TNKernel-PIC32 "TNKernel-PIC32"). I don't like several design decisions of original TNKernel, as well as some of the implementation details, but Anders wants to keep his port as close to original TNKernel as possible. So I decided to fork it and have fun implementing what I want.

The more I get into how TNKernel works, the less I like its code. There is a lot of code duplication and a lot of inconsistency, all this leads to bugs. So I decided to rewrite it almost completely, and that's what I'm doing now.

Together with almost totally re-writting TNKernel, I'm implementing detailed unit tests for it, to make sure I didn't break anything, and of course I've found several bugs in original TNKernel. Several of them:

  * If low-priority `task_low` locks mutex M1 with priority inheritance, high-priority `task_high` tries to lock mutex M1 and gets blocked -> `task_low`'s priority elevates to `task_high`'s priority; then `task_high` stops waiting for mutex by timeout -> priority of `task_low` remains elevated. The same happens if `task_high` is terminated by `tn_task_terminate()`;
  * `tn_mutex_delete()` : `mutex->holder` is checked against `tn_curr_run_task` without disabling interrupts;
  * `tn_mutex_delete()` : if mutex is not locked, `TERR_ILUSE` is returned. I believe task should be able to delete non-locked mutex;
  * if task that waits for mutex is in `WAITING_SUSPENDED` state, and mutex is deleted, `TERR_NO_ERR` is returned after returning from `SUSPENDED` state, instead of `TERR_DLT`
  * `tn_sys_tclice_ticks()` : if wrong param is given, `TERR_WRONG_PARAM` is returned and interrupts remain disabled.

I'm rewriting it in centralized and consistent way, and test it carefully by unit tests, which are, of course, must-have for project like this.

Note that PIC32-dependent routines (such as context switch and so on) are originally implemented by Anders Montonen; I just examined them in detail and changed a couple of things which I believe should be implemented differently. Anders, great thanks for sharing your job.

Another existing PIC32 port, [the one by Alex Borisov](http://www.tnkernel.com/tn_port_pic24_dsPIC_PIC32.html), also affected my port. In fact, I used to use Alex's port for a long time, but it has several concepts that I don't like, so I had to move eventually. Nevertheless, Alex's port has several nice ideas and solutions, so I didn't hesitate to take what I like from his port. Alex, great thanks to you too.

For a full description of the kernel API, please see the [TNKernel project documentation](http://www.tnkernel.com/tn_description.html "TNKernel project documentation"). Note though that this port has several differences from original TNKernel, they are explained below.

#Wiki contents

  * [Building the project](/dfrank/tnkernel/wiki/building)
  * [PIC32 port implementation details](/dfrank/tnkernel/wiki/pic32_details)
  * [Differences from original TNKernel](/dfrank/tnkernel/wiki/diff_orig_tnkernel)
  * [Differences from the port by Alex Borisov](/dfrank/tnkernel/wiki/diff_alexb_tnkernel)
  * [Why refactor?](/dfrank/tnkernel/wiki/why_refactor)
  * [License](/dfrank/tnkernel/wiki/license)

