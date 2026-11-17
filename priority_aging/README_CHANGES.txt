================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 3 - QUESTION 2
Priority-Based Preemptive Scheduler with Aging (Anti-Starvation)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 3, Question 2 (3b) [Also referenced as Lab 7d]
- Topic          : CPU Scheduling Algorithms (Set 1)
- Test Program   : user/priotest.c
- Shell Command  : $ priotest

Objective:
Replace xv6's default round-robin scheduler with a Priority-Based Preemptive
Scheduler featuring an Aging Mechanism to prevent starvation:
1. Priority Range: Integer scale from 1 (highest priority) to 20 (lowest priority).
   Default priority for every newly allocated process is 10.
2. System Calls:
   - `int set_priority(int pid, int priority)`: Sets priority of target process
     (or current process if `pid == 0`), resets its wait counter, and returns old priority.
   - `int get_priority(int pid)`: Returns the current effective priority.
3. Preemptive Scheduling:
   The scheduler always selects the `RUNNABLE` process with the lowest priority
   number (highest priority). Ties among equal-priority processes are resolved
   using Round-Robin order.
4. Dynamic Aging (Anti-Starvation):
   If any `RUNNABLE` process waits for more than 30 ticks without getting CPU
   time, its priority number is decremented by 1 (raising priority towards 1),
   and its wait counter is reset to 0.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Priority Scheduling & Starvation:
   - In pure priority scheduling, CPU-bound high-priority processes can monopolize
     the CPU indefinitely, causing indefinite blocking or starvation of lower-priority
     processes.
2. Anti-Starvation through Aging:
   - Aging gradually increases the priority of processes that spend extended periods
     waiting in the ready (`RUNNABLE`) queue.
   - Aging Rule:
     For every process p where `p->state == RUNNABLE`:
       `p->wait_ticks++`
       if `p->wait_ticks > 30` and `p->priority > 1`:
         `p->priority--`    (lower numerical value = higher priority)
         `p->wait_ticks = 0`
   - Bound: Priority cannot be promoted higher than 1 (`max(1, p->priority - 1)`).
3. Preemption:
   - Unlike non-preemptive schedulers, timer interrupts continue calling `yield()`
     in `kernel/trap.c`. If a newly arrived or aged process attains a higher priority
     than the currently running process, the running process is immediately preempted.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Added int priority and uint wait_ticks to
                      |          | struct proc.
kernel/defs.h         | Modified | Declared set_priority, get_priority, and
                      |          | update_proc_aging.
kernel/proc.c         | Modified | Initialized priority=10 and wait_ticks=0 in
                      |          | allocproc(); reset in freeproc(); implemented
                      |          | set_priority(), get_priority(),
                      |          | update_proc_aging(), and priority scheduler().
kernel/trap.c         | Modified | Called update_proc_aging() on CPU 0 every
                      |          | clock tick in clockintr(); kept timer yield().
kernel/syscall.h      | Modified | Defined SYS_set_priority (23) & SYS_get_priority (24).
kernel/syscall.c      | Modified | Registered sys_set_priority and sys_get_priority.
kernel/sysproc.c      | Modified | Implemented syscall handlers for set/get priority.
user/user.h           | Modified | Declared set_priority() and get_priority() stubs.
user/usys.pl          | Modified | Added user-space syscall generation entries.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_priotest.
user/priotest.c       | Created  | Test program spawning 4 processes with priorities
                      |          | 5, 8, 12, 20 and demonstrating priority order & aging.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/proc.h:
   ```c
   struct proc {
     ...
     int priority;      // Priority level (1 = highest, 20 = lowest, default = 10)
     uint wait_ticks;   // Accumulated ticks spent waiting in RUNNABLE state
     ...
   };
   ```

b) kernel/proc.c:
   1. `allocproc()`:
      ```c
      p->priority = 10;
      p->wait_ticks = 0;
      ```
   2. `set_priority(int pid, int new_priority)`:
      Validates `1 <= new_priority <= 20`. Searches process table for `pid`
      (or `myproc()` if `pid == 0`). Updates `p->priority = new_priority`,
      resets `p->wait_ticks = 0`, and returns previous priority.
   3. `get_priority(int pid)`:
      Looks up process by `pid` and returns its current `priority`.
   4. `update_proc_aging()`:
      Scans all processes in `proc[]`. For every process in `RUNNABLE` state:
      ```c
      acquire(&p->lock);
      if (p->state == RUNNABLE) {
        p->wait_ticks++;
        if (p->wait_ticks > 30) {
          if (p->priority > 1)
            p->priority--; // Promote priority
          p->wait_ticks = 0;
        }
      }
      release(&p->lock);
      ```
   5. `scheduler()`:
      Two-phase selection with Round-Robin tie-breaking:
      - Phase 1: Scan all `RUNNABLE` processes to find `min_prio` (numerically smallest).
      - Phase 2: Starting from `last_idx + 1` (circularly modulo `NPROC`), pick the
        first `RUNNABLE` process where `p->priority == min_prio`.
      - When scheduled, reset `p->wait_ticks = 0` and context switch via `swtch()`.

c) kernel/trap.c:
   Inside `clockintr()`:
   ```c
   if (cpuid() == 0) {
     acquire(&tickslock);
     ticks++;
     update_proc_aging(); // Update aging counters every tick on primary core
     wakeup(&ticks);
     release(&tickslock);
   }
   ```
   In `usertrap()` and `kerneltrap()`:
   `yield()` remains enabled on `which_dev == 2` to guarantee preemption.


