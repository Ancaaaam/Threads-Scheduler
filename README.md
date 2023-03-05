# Threads-Scheduler
The threads scheduler exports the following functions:

**INIT** - initializes the scheduler's internal structures.

**FORK** - start a new thread.

**EXEC** - simulates the execution of an instruction.

**WAIT** - waiting for an I/O event/operation.

**SIGNAL** - signals threads waiting for an I/O event/operation.

**END** - destroy the scheduler and free the allocated structures.

In addition to the actual implementation of the above functions, the correct scheduling/execution of threads is ensured according to the **prioritized Round Robin** algorithm . Each thread must run in the context of a real thread in the system. Being a scheduler for uniprocessor systems, only one thread will be able to run at a time.

# Detailed description of how the scheduler works
### Events and I/O
Threads in the system can hang while waiting for an event or an I/O operation. Such an event will be identified by an id (0 - nr_events). The total number of events (that can occur at any one time in the system) will be given as a parameter when initializing the scheduler.

A thread blocks on the function call wait(which receives the id/index of the event/device as a parameter) and is released when another thread calls the function signalon the same event/device. Signal wakes up all threads waiting for a certain event.

Note that a thread that called the function wait, regardless of priority, cannot be scheduled until it is woken up by another thread.

### Execution time:
For execution control, counting the running time of a process is done on every clock interrupt.

Each of the functions shown above represents **only one** instruction that can be executed by a thread at a time.

### Threads States
The states a thread can go through are:

**New** - new thread created following a call of the type fork.

**Ready** - waiting to be planned.

**Running** - scheduled - only one thread can run at a time.

**Waiting** - waits after an event or I/O operation. A thread will hang in the wait state following the call so_wait(event/io).

**Terminated** - completed its execution.

# Detailed instructions implementation

I have implemented 2 structures: _thread_ which contains the characteristics of a thread
and _scheduler_ which describes the scheduler, containg the running thread
at a given time(actual_threads), the priority queue(q), the array that holds
all threads etc. To check if all threads have
finished the execution, I used the semaphore from the scheduler structure, and each thread
has a semaphore to plan the order of their execution.

The implemented functions are the following:

- int so_init(quantum, io)
I check the received parameters, initialize the scheduler and allocate the necessary memory. It takes as arguments the amount of time after which a process must be preempted and the number of events (I/O devices) supported. Returns 0 if the scheduler was initialized successfully, or negative on error.

- void so_end()
The threads are expected to finish, the memory is released and the semaphores are destroyed.

- tid_t so_fork(handler, priority)
Checked parameters. I create the semaphore for the first thread. I create the new thread, I add it
in the thread vector, in the priority queue and then the begin_thread function is executed
At the end of so_fork, the so_exec function is called for scheduling the do_scheduler() semaphore's threads
if there is any thread in the RUN state.

- do_scheduler()
Plans the next thread, as there are several possible cases (see comments in the code).

- begin_thread()
In this function, a thread waits to be scheduled, the semaphore passing to WAIT,
it executes what it received and changes its state to FINISH, achieving what it should (i.e. it works)
Planning is done again after that.

- so_exec()
Simulates the execution of a generic instruction. Basically, it just consumes CPU time.
The quantum is decremented and the semaphore gets in the WAIT state.

- so wait()
The parameters are checked, the current thread is blocked by switching it to WAIT state. and the replanning is done. Returns 0 if the event exists (its id is valid) or negative on error.

- so_signal()
The parameters are also checked for each thread that is in the WAIT state and that has no
required events, goes into the ready state and is placed in the queue.

- new_thread()
A new thread is created with the characteristics given as a parameter and the thread is started with
pthread_create

- press()
A new element is placed in the priority queue

- pop()
The first element in the queue is deleted

- execute_current_thread()
Starts the first element in the queue, placing itself in the actual_threads field of the scheduler
and changing its state to 1, which means READY.
