Modified TNKernel-PIC32
==============

A port of [TNKernel real-time system](http://www.tnkernel.com/ "TNKernel"). Tested on PIC32MX.

--------------

#Overview

  * Separate interrupt stack
  * Nested interrupts are supported
  * Shadow register set interrupts are supported
  * Recursive mutexes (optionally)
  * No timer task, so, system tick processing works much faster and takes less RAM

This port is a fork of another PIC32 port by Anders Montonen: [TNKernel-PIC32](https://github.com/andersm/TNKernel-PIC32 "TNKernel-PIC32"). I don't like several design decisions of original TNKernel, as well as some of the implementation details, but Anders wants to keep his port as close to original TNKernel as possible. So I decided to fork it and have fun implementing what I want.

Note that almost all PIC32-dependent routines (such as context switch and so on) are implemented by Anders Montonen; I just examined them in detail and changed a couple of things which I believe should be implemented differently. Anders, great thanks for sharing your job.

Another existing PIC32 port, [the one by Alex Borisov](http://www.tnkernel.com/tn_port_pic24_dsPIC_PIC32.html), also affected my port. In fact, I used to use Alex's port for a long time, but it has several concepts that I don't like, so I had to move eventually. Nevertheless, Alex's port has several nice ideas and solutions, so I didn't hesitate to take what I like from his port. Alex, great thanks to you too.

For a full description of the kernel API, please see the [TNKernel project documentation](http://www.tnkernel.com/tn_description.html "TNKernel project documentation"). Note though that this port has several differences from original TNKernel, they are explained below.

#Wiki contents

  * [Building the project](wiki/building)
  * [PIC32 port implementation details](wiki/pic32_details)
  * [Differences from original TNKernel](wiki/diff_orig_tnkernel)
  * [Differences from the port by Alex Borisov](wiki/diff_alexb_tnkernel)
  * [Why refactor?](wiki/why_refactor)
  * [License](wiki/license)

