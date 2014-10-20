
This is an usage example of queues in TNeoKernel.

When you need to build a queue of some data objects from one task to
another, the common pattern in the TNeoKernel is to use fixed memory pool
and queue together. Number of elements in the memory pool is equal to the
number of elements in the queue. So when you need to send the message,
you get block from the memory pool, fill it with data, and send pointer
to it by means of the queue.

When you receive the message, you handle the data, and release the memory
back to the memory pool.

This example project demonstrates this technique: there are two tasks:

- producer task (which sends messages to the consumer)
- consumer task (which receives messages from the producer)

Actual functionality is, of course, very simple: each message contains 
the following:

- command to perform: currently there is just a single command: toggle pin
- pin number: obviously, number of the pin for which command should
  be performed.

For the message structure, refer to `struct TaskConsumerMsg` in the 
file `task_consumer.c`. Note, however, that it is a "private" structure:
producer doesn't touch it directly; instead, it uses special function:
`task_consumer_msg_send()`, which sends message to the queue.

So the producer just sends new message each 100 ms, specifying
one of three pins. Consumer, consequently, toggles the pin specified
in the message.

Before we can get to the business, we need to initialize things.
How it is done:

- `init_task_create()`, which is called from the `tn_sys_start()`, 
  creates first application task: the producer task.
- producer task gets executed and first of all it initializes the
  essential application stuff, by calling `appl_init()`:

  - `queue_example_init()` is called, in which common application
    objects are created: in this project, there's just a single 
    object: event group that is used to synchronize task
    initialization. See `enum E_QueExampleFlag`.
  - `queue_example_arch_init()` is called, in which architecture
    dependent things are initialized: at least, we need to setup
    some GPIO pins so that consumer could use them.
  - create consumer task
  - wait until it is initialized (i.e. wait for the flag 
    QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT in the event group returned 
    by `queue_example_eventgrp_get()`)

- consumer task gets executed
  - gpio is configured: we need to set up certain pins to output
    as well as initially clear them
  - consumer initializes its own objects:
    - fixed memory pool
    - queue
  - sets the flag QUE_EXAMPLE_FLAG__TASK_CONSUMER_INIT
  - enters endless loop in which it will receive and 
    handle messages. Now, it is going to wait for first message.

- producer task gets executed and enters its endless loop in which
  it will send messages to the consumer.

At this point, everything is initialized.

