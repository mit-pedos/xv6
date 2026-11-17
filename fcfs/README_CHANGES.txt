================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 3 - QUESTION 1
First-Come, First-Served (FCFS) Non-Preemptive Scheduler
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 3, Question 1 (3a)
- Topic          : CPU Scheduling Algorithms (Set 1)
- Test Program   : user/fcfstest.c
- Shell Command  : $ fcfstest

Objective:
Replace the default xv6 round-robin (preemptive) scheduler with a First-Come,
First-Served (FCFS) non-preemptive scheduler. In FCFS:
1. Every process is timestamped with its creation time (ctime) at allocation.
2. The scheduler always dispatches the RUNNABLE process that has the earliest
   creation time (smallest ctime).
3. Once a process is given the CPU, it runs non-preemptively until it finishes
   (exits) or voluntarily blocks (sleeps/waits on I/O). Timer interrupts must NOT
   preempt a running process.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. FCFS Scheduling Mechanics:
   - Category: Non-preemptive scheduling.
   - Criterion: Earliest Arrival / Creation Time (min ctime).
   - Selection Rule: For all p in RUNNABLE, select p* such that:
       ctime(p*) = min_{p in RUNNABLE} (p->ctime)
   - Advantage: Extremely simple; zero starvation if all jobs are finite; low
     scheduler overhead since no complex priority calculations are required.
   - Disadvantage: Convoy Effect (short processes stuck behind a long CPU-bound
     process, leading to high average waiting time).

2. How xv6 Scheduling Differs from Default:
   - Default xv6:
     * Round-Robin (RR) scheduling with a fixed time slice (1 timer tick ~ 100ms).
     * In kernel/trap.c: upon receiving a timer interrupt (which_dev == 2),
       usertrap() and kerneltrap() call yield(), forcing the running process
       to relinquish the CPU.
     * scheduler() sequentially loops over the process table array and runs the
       first RUNNABLE process it encounters.
   - FCFS xv6:
     * Non-preemptive: yield() on timer interrupts is disabled in kernel/trap.c.
     * Creation Timestamp: Each process records ticks at creation in allocproc().
     * Global Selection: scheduler() scans the entire proc[] table, finds the
       RUNNABLE process with minimum ctime, and context-switches to it.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Added uint ctime to struct proc.
kernel/proc.c         | Modified | Recorded ctime in allocproc(); reset in
                      |          | freeproc(); implemented min-ctime FCFS scan
                      |          | in scheduler().
kernel/trap.c         | Modified | Commented out yield() calls on timer
                      |          | interrupts in usertrap() and kerneltrap().
kernel/syscall.h      | Modified | Kept synchronized with kernel declarations.
kernel/syscall.c      | Modified | Kept synchronized with kernel declarations.
kernel/sysproc.c      | Modified | Documented FCFS configuration.
Makefile              | Modified | Set CPUS := 1 to prevent multi-core interleaving;
                      |          | cleared DETFLAGS for paths with spaces; added
                      |          | $U/_fcfstest to UPROGS.
user/fcfstest.c       | Created  | Test program spawning 3 staggered CPU-bound
                      |          | children and checking completion order.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/proc.h:
   Inside `struct proc`:
   ```c
   struct proc {
     struct spinlock lock;
     ...
     uint ctime;                  // Creation timestamp (in clock ticks)
     ...
   };
   ```

b) kernel/proc.c:
   1. `allocproc()`:
      When a process is allocated, its creation timestamp is captured from the
      global `ticks` counter under `tickslock`:
      ```c
      acquire(&tickslock);
      p->ctime = ticks;
      release(&tickslock);
      ```
   2. `freeproc()`:
      When a process terminates and is reaped, its timestamp is cleared:
      ```c
      p->ctime = 0;
      ```
   3. `scheduler()`:
      Replaced the round-robin loop. In each iteration, the scheduler finds the
      runnable process with the lowest `ctime`:
      ```c
      struct proc *earliest = 0;
      for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == RUNNABLE) {
          if (earliest == 0 || p->ctime < earliest->ctime) {
            if (earliest)
              release(&earliest->lock);
            earliest = p;
            continue; // keep earliest->lock held
          }
        }
        release(&p->lock);
      }

      if (earliest) {
        earliest->state = RUNNING;
        c->proc = earliest;
        swtch(&c->context, &earliest->context);
        c->proc = 0;
        release(&earliest->lock);
      }
      ```
      Locking Discipline: Notice `earliest->lock` is held continuously once
      found so no other CPU core can race and schedule it.

