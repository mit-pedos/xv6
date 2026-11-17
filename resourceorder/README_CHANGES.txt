================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 6 - QUESTION 3
Deadlock Prevention via Resource Ordering (Hierarchical Allocation)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 6, Question 3 (6c)
- Topic          : Deadlock Prevention & Resource Ordering
- Test Program   : user/resourceorder.c
- Shell Command  : $ resourceorder fixed (or $ resourceorder bad)

Objective:
Demonstrate deadlock prevention on actual concurrent xv6 processes using Havender's
Resource Ordering (Hierarchical Allocation) technique:
1. Deadlock Demonstration Mode (`bad`):
   - Process A requests Lock 1, then Lock 2.
   - Process B requests Lock 2, then Lock 1 (inconsistent order).
   - Proves how inconsistent ordering generates circular wait and freezes the system.
2. Deadlock Prevention Mode (`fixed`):
   - Imposes a global linear order: $F(\text{Lock } 1) < F(\text{Lock } 2)$.
   - Both processes must request Lock 1 before Lock 2.
   - Mathematically eliminates the Circular Wait condition so both processes complete cleanly.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. The 4 Coffman Conditions for Deadlock:
   - Mutual Exclusion: Resources cannot be shared simultaneously.
   - Hold and Wait: A process holds at least one resource and is waiting to acquire another.
   - No Preemption: Resources cannot be forcibly seized from a process.
   - Circular Wait: A closed chain of processes exists: $P_0 \to P_1 \to \dots \to P_k \to P_0$.

2. Havender's Linear Resource Ordering Protocol:
   - Define a one-to-one mapping function $F: R \to \mathbb{N}$ that assigns a unique
     integer to every system resource.
   - Protocol Rule: A process can only request resource $R_j$ if $F(R_j) > F(R_i)$ for all
     resources $R_i$ currently held by the process.
   - Proof of Deadlock Impossibility:
     Suppose a circular wait exists among processes $P_0, P_1, \dots, P_k$.
     Process $P_i$ holds $R_{(i)}$ and waits for $R_{(i+1)}$, and $P_k$ holds $R_{(k)}$
     and waits for $R_{(0)}$.
     Under the ordering rule:
       $$F(R_{(0)}) < F(R_{(1)}) < F(R_{(2)}) < \dots < F(R_{(k)}) < F(R_{(0)})$$
     This implies $F(R_{(0)}) < F(R_{(0)})$, which is a logical contradiction!
     Therefore, circular wait cannot occur.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/sysproc.c      | Modified | Implemented kernel semaphore subsystem (sem_init,
                      |          | sem_wait, sem_post) and shm_get.
kernel/memlayout.h    | Modified | Defined SHM_VA = (TRAPFRAME - PGSIZE).
kernel/defs.h         | Modified | Declared semaphore and shared memory functions.
kernel/main.c         | Modified | Initialized semaphore table (seminit()).
kernel/proc.c         | Modified | Inherited shared page mapping across fork().
kernel/syscall.h      | Modified | Registered SYS_sem_init, SYS_sem_wait, SYS_sem_post.
kernel/syscall.c      | Modified | Wired semaphore syscall entries.
user/user.h           | Modified | Exported sem_init, sem_wait, sem_post to user API.
user/usys.pl          | Modified | Added syscall stubs.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_resourceorder.
user/resourceorder.c  | Created  | Implemented deadlock demonstration ("bad") and
                      |          | deadlock prevention ("fixed") modes with tick logging.


4. DETAILED CODE WALKTHROUGH (user/resourceorder.c)
--------------------------------------------------------------------------------
a) Inconsistent Ordering ("bad"):
   ```c
   // Process A: Lock 1 -> Lock 2
   sem_wait(LOCK1);
   pause(2);
   sem_wait(LOCK2); // Hangs waiting for Process B

   // Process B: Lock 2 -> Lock 1
   sem_wait(LOCK2);
   pause(2);
   sem_wait(LOCK1); // Hangs waiting for Process A
   ```

b) Hierarchical Resource Ordering ("fixed"):
   ```c
   // Global hierarchy: LOCK1 (0) < LOCK2 (1)
   // Both Process A and Process B acquire in the exact same order:
   sem_wait(LOCK1); // Process B blocks here BEFORE getting Lock 2!
   sem_wait(LOCK2);
   // Critical Section
   sem_post(LOCK2);
   sem_post(LOCK1);
   ```


5. SYSTEM CALL SPECIFICATIONS
--------------------------------------------------------------------------------
1. int sem_init(int sem_id, int initial_val): Initializes semaphore `sem_id`.
2. int sem_wait(int sem_id): Decrements value; blocks if `value <= 0`.
3. int sem_post(int sem_id): Increments value and wakes waiting processes.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/resourceorder

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Run the FIXED test (Deadlock Prevented - completes cleanly):
        $ resourceorder fixed
        (or simply $ resourceorder)

Step 4: (Optional) Run the BAD test (Deadlock Demonstration - hangs):
        $ resourceorder bad
        (Exit QEMU: Press Ctrl-A, then X).


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
A. Fixed Run (Deadlock Prevented):
--------------------------------------------------------------------------------
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
--------------------------------------------------------------------------------
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


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: What is the fundamental difference between Deadlock Prevention and Deadlock Avoidance?
A1: - Deadlock Prevention designs the system protocol such that at least one of the
      four necessary Coffman conditions can never hold (e.g. eliminating circular wait
      via resource ordering). No runtime safety checks are needed.
    - Deadlock Avoidance allows processes to request resources in any order, but the
      operating system dynamically evaluates each request (e.g. Banker's algorithm)
      and delays requests that would transition the system into an unsafe state.

Q2: Which Coffman condition does Resource Ordering break?
A2: The Circular Wait condition.

Q3: What is the main drawback of Resource Ordering in real-world systems?
A3: It reduces programmer flexibility and can cause inefficient resource utilization:
    a process may need to acquire a low-ordered resource long before it actually uses
    it simply because it needs to acquire a high-ordered resource later, preventing
    other processes from using that low-ordered resource in the meantime.

Q4: In the "fixed" scenario, why doesn't Process B cause a deadlock with Process A?
A4: Because Process B must acquire Lock 1 first. Since Process A already holds Lock 1,
    Process B blocks immediately before acquiring Lock 2. Process A can freely
    acquire Lock 2, finish its work, and release both locks.
================================================================================
