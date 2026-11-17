================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 5 - QUESTION 2
Producer-Consumer Problem with Bounded Buffer using Semaphores
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 5, Question 2 (5b)
- Topic          : Process Concurrency & Semaphore Synchronization
- Test Program   : user/prodcons.c
- Shell Command  : $ prodcons

Objective:
Solve the classic Bounded-Buffer Producer-Consumer synchronization problem using
kernel-level counting semaphores and a shared memory region in xv6:
1. Shared Memory Circular Buffer:
   Mapped at `SHM_VA = (TRAPFRAME - PGSIZE) = 0x3FFFFFFE000` with capacity $N = 5$.
2. Synchronization Invariant:
   - Prevent buffer overflow: Producer must block when buffer is full (count == 5).
   - Prevent buffer underflow: Consumer must block when buffer is empty (count == 0).
   - Mutual exclusion: Only one process may modify buffer indices and count at any instant.
3. Semaphores:
   - `SEM_MUTEX` (ID 0) : Binary semaphore initialized to 1 (mutual exclusion).
   - `SEM_EMPTY` (ID 1) : Counting semaphore initialized to 5 (empty slots available).
   - `SEM_FULL`  (ID 2) : Counting semaphore initialized to 0 (filled slots available).


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Dijkstra's Bounded-Buffer Formulation:
   - Two counting semaphores (`empty` and `full`) track buffer state.
   - One binary semaphore (`mutex`) ensures critical-section atomicity.
   - Fundamental Invariant:
       $$Empty + Full = BufferSize = 5$$
       $$0 \le Count \le BufferSize$$

2. Producer Synchronization Pattern:
   ```c
   sem_wait(SEM_EMPTY); // Wait for available empty slot (decrements empty)
   sem_wait(SEM_MUTEX); // Enter critical section
   // Insert item into buffer
   buffer[in] = item;
   in = (in + 1) % BUFFER_SIZE;
   count++;
   sem_post(SEM_MUTEX); // Exit critical section
   sem_post(SEM_FULL);  // Signal filled slot available (increments full)
   ```

3. Consumer Synchronization Pattern:
   ```c
   sem_wait(SEM_FULL);  // Wait for available filled slot (decrements full)
   sem_wait(SEM_MUTEX); // Enter critical section
   // Remove item from buffer
   item = buffer[out];
   out = (out + 1) % BUFFER_SIZE;
   count--;
   sem_post(SEM_MUTEX); // Exit critical section
   sem_post(SEM_EMPTY); // Signal empty slot available (increments empty)
   ```

4. Deadlock-Avoidance Rule (Order of Waits):
   - Notice that `sem_wait(SEM_EMPTY)` must occur BEFORE `sem_wait(SEM_MUTEX)`.
   - If a producer acquired `mutex` first and then waited on `empty` when the buffer
     was full, it would sleep while holding `mutex`, permanently preventing the
     consumer from entering to consume and free a slot (causing classic deadlock).


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/memlayout.h    | Modified | Defined SHM_VA = (TRAPFRAME - PGSIZE) = 0x3FFFFFFE000.
kernel/defs.h         | Modified | Added declarations for shminit, shm_get, seminit,
                      |          | ksem_init, ksem_wait, ksem_post, walk.
kernel/main.c         | Modified | Initialized shared memory and semaphore tables.
kernel/proc.c         | Modified | Propagated SHM_VA in fork(); preserved physical
                      |          | frame on exit in proc_freepagetable().
kernel/sysproc.c      | Modified | Implemented kernel counting semaphore subsystem
                      |          | with sleep/wakeup queues and shm allocator.
kernel/syscall.h      | Modified | Defined SYS_shm_get (23), SYS_sem_init (24),
                      |          | SYS_sem_wait (25), SYS_sem_post (26).
