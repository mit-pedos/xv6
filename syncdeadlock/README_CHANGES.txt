================================================================================
Operating Systems Lab Assignment
Question 4: Combined Multi-Resource Synchronization & Deadlock Avoidance
================================================================================

1. OVERVIEW & PROBLEM MODEL
---------------------------
This question implements a generalized multi-resource synchronization system
for 5 concurrent processes with limited shared resources in xv6 (user/syncdeadlock.c).

System Configuration:
- 5 Processes: Process 0 to Process 4, created using fork().
- 3 Shared Resource Types:
  * Resource 0: Printer  (Capacity = 2 instances)
  * Resource 1: Scanner  (Capacity = 1 instance)
  * Resource 2: Disk     (Capacity = 2 instances)
- Resource Demands: Each process requires 2 different resources to complete a task:
  * P0: Printer (0) & Scanner (1)
  * P1: Scanner (1) & Disk (2)
  * P2: Printer (0) & Disk (2)
  * P3: Printer (0) & Scanner (1)
  * P4: Scanner (1) & Disk (2)
- Task Execution:
  Each process alternates between requesting resources, waiting until granted,
  performing its work (holding both resources), releasing both resources, and
  repeating for 3 cycles (15 total tasks).

2. DEADLOCK AVOIDANCE STRATEGY: RESOURCE ORDERING (HIERARCHY OF RESOURCES)
--------------------------------------------------------------------------
Strategy Chosen & Justification:
- We assign a global strict linear order to all resource types:
    Printer (0) < Scanner (1) < Disk (2)
- Protocol:
  When any process requests its pair of required resources {Ra, Rb}, it MUST request
  them in strictly increasing order of their numerical ID:
    first_resource  = min(Ra, Rb)
    second_resource = max(Ra, Rb)
  The process acquires first_resource, waits until granted, and only then requests
  second_resource.
- Why Deadlock is Impossible:
  Deadlock requires a circular wait cycle in the Resource Allocation Graph:
    P_0 -> P_1 -> ... -> P_k -> P_0
  where each process holds a resource needed by the next.
  Because all requests follow the strict linear ordering 0 < 1 < 2:
  * A process holding a lower-numbered resource can only request a strictly higher-numbered one.
  * No process can ever hold a higher-numbered resource (e.g. Disk or Scanner) while waiting
    for a lower-numbered resource (e.g. Printer).
  Consequently, all wait-for dependency edges point strictly in the direction of increasing
  resource numbers. The dependency graph is guaranteed to be a Directed Acyclic Graph (DAG),
  making circular wait mathematically impossible.

3. FILES MODIFIED / ADDED
-------------------------
- kernel/memlayout.h, kernel/defs.h, kernel/main.c, kernel/proc.c, kernel/sysproc.c,
  kernel/syscall.h, kernel/syscall.c, user/user.h, user/usys.pl:
  * Implemented shared memory page mapping (shm_get) and counting semaphores
    (sem_init, sem_wait, sem_post) to represent resource instance pools.
- user/syncdeadlock.c:
  * Test driver creating 5 processes, enforcing resource ordering, simulating work,
    and verifying resource capacity limits.
- Makefile:
  * Added $U/_syncdeadlock to UPROGS.
  * Cleared DETFLAGS to prevent compiler errors on paths with spaces.
  * Set CPUS := 1.

4. HOW TO BUILD AND RUN
-----------------------
Inside this directory:
    $ make clean
    $ make qemu

Once xv6 boots into the shell, run:
    $ syncdeadlock

To exit QEMU:
    Press Ctrl+A, then X.

5. SAMPLE OUTPUT VERIFICATION
-----------------------------
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
