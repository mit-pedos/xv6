================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 6 - QUESTION 4
Combined Multi-Resource Synchronization & Deadlock Avoidance
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 6, Question 4 (6d)
- Topic          : Multi-Resource Synchronization & Deadlock Prevention
- Test Program   : user/syncdeadlock.c
- Shell Command  : $ syncdeadlock

Objective:
Manage 5 concurrent processes competing for 3 limited shared resource types in xv6
using kernel counting semaphores and hierarchical resource ordering:
1. Resource Types & Capacities:
   - Resource 0: Printers (Capacity = 2 instances)
   - Resource 1: Scanners (Capacity = 1 instance)
   - Resource 2: Disks    (Capacity = 2 instances)
2. Demands:
   - P0: Printer (0) & Scanner (1)
   - P1: Scanner (1) & Disk (2)
   - P2: Printer (0) & Disk (2)
   - P3: Printer (0) & Scanner (1)
   - P4: Scanner (1) & Disk (2)
3. Deadlock Prevention Strategy:
   Enforce Havender's hierarchical resource ordering:
     $$\text{Printer (0)} < \text{Scanner (1)} < \text{Disk (2)}$$
   Each process requests its required resources strictly in increasing order of
   resource ID: first $\min(R_a, R_b)$, then $\max(R_a, R_b)$.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Multi-Resource Competition & Circular Wait:
   When multiple processes require multiple heterogeneous resources, inconsistent
   request orderings (e.g. P0 grabs Printer and waits for Scanner; P1 grabs Scanner
   and waits for Printer) cause immediate deadlocks.
2. Directed Acyclic Graph (DAG) Guarantee:
   By mandating that any process requesting resources $\{R_a, R_b\}$ must request
   $R_{lower}$ before $R_{higher}$, all edges in the Wait-For Graph point from
   lower resource classes to higher resource classes.
   A directed graph where all edges follow a strict topological order cannot contain
   cycles, mathematically eliminating the Circular Wait condition.
3. Capacity Invariant:
   Counting semaphores initialized to `[2, 1, 2]` guarantee that no more than
   2 printers, 1 scanner, and 2 disks can ever be simultaneously allocated.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/memlayout.h    | Modified | Defined SHM_VA = (TRAPFRAME - PGSIZE).
kernel/defs.h         | Modified | Declared shm and semaphore functions.
kernel/main.c         | Modified | Initialized semaphore subsystem (seminit()).
kernel/proc.c         | Modified | Propagated shared page in fork().
kernel/sysproc.c      | Modified | Implemented counting semaphores and shm allocator.
kernel/syscall.h      | Modified | Registered semaphore syscall numbers.
kernel/syscall.c      | Modified | Wired syscall table.
user/user.h           | Modified | Declared sem_init, sem_wait, sem_post.
user/usys.pl          | Modified | Added syscall stubs.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_syncdeadlock.
user/syncdeadlock.c   | Created  | Test driver creating 5 processes running 3 cycles
                      |          | across 3 resource pools with capacity verification.


4. DETAILED CODE WALKTHROUGH (user/syncdeadlock.c)
--------------------------------------------------------------------------------
a) Shared State Structure:
   ```c
   struct system_state {
     int in_use[3];          // Current allocation count for Printer, Scanner, Disk
     int capacity_limit[3];  // Max capacity [2, 1, 2]
     int tasks_done[5];      // Number of tasks completed per process
     int violations;         // Over-allocation violation counter
     int mutex;              // Semaphore 3 protects state updates
   };
   ```

b) Process Execution Protocol:
   ```c
   void run_process(int p_id, int r1, int r2) {
     int first = (r1 < r2) ? r1 : r2;
     int second = (r1 < r2) ? r2 : r1;

     for (int cycle = 1; cycle <= 3; cycle++) {
       // Request first resource (lower order ID)
       sem_wait(first);
       // Request second resource (higher order ID)
       sem_wait(second);

       // Work section (holding both resources)
       pause(1);

       // Release both resources
       sem_post(second);
       sem_post(first);
     }
   }
   ```


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/syncdeadlock

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ syncdeadlock

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ syncdeadlock
============================================================
 Multi-Resource Synchronization with Deadlock Avoidance   
 Processes: 5, Resources: Printer(2), Scanner(1), Disk(2)   
 Strategy : Resource Ordering (Hierarchy of Resources)      
 Cycles   : 3 per process (Total tasks = 15)               
============================================================
[Proc 0 (pid 4)]: Requesting Printer (first resource)...
[Proc 0 (pid 4)]: GRANTED Printer (In use: 1/2)
[Proc 0 (pid 4)]: Requesting Scanner (second resource)...
[Proc 0 (pid 4)]: GRANTED Scanner (In use: 1/1)
[Proc 0 (pid 4)]: Holding [Printer, Scanner]. STARTING WORK (Cycle 1/3)...
...
[Proc 4 (pid 8)]: FINISHED WORK. Released [Scanner, Disk] (Cycle 3/3 done).
[Proc 4 (pid 8)]: Completed all 3 cycles cleanly.
============================================================
Simulation Finished Summary:
Process 0 completed: 3 / 3 tasks.
Process 1 completed: 3 / 3 tasks.
Process 2 completed: 3 / 3 tasks.
Process 3 completed: 3 / 3 tasks.
Process 4 completed: 3 / 3 tasks.
Total tasks completed: 15 (Expected: 15)
Capacity violations: 0
SUCCESS:
  1. All 5 processes completed without any deadlock or starvation.
  2. Resource capacities (Printer: 2, Scanner: 1, Disk: 2) were strictly preserved.
  3. Resource ordering guaranteed acyclic wait-for relationships.
============================================================

Analysis:
- All 5 processes finished all 3 cycles ($5 \times 3 = 15$ total tasks).
- Peak resource counts strictly respected limits (Printer $\le 2$, Scanner $\le 1$, Disk $\le 2$).
- Zero deadlocks occurred despite high contention on the single Scanner instance.


7. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is Scanner (Capacity = 1) the bottleneck in this system?
A1: Because 4 out of 5 processes (P0, P1, P3, P4) compete for Scanner, and only 1
    instance exists. The binary semaphore on Scanner serializes these processes
    while allowing other processes to utilize Printers and Disks in parallel.

Q2: How does the system guarantee that P0 (Printer + Scanner) and P1 (Scanner + Disk)
    never deadlock?
A2: Under the ordering protocol, P0 acquires Printer (0) then Scanner (1). P1 acquires
    Scanner (1) then Disk (2). If P0 holds Printer and waits for Scanner while P1 holds
    Scanner and waits for Disk, no cycle exists because P1 is waiting for Disk (2),
    not Printer (0). Wait dependencies only flow upward ($0 \to 1 \to 2$).

Q3: Can counting semaphores alone prevent deadlock without resource ordering?
A3: No. Counting semaphores enforce capacity constraints and prevent race conditions,
    but they do not restrict the order in which processes request multiple semaphores.
    Without resource ordering, concurrent processes requesting semaphores in reverse
    order will deadlock.

Q4: In what order should resources be released?
A4: Typically, resources are released in reverse order of acquisition ($\max \to \min$)
    to respect LIFO nesting, but once acquired, the order of release does not affect
    deadlock safety since holding resources while requesting none cannot cause circular wait.
================================================================================
