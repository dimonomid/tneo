
This is an usage example of waiting for messages from multiple queues 
at once in TNeo.

This example is similar to the plain "queue" example (which you might 
want to examine before this one), but in this project, task_consumer
waits for messages from two queues at a time.

To this end, TNeo offers a way to connect the event group to
another kernel object (currently, only to queue). When an event group
with specified flag pattern is connected to the queue, the queue maintains
specified flag pattern automatically: if queue contains is non-empty,
the flag is set, if queue becomes empty, the flag is clear.

Please note that there's no need to clear the flag manually: queue 
maintains it completely.

So in this example project, task_producer creates and starts kernel timer as a
part of its initialization routine. Timer fires after 750 system ticks, and
timer callback sends the message to the second queue and restarts timer again.

task_consumer waits for one of the two flags, each flag indicates whether
appropriate queue is non-empty. When either queue becomes non-empty, flag
is automatically set, so task_consumer wakes up, checks which flag is 
set, receives message from the appropriate queue and handles it.

In this example project, the message from the second queue could just make
task_consumer clear all the LEDs or, conversely, set them all at once.

Event group connection is a perfectly fine solution. If the kernel doesn't
offer a mechanism for that, programmer usually have to use polling services on
these queues and sleep for a few system ticks.  Obviously, this approach has
serious drawbacks: we have a lot of useless context switches, and response for
the message gets much slower. Actually, we lost the main goal of the
preemptive kernel when we use polling services like that.


