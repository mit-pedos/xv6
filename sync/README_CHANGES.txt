================================================================================
Operating Systems Lab Assignment
Question 2: Process/Thread Synchronization using Counting Semaphores
================================================================================

1. OVERVIEW & DESIGN
--------------------
This question implements a kernel-level counting semaphore subsystem in xv6 to eliminate
race conditions among concurrent threads and coordinate multi-threaded workflows.

Key Components:
1. Kernel Semaphore Subsystem:
   - Structure in kernel:
       struct semaphore {
         struct spinlock lk;
         int value;
         int active;
       };
   - Fixed-size pool of semaphores (MAX_SEM = 16) maintained in kernel space.
   - Uses xv6's native sleep_prepare(&sems[id]), sleep(), and wakeup(&sems[id]) primitives
     tied to the process scheduler to put waiting threads to sleep and awaken them when
     the semaphore is signaled.

2. System Calls Implemented:
   - int sem_init(int sem_id, int value):
     Initializes semaphore sem_id with initial count value and activates it.
   - int sem_wait(int sem_id):
     Decrements the semaphore value. If value was <= 0, sleeps until value becomes positive.
   - int sem_signal(int sem_id):
     Increments the semaphore value and wakes up any threads sleeping on &sems[id].

3. Thread Support (from Question 1):
   - clone() and join() integrated into this directory so threads execute concurrently
     in a shared address space.

4. User Test Program (user/synctest.c):
   - Part 1: Eliminating the Race Condition:
     * 3 concurrent threads each perform 100,000 increments on a shared counter.
     * Each increment is enclosed within sem_wait(SEM_COUNTER) and sem_signal(SEM_COUNTER).
     * Expected vs Actual: Exactly 300,000 reached (0 lost updates).
     * Compares against Question 1's unsynchronized loss.
   - Part 2: Classic Bounded-Buffer Producer-Consumer:
     * Buffer size N = 5.
     * 3 Semaphores:
       - SEM_PC_MUTEX (init 1): Binary semaphore for buffer mutual exclusion.
       - SEM_PC_EMPTY (init 5): Counting semaphore for free slots.
       - SEM_PC_FULL  (init 0): Counting semaphore for filled items.
     * Producer thread produces 15 items; Consumer thread consumes 15 items.
     * Verified: Peak buffer usage strictly <= 5, final count = 0, strict FIFO ordering.

2. FILES MODIFIED / ADDED
-------------------------
- kernel/proc.h:
  * Added int is_thread to struct proc.
- kernel/defs.h:
  * Declared kclone(), kjoin(), seminit(), ksem_init(), ksem_wait(), ksem_signal().
- kernel/main.c:
  * Added seminit() invocation during boot.
- kernel/proc.c:
  * Updated allocproc(), freeproc(), and kwait() to support threads without memory corruption.
  * Implemented uvmshare(), kclone(), and kjoin().
- kernel/sysproc.c:
  * Implemented semaphore data structures and syscall handlers:
    sys_sem_init(), sys_sem_wait(), sys_sem_signal(), sys_clone(), sys_join().
- kernel/syscall.h & kernel/syscall.c:
  * Registered SYS_clone (23), SYS_join (24), SYS_sem_init (25), SYS_sem_wait (26),
    SYS_sem_signal (27).
- user/user.h & user/usys.pl:
  * Exported semaphore and thread syscall prototypes and stubs to user space.
- user/synctest.c:
  * Created test suite covering synchronized thread counters and bounded-buffer producer-consumer.
- Makefile:
  * Added $U/_synctest to UPROGS.
  * Cleared DETFLAGS to prevent compiler errors on paths with spaces.

3. HOW TO BUILD AND RUN
-----------------------
Inside this directory:
    $ make clean
    $ make qemu

Once xv6 boots into the shell, run:
    $ synctest

To exit QEMU:
    Press Ctrl+A, then X.

4. SAMPLE OUTPUT VERIFICATION
-----------------------------
$ synctest
============================================================
      xv6 Process/Thread Synchronization using Semaphores   
============================================================
============================================================
 PART 1: Synchronized Threads Counter Test using Semaphores 
 Threads: 3, Increments: 100000 each, Expected: 300000            
============================================================
Main: Cloned Thread 1 (pid 4)
Main: Cloned Thread 2 (pid 5)
Main: Cloned Thread 3 (pid 6)
Main: Releasing all 3 threads concurrently...
Thread 1 (pid 4) finished 100000 synchronized increments.
Main: Reaped thread with pid 4 via join()
Thread 2 (pid 5) finished 100000 synchronized increments.
Main: Reaped thread with pid 5 via join()
Thread 3 (pid 6) finished 100000 synchronized increments.
Main: Reaped thread with pid 6 via join()
------------------------------------------------------------
Part 1 Results: Expected = 300000, Actual = 300000
SUCCESS: Counter reached EXACTLY 300000 with 0 lost updates!
Semaphores successfully eliminated the race condition.

============================================================
 PART 2: Classic Producer-Consumer Bounded Buffer Test      
 Buffer Size = 5, Total Items = 15                         
 Semaphores: Empty (init 5), Full (init 0), Mutex (init 1)  
============================================================
[Producer (pid 7)]: Produced 1 | Buffer count: 1/5
[Consumer (pid 8)]: Consumed 1 | Buffer count: 0/5
...
[Producer (pid 7)]: Produced 15 | Buffer count: 5/5
[Consumer (pid 8)]: Consumed 15 | Buffer count: 0/5
Consumer: All 15 items verified in strict FIFO order!
------------------------------------------------------------
Part 2 Results: Peak Buffer Usage = 5/5 slots, Final Count = 0
SUCCESS: No buffer overflow or underflow occurred.
Producer-Consumer synchronization fully verified!

============================================================
All synchronization tests completed successfully!
============================================================