kernel/syscall.c      | Modified | Registered semaphore and shared memory syscalls.
user/user.h           | Modified | Declared shm_get(), sem_init(), sem_wait(), sem_post().
user/usys.pl          | Modified | Added syscall stub entries.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_prodcons.
user/prodcons.c       | Created  | Test program spawning producer & consumer processes
                      |          | passing 20 items through a 5-slot bounded buffer.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) Kernel Semaphore Structure (kernel/sysproc.c):
   ```c
   struct semaphore {
     int value;             // Counting value
     struct spinlock lock;  // Protects value and state
     int active;            // 1 if allocated, 0 if free
   } sem_table[32];
   ```

b) Kernel Semaphore Operations:
   - `ksem_init(int id, int val)`: Sets `sem->value = val` and `sem->active = 1`.
   - `ksem_wait(int id)`:
     ```c
     acquire(&sem->lock);
     while (sem->value <= 0) {
       sleep(sem, &sem->lock); // Atomically sleeps and releases spinlock
     }
     sem->value--;
     release(&sem->lock);
     ```
   - `ksem_post(int id)`:
     ```c
     acquire(&sem->lock);
     sem->value++;
     wakeup(sem);              // Wakes all processes sleeping on this semaphore
     release(&sem->lock);
     ```


5. SYSTEM CALL SPECIFICATIONS
--------------------------------------------------------------------------------
1. void *shm_get(void)
   - Returns: Virtual address pointer to shared page (`0x3FFFFFFE000`), or 0 on error.
2. int sem_init(int sem_id, int initial_value)
   - Initializes semaphore `sem_id` (0..31) with `initial_value`. Returns 0 on success.
3. int sem_wait(int sem_id)
   - Blocks if `value <= 0`, then decrements value. Returns 0 on success.
4. int sem_post(int sem_id)
   - Increments value and wakes sleeping processes. Returns 0 on success.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/prodcons

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ prodcons

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
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
Producer [pid 4]: Produced item 4 | Buffer count: 3/5
Producer [pid 4]: Produced item 5 | Buffer count: 4/5
Producer [pid 4]: Produced item 6 | Buffer count: 5/5
Producer: Buffer is FULL (5/5 slots), waiting on empty semaphore...
Consumer [pid 5]: Consumed item 2 | Buffer count: 4/5
Producer [pid 4]: Produced item 7 | Buffer count: 5/5
Producer: Buffer is FULL (5/5 slots), waiting on empty semaphore...
Consumer [pid 5]: Consumed item 3 | Buffer count: 4/5
...
Consumer verification: ALL 20 items received correctly in FIFO order!
============================================================
Simulation Finished Summary:
Peak buffer usage: 5 / 5 slots
Min buffer usage: 0 / 5 slots
SUCCESS: No buffer overflow or underflow detected.
Buffer is cleanly drained (final count = 0).
============================================================

Analysis:
- Whenever buffer reached 5 items, producer blocked on `SEM_EMPTY`.
- As soon as consumer consumed an item, it signaled `SEM_EMPTY`, unblocking producer.
- Buffer usage strictly remained between 0 and 5 slots. All 20 items passed in order.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: What happens if the producer executes `sem_wait(SEM_MUTEX)` before `sem_wait(SEM_EMPTY)`?
A1: A deadlock will occur when the buffer is full. The producer acquires `mutex`,
    finds the buffer full, and blocks on `empty`. The consumer cannot acquire
    `mutex` to consume items and signal `empty`, causing permanent deadlock.

Q2: Why are counting semaphores needed instead of just binary semaphores?
A2: Counting semaphores keep track of integer resource quantities (here, the number
    of empty slots and full slots available). Binary semaphores can only represent
    0 or 1, which suffices for mutual exclusion but cannot naturally track bounded
    capacities.

Q3: How does `sleep(chan, &lock)` in xv6 avoid lost wakeup race conditions?
A3: `sleep()` atomically releases the passed spinlock while simultaneously putting
    the process to sleep and acquiring `p->lock`. This prevents an intervening
    `wakeup()` from firing between releasing the lock and sleeping.

Q4: Why does `ksem_wait()` use a `while (sem->value <= 0)` loop instead of `if`?
A4: To handle spurious wakeups or multi-process wakeup races: when `wakeup()` is
    called, multiple waiting processes may wake up. Using a `while` loop re-checks
    the condition to ensure an available slot actually exists before decrementing.
================================================================================
