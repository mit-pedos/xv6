================================================================================
Operating Systems Lab Assignment - 5
Question 2: Producer-Consumer Problem with Bounded Buffer using Semaphores
================================================================================

1. OVERVIEW & DESIGN
--------------------
This question implements the classic Bounded-Buffer Producer-Consumer synchronization
problem using kernel-level counting semaphores and a shared memory circular buffer in xv6.

Key Components:
- Shared Memory:
  Mapped at virtual address SHM_VA = (TRAPFRAME - PGSIZE) = 0x3FFFFFFE000.
  A shared physical page is allocated upon first call to shm_get() and mapped with
  PTE_R | PTE_W | PTE_U. It is automatically propagated to child processes during fork()
  and unmapped (without deallocating the physical frame) during process exit.
  The shared memory holds a circular buffer:
    - buffer[5]: 5-element integer array (buffer capacity N = 5).
    - in, out: read and write circular indices.
    - count: current number of items in the buffer.
    - max_observed, min_observed: statistics tracking buffer boundaries.

- Kernel Semaphore Subsystem:
  A table of 32 counting semaphores (struct semaphore) maintained in the kernel:
    - value: current semaphore count.
    - lock: spinlock protecting the semaphore state.
    - active: flag indicating whether the semaphore is in use.
  Integrated with xv6's process scheduler via sleep_prepare(chan), sleep(), and wakeup(chan).

- Synchronization Logic:
  Uses three semaphores:
  1. SEM_MUTEX (ID 0): Binary semaphore initialized to 1 for mutual exclusion on buffer access.
  2. SEM_EMPTY (ID 1): Counting semaphore initialized to BUFFER_SIZE (5) tracking empty slots.
  3. SEM_FULL  (ID 2): Counting semaphore initialized to 0 tracking filled slots.

- User Program (user/prodcons.c):
  - Spawns a producer process and a consumer process via fork().
  - Producer:
    * Waits for SEM_EMPTY, then SEM_MUTEX.
    * Places item into buffer[in], updates in = (in + 1) % 5, increments count.
    * Releases SEM_MUTEX, then signals SEM_FULL.
    * Blocks and prints when the buffer reaches capacity (5/5).
  - Consumer:
    * Waits for SEM_FULL, then SEM_MUTEX.
    * Retrieves item from buffer[out], updates out = (out + 1) % 5, decrements count.
    * Releases SEM_MUTEX, then signals SEM_EMPTY.
    * Verifies that items are received in strict ascending FIFO order (1..20).
  - Parent waits for both processes to terminate and prints verification metrics.

2. FILES MODIFIED / ADDED
-------------------------
- kernel/memlayout.h:
  * Defined SHM_VA as (TRAPFRAME - PGSIZE).
- kernel/defs.h:
  * Added declarations for walk(), shminit(), shm_get(), seminit(), ksem_init(),
    ksem_wait(), and ksem_post().
- kernel/main.c:
  * Added calls to shminit() and seminit() during kernel initialization.
- kernel/proc.c:
  * Updated proc_freepagetable() to unmap SHM_VA without freeing the physical page.
  * Updated kfork() to map SHM_VA into child process pagetable if mapped in parent.
- kernel/sysproc.c:
  * Implemented shminit(), shm_get(), sys_shm_get().
  * Implemented counting semaphore subsystem: seminit(), ksem_init(), ksem_wait(),
    ksem_post(), and syscall handlers sys_sem_init(), sys_sem_wait(), sys_sem_post().
- kernel/syscall.h & kernel/syscall.c:
  * Added SYS_shm_get (23), SYS_sem_init (24), SYS_sem_wait (25), SYS_sem_post (26).
- user/user.h & user/usys.pl:
  * Added user-space prototypes and system call stubs for shm_get(), sem_init(),
    sem_wait(), and sem_post().
- user/prodcons.c:
  * Created test program for bounded-buffer producer-consumer synchronization.
- Makefile:
  * Added $U/_prodcons to UPROGS.
  * Cleared DETFLAGS to prevent compiler errors on paths with spaces.
  * Set CPUS := 1.

3. HOW TO BUILD AND RUN
-----------------------
Inside this directory:
    $ make clean
    $ make qemu

Once xv6 boots into the shell, run:
    $ prodcons

To exit QEMU:
    Press Ctrl+A, then X.

4. SAMPLE OUTPUT VERIFICATION
-----------------------------
$ prodcons
============================================================
Producer-Consumer Bounded Buffer Simulation with Semaphores
Buffer capacity = 5, Total items = 20
============================================================
Producer started (pid 4)
Producer [pid 4]: Produced item 1 | Buffer count: 1/5
Consumer started (pid 5)
Consumer [pid 5]: Consumed item 1 | Buffer count: 0/5
Producer [pid 4]: Produced item 2 | Buffer count: 1/5
Producer [pid 4]: Produced item 3 | Buffer count: 2/5
...
Producer [pid 4]: Produced item 10 | Buffer count: 5/5
Producer: Buffer is FULL (5/5 slots), waiting on empty semaphore...
Consumer [pid 5]: Consumed item 6 | Buffer count: 4/5
Producer [pid 4]: Produced item 11 | Buffer count: 5/5
...
Consumer verification: ALL 20 items received correctly in FIFO order!
============================================================
Simulation Finished Summary:
Peak buffer usage: 5 / 5 slots
Min buffer usage: 0 / 5 slots
SUCCESS: No buffer overflow or underflow detected.
Buffer is cleanly drained (final count = 0).
============================================================