c) kernel/trap.c:
   Enforcing Non-Preemption:
   In `usertrap()` and `kerneltrap()`, timer interrupts normally trigger `yield()`.
   These are commented out:
   ```c
   // if(which_dev == 2)
   //   yield();
   ```
   Now, a CPU-bound process continues executing on the CPU until it explicitly
   terminates via `exit()` or sleeps.

d) Makefile:
   - `CPUS := 1`: A single CPU core ensures pure FCFS ordering is observable
     without concurrent multi-core parallelism.
   - `UPROGS += $U/_fcfstest`: Adds the test binary to the root filesystem.


5. USER TEST HARNESS WALKTHROUGH (user/fcfstest.c)
--------------------------------------------------------------------------------
The test program verifies non-preemptive FCFS behavior:
1. The parent process records its own PID.
2. It forks 3 child processes sequentially. Between each `fork()`, it executes a
   busy `delay(5000000)` loop so that Child 1, Child 2, and Child 3 are assigned
   strictly increasing creation timestamps (`ctime1 < ctime2 < ctime3`).
3. Each child performs heavy arithmetic computation (`WORKLOAD = 50000000ULL`).
4. The parent reaps the children using `wait(0)` in a loop and prints their
   completion order.
5. In a preemptive round-robin scheduler, processes would interleave. Under FCFS,
   Child 1 must run entirely to completion, followed by Child 2, followed by Child 3.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this assignment folder:
        cd /home/udit/Desktop/xv6/fcfs

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ fcfstest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ fcfstest
--- FCFS Scheduler Test Starting ---
Parent PID: 3
Forking 3 CPU-bound children with staggered arrival...
[Parent] Forked Child 1 (PID 4)
[Parent] Forked Child 2 (PID 5)
[Parent] Forked Child 3 (PID 6)

[Parent] All 3 children forked. Waiting for children to complete...

[Child 1 | PID 4] Created, starting CPU-bound workload...
[Child 1 | PID 4] Finished computation (counter=1283106752)
>> Completion Order 1: PID 4 (Child 1)
[Child 2 | PID 5] Created, starting CPU-bound workload...
[Child 2 | PID 5] Finished computation (counter=1283106752)
>> Completion Order 2: PID 5 (Child 2)
[Child 3 | PID 6] Created, starting CPU-bound workload...
[Child 3 | PID 6] Finished computation (counter=1283106752)
>> Completion Order 3: PID 6 (Child 3)

--- FCFS Test Passed: Processes completed strictly in creation order! ---

Analysis:
- Child 1 (PID 4) was forked first and had the lowest `ctime`. It ran to 100%
  completion without yielding to Child 2 or Child 3.
- Child 2 (PID 5) only started after Child 1 completely finished.
- Child 3 (PID 6) ran last.
- The completion order (PID 4 -> PID 5 -> PID 6) strictly matches the arrival order.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: How did you make the scheduler non-preemptive in xv6?
A1: In default xv6, timer interrupts generate a device interrupt (`which_dev == 2`),
    prompting `usertrap()` and `kerneltrap()` in `kernel/trap.c` to call `yield()`.
    We commented out `yield()` on timer interrupts. Thus, timer ticks increment
    the clock but never strip the CPU from a running process.

Q2: Why must `tickslock` be acquired when reading `ticks` in `allocproc()`?
A2: `ticks` is a shared global variable incremented by the timer interrupt handler
    in `clockintr()`. Reading it without `tickslock` could result in a data race
    or reading a torn/inconsistent value on 32/64-bit systems.

Q3: What happens to process locking while searching for the earliest process?
A3: To safely inspect `p->state` and `p->ctime`, we must acquire `p->lock`. If a
    candidate has a lower `ctime` than the current best candidate (`earliest`),
    we release the previous `earliest->lock` and retain the lock on the new
    `earliest`. This prevents any race condition where another CPU or an interrupt
    changes the state of the selected candidate before context switching.

Q4: What is the Convoy Effect, and does this implementation suffer from it?
A4: Yes. In FCFS, if a long CPU-bound process arrives before several short
    interactive processes, all short processes must wait in the `RUNNABLE` queue
    until the long process completes. This significantly degrades average waiting
    and turnaround time.

Q5: Why did we set `CPUS := 1` in the Makefile?
A5: In a multi-core environment (`CPUS > 1`), multiple CPU cores run the `scheduler()`
    loop concurrently. Even if Child 1 runs on CPU 0, CPU 1 could simultaneously
    pick up Child 2, causing them to execute in parallel. Setting `CPUS := 1`
    forces strict single-core sequential execution so FCFS ordering is deterministic.
================================================================================
