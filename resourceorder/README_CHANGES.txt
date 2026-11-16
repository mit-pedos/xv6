================================================================================
Operating Systems Lab Assignment
Question 3: Deadlock Prevention via Resource Ordering (Hierarchical Allocation)
================================================================================

1. OVERVIEW & THEORETICAL FOUNDATION
------------------------------------
This question demonstrates deadlock prevention on actual running xv6 processes
using the Resource Ordering (Hierarchical Allocation) technique.

The Four Coffman Conditions for Deadlock:
1. Mutual Exclusion: At least one non-shareable resource.
2. Hold and Wait: A process holds at least one resource and is waiting to acquire
   additional resources held by other processes.
3. No Preemption: Resources cannot be forcibly preempted from a process.
4. Circular Wait: A closed chain of processes exists, where each process holds
   a resource needed by the next process in the chain.

Why Resource Ordering Eliminates Deadlock:
- Let R = {R1, R2, ..., Rm} be the set of all system resource types.
- Assign a 1-to-1 mapping function F: R -> N that establishes a global linear order.
- Protocol: Every process must request resources in strictly increasing order of F.
  If a process holds resource Ri, it can only request resource Rj if F(Rj) > F(Ri).
- Proof of Deadlock Prevention:
  Assume a circular wait exists: P0 -> P1 -> P2 -> ... -> Pk -> P0, where Pi holds
  resource R(i) and waits for resource R(i+1), and Pk holds R(k) and waits for R(0).
  By the ordering protocol:
    F(R(0)) < F(R(1)) < F(R(2)) < ... < F(R(k)) < F(R(0))
  This yields F(R(0)) < F(R(0)), which is a mathematical contradiction!
  Therefore, a circular wait is impossible, mathematically guaranteeing that the
  system remains completely free of deadlocks.

2. IMPLEMENTATION DETAILS
-------------------------
- Resources:
  Two shared locks implemented via kernel semaphores (LOCK1 = ID 0, LOCK2 = ID 1).
- Program (user/resourceorder.c):
  Accepts a mode argument:
  1. "bad": Demonstrates the Deadlock Problem.
     - Process A: Acquires LOCK1, delays, then attempts to acquire LOCK2.
     - Process B: Acquires LOCK2, delays, then attempts to acquire LOCK1.
     - Result: Process A holds LOCK1 and waits for LOCK2; Process B holds LOCK2
       and waits for LOCK1. The system hangs indefinitely in circular wait.
  2. "fixed": Demonstrates Deadlock Prevention via Consistent Resource Ordering.
     - Global order: LOCK1 (ID 0) < LOCK2 (ID 1).
     - Both Process A and Process B are modified to acquire LOCK1 FIRST, then LOCK2.
     - When Process A holds LOCK1, Process B blocks waiting for LOCK1 BEFORE it can
       acquire LOCK2.
     - Process A completes its critical section and releases both locks.
     - Process B then proceeds and completes cleanly without any hang.

3. FILES MODIFIED / ADDED
-------------------------
- kernel/memlayout.h, kernel/defs.h, kernel/main.c, kernel/proc.c, kernel/sysproc.c,
  kernel/syscall.h, kernel/syscall.c, user/user.h, user/usys.pl:
  * Implemented kernel counting semaphore subsystem (sem_init, sem_wait, sem_post).
- user/resourceorder.c:
  * Implemented deadlock reproduction scenario ("bad") and deadlock prevention scenario ("fixed")
    with tick timestamp logging via uptime().
- Makefile:
  * Added $U/_resourceorder to UPROGS.
  * Cleared DETFLAGS to prevent compiler errors on paths with spaces.
  * Set CPUS := 1.

4. HOW TO BUILD AND RUN
-----------------------
Inside this directory:
    $ make clean
    $ make qemu

Once xv6 boots into the shell:

1. To run the FIXED run (Deadlock Prevented - completes cleanly):
    $ resourceorder fixed
    (or simply $ resourceorder)

2. To run the BAD run (Deadlock Demonstration - hangs indefinitely):
    $ resourceorder bad
    (To exit the hung QEMU session: Press Ctrl+A, then X.)

5. SAMPLE OUTPUT LOGS
---------------------

A. Fixed Run (Deadlock Prevented):
----------------------------------
$ resourceorder fixed
============================================================
  SCENARIO 2: RESOURCE ORDERING (DEADLOCK PREVENTED)
  Global Order: Lock1 (Resource 0) < Lock2 (Resource 1)
  Both Process A & Process B acquire Lock1 THEN Lock2.
============================================================
[Tick 2] [Process A (pid 4)]: Attempting to acquire Lock 1...
[Tick 2] [Process A (pid 4)]: ACQUIRED Lock 1.
[Tick 3] [Process A (pid 4)]: Attempting to acquire Lock 2...
[Tick 3] [Process A (pid 4)]: ACQUIRED Lock 2. Entering Critical Section.
[Tick 3] [Process B (pid 5)]: Attempting to acquire Lock 1...
[Tick 5] [Process A (pid 4)]: Exiting Critical Section. Releasing Lock 2 and Lock 1.
[Tick 5] [Process A (pid 4)]: Completed successfully.
[Tick 5] [Process B (pid 5)]: ACQUIRED Lock 1.
[Tick 5] [Process B (pid 5)]: Attempting to acquire Lock 2...
[Tick 5] [Process B (pid 5)]: ACQUIRED Lock 2. Entering Critical Section.
[Tick 7] [Process B (pid 5)]: Exiting Critical Section. Releasing Lock 2 and Lock 1.
[Tick 7] [Process B (pid 5)]: Completed successfully.
============================================================
SUCCESS: Both Process A and Process B completed cleanly!
Deadlock was completely eliminated by enforcing global resource ordering.
============================================================

B. Bad Run (Deadlock Demonstration - Hangs):
--------------------------------------------
$ resourceorder bad
============================================================
  SCENARIO 1: INCONSISTENT ORDERING (DEADLOCK DEMONSTRATION)
  Process A: Requests Lock1 -> Lock2
  Process B: Requests Lock2 -> Lock1 (Reverse Order)
============================================================
[Tick 1] [Process A (pid 4)]: Attempting to acquire Lock 1...
[Tick 1] [Process A (pid 4)]: ACQUIRED Lock 1.
[Tick 1] [Process A (pid 4)]: Holding Lock 1, sleeping 2 ticks...
[Tick 3] [Process B (pid 5)]: Attempting to acquire Lock 2...
[Tick 3] [Process B (pid 5)]: ACQUIRED Lock 2.
[Tick 3] [Process B (pid 5)]: Holding Lock 2, sleeping 2 ticks...
[Tick 4] [Process A (pid 4)]: Attempting to acquire Lock 2...
[Tick 5] [Process B (pid 5)]: Attempting to acquire Lock 1...
<System remains indefinitely hung waiting in circular wait>