5. SYSTEM CALL SPECIFICATIONS
--------------------------------------------------------------------------------
1. int set_priority(int pid, int priority)
   - Syscall Number: SYS_set_priority (23)
   - Parameters:
     * `pid`: Target process ID (if `pid == 0`, applies to calling process).
     * `priority`: Integer value between 1 and 20.
   - Return Value: Old priority (>= 0) on success, -1 on invalid argument or PID not found.

2. int get_priority(int pid)
   - Syscall Number: SYS_get_priority (24)
   - Parameters:
     * `pid`: Target process ID (if `pid == 0`, applies to calling process).
   - Return Value: Current priority on success, -1 on invalid PID.


6. USER TEST HARNESS WALKTHROUGH (user/priotest.c)
--------------------------------------------------------------------------------
1. Forks 4 CPU-bound child processes with distinct initial priorities:
   - Child 1: Priority 5 (High)
   - Child 2: Priority 8 (Medium-High)
   - Child 3: Priority 12 (Medium-Low)
   - Child 4: Priority 20 (Lowest)
2. Each child executes heavy computation and periodically reports its current priority
   via `get_priority()`.
3. Verifies that higher-priority processes (5, 8) finish first, while low-priority
   processes (12, 20) have their priority dynamically boosted via aging as
   `wait_ticks` exceeds 30, preventing starvation.


7. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/priority_aging

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ priotest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


8. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ priotest
============================================================
 Priority Preemptive Scheduler with Aging (Anti-Starvation) 
============================================================
Parent PID: 3
Forking 4 CPU-bound processes with priorities:
  Child 1: Priority 5 (High)
  Child 2: Priority 8 (Medium-High)
  Child 3: Priority 12 (Medium-Low)
  Child 4: Priority 20 (Lowest)

[Parent] All children forked. Waiting for completion...

[Child 1 | PID 4] Starting execution at tick 9 (Initial: 5, Current: 5)
[Child 1 | PID 4] 50% Workload Done (Initial: 5 -> Current: 5, tick: 18)
[Child 1 | PID 4] Workload Completed! (Initial: 5 -> Final: 5, tick: 27)
>> Completion Order 1: PID 4 (Child 1, Initial Priority 5, finished at tick 27)
[Child 2 | PID 5] Starting execution at tick 27 (Initial: 8, Current: 8)
[Child 2 | PID 5] 50% Workload Done (Initial: 8 -> Current: 8, tick: 37)
[Child 2 | PID 5] Workload Completed! (Initial: 8 -> Final: 8, tick: 46)
>> Completion Order 2: PID 5 (Child 2, Initial Priority 8, finished at tick 46)
[Child 3 | PID 6] Starting execution at tick 46 (Initial: 12, Current: 11)
[Child 3 | PID 6] 50% Workload Done (Initial: 12 -> Current: 11, tick: 55)
[Child 3 | PID 6] Workload Completed! (Initial: 12 -> Final: 11, tick: 64)
>> Completion Order 3: PID 6 (Child 3, Initial Priority 12, finished at tick 64)
[Child 4 | PID 7] Starting execution at tick 64 (Initial: 20, Current: 19)
[Child 4 | PID 7] 50% Workload Done (Initial: 20 -> Current: 19, tick: 73)
[Child 4 | PID 7] Workload Completed! (Initial: 20 -> Final: 19, tick: 82)
>> Completion Order 4: PID 7 (Child 4, Initial Priority 20, finished at tick 82)

============================================================
Test Summary:
1. Preemption & Priority: Higher priority processes (5, 8) completed first.
2. Anti-Starvation Aging: Lower priority processes (12, 20) had priority boosted
   dynamically as wait_ticks exceeded 30, preventing starvation.
============================================================

Analysis:
- Child 1 (Priority 5) finished at tick 27.
- Child 2 (Priority 8) finished at tick 46.
- Child 3 (Priority 12) waited while Child 1 and 2 ran; its wait_ticks exceeded 30,
  promoting its priority from 12 to 11 before it started!
- Child 4 (Priority 20) waited even longer; its wait_ticks exceeded 30, promoting
  its priority from 20 to 19.
- Perfect confirmation of priority scheduling and aging prevention of starvation.


9. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is priority represented such that 1 is highest and 20 is lowest?
A1: This convention mirrors Unix 'nice' levels and POSIX real-time priority classes
    where lower numerical values represent higher urgency or earlier rank.

Q2: Why must aging be called inside `clockintr()` under `tickslock`?
A2: `clockintr()` fires deterministically on every hardware timer interrupt. Calling
    `update_proc_aging()` on CPU 0 ensures wait times are updated synchronously with
    global tick progression without multi-core duplicate aging calls.

Q3: How are ties broken when two runnable processes have the identical priority?
A3: Using Round-Robin tie-breaking. The scheduler maintains a `last_idx` variable
    recording the index of the previously scheduled process, and scans circularly
    starting from `(last_idx + 1) % NPROC`.

Q4: What happens if a running process with priority 10 is executing and a new
    process with priority 5 becomes RUNNABLE?
A4: On the very next timer interrupt (~100ms), `usertrap()` calls `yield()`. The
    running process transitions to `RUNNABLE`. The scheduler runs, sees the new
    process has `min_prio = 5`, and preempts the old process to run the new one.

Q5: Why is `wait_ticks` reset to 0 when a process is scheduled to run?
A5: Because `wait_ticks` measures consecutive starvation time spent waiting in the
    ready queue without receiving CPU attention. Once scheduled, its starvation is
    relieved, so its timer resets.
================================================================================
