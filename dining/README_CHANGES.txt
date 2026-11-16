================================================================================
Operating Systems Lab Assignment - 5
Question 4: Dining Philosophers Problem with Deadlock Avoidance using Semaphores
================================================================================

1. OVERVIEW & PROBLEM MODEL
---------------------------
This question implements a deadlock-free synchronization solution for the classic
Dining Philosophers problem in xv6 using kernel-level counting semaphores and a
shared memory region.

Model Specifications:
- 5 Philosophers: Spawned as 5 concurrent child processes using fork().
- 5 Forks: Represented as binary semaphores (IDs 0..4), each initialized to 1.
- Seating Arrangement:
  Philosopher i sits between fork i (left fork) and fork (i + 1) % 5 (right fork).
- Dining Cycles:
  Each philosopher iterates through 5 cycles of:
  * THINKING: Sleeps for a short period.
  * HUNGRY: Requests and acquires both required forks.
  * EATING: Holds both forks, verifies mutual exclusion with adjacent neighbors,
            eats (sleeps for 2 ticks), and releases both forks.

2. DEADLOCK AVOIDANCE STRATEGY: RESOURCE ORDERING (HIERARCHY OF RESOURCES)
--------------------------------------------------------------------------
Deadlock requires four necessary Coffman conditions:
1. Mutual Exclusion
2. Hold and Wait
3. No Preemption
4. Circular Wait

To prevent deadlock, we strictly eliminate the Circular Wait condition using
Resource Ordering:
- We assign a global strict total ordering to the forks: Fork 0 < Fork 1 < Fork 2 < Fork 3 < Fork 4.
- Every philosopher is required to request and acquire the lower-numbered fork first,
  followed by the higher-numbered fork:
    first_fork  = min(left, right)
    second_fork = max(left, right)

Concrete Behavior:
- Philosopher 0: forks 0 & 1 -> acquires Fork 0 first, then Fork 1.
- Philosopher 1: forks 1 & 2 -> acquires Fork 1 first, then Fork 2.
- Philosopher 2: forks 2 & 3 -> acquires Fork 2 first, then Fork 3.
- Philosopher 3: forks 3 & 4 -> acquires Fork 3 first, then Fork 4.
- Philosopher 4: forks 4 & 0 -> min(4, 0) = 0! Acquires Fork 0 first, then Fork 4.

Why Deadlock is Mathematically Impossible:
If all philosophers were to pick up their left fork simultaneously in a naive solution,
a cycle of dependencies forms (P0 holds F0 waits for F1, P1 holds F1 waits for F2, ...,
P4 holds F4 waits for F0).
Under resource ordering, Philosopher 4 competes directly with Philosopher 0 for Fork 0.
Only one of them can acquire Fork 0. The other blocks before picking up any fork.
Therefore, at least one philosopher is always guaranteed to acquire both of their required
forks and eat, completely eliminating circular wait.

3. FILES MODIFIED / ADDED
-------------------------
- kernel/memlayout.h:
  * Defined SHM_VA as (TRAPFRAME - PGSIZE).
- kernel/defs.h:
  * Added declarations for walk(), shminit(), shm_get(), seminit(), ksem_init(),
    ksem_wait(), and ksem_post().
- kernel/main.c:
  * Initialized shminit() and seminit() during system boot.
- kernel/proc.c:
  * Unmapped SHM_VA in proc_freepagetable() without freeing the shared physical frame.
  * Inherited SHM_VA mapping across fork() in kfork().
- kernel/sysproc.c:
  * Implemented shminit(), shm_get(), sys_shm_get().
  * Implemented kernel semaphores table and syscalls (sem_init, sem_wait, sem_post).
- kernel/syscall.h & kernel/syscall.c:
  * Registered SYS_shm_get (23), SYS_sem_init (24), SYS_sem_wait (25), SYS_sem_post (26).
- user/user.h & user/usys.pl:
  * Exported shm_get(), sem_init(), sem_wait(), sem_post() to user space.
- user/dining.c:
  * Implemented 5-philosopher simulation with state transition printing and mutual exclusion assertions.
- Makefile:
  * Added $U/_dining to UPROGS.
  * Cleared DETFLAGS to prevent compiler errors on paths with spaces.
  * Set CPUS := 1.

4. HOW TO BUILD AND RUN
-----------------------
Inside this directory:
    $ make clean
    $ make qemu

Once xv6 boots into the shell, run:
    $ dining

To exit QEMU:
    Press Ctrl+A, then X.

5. SAMPLE OUTPUT VERIFICATION
-----------------------------
$ dining
============================================================
Dining Philosophers Simulation with Deadlock Avoidance
Philosophers = 5, Forks = 5, Cycles = 5
Strategy: Resource Ordering (acquire lower-numbered fork first)
============================================================
Philosopher 0 [pid 4]: THINKING -> HUNGRY (Cycle 1/5)
Philosopher 0 [pid 4]: picked up fork 0, waiting for fork 1
Philosopher 0 [pid 4]: HUNGRY -> EATING (holding forks 0, 1) [Meal #1]
...
Philosopher 4 [pid 8]: picked up fork 0, waiting for fork 4
Philosopher 4 [pid 8]: HUNGRY -> EATING (holding forks 0, 4) [Meal #1]
...
============================================================
Simulation Finished Summary:
Philosopher 0 ate 5 / 5 meals.
Philosopher 1 ate 5 / 5 meals.
Philosopher 2 ate 5 / 5 meals.
Philosopher 3 ate 5 / 5 meals.
Philosopher 4 ate 5 / 5 meals.
Total meals eaten: 25 (Expected: 25)
Mutual exclusion violations: 0
SUCCESS:
  1. All 5 philosophers completed all 5 cycles without deadlocking.
  2. No two adjacent philosophers ate simultaneously.
  3. Resource ordering effectively eliminated circular wait.
============================================================
