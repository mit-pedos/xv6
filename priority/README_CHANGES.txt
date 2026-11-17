================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 7 - QUESTION 4 (Lab 7d)
Priority-Based Preemptive Scheduler with Aging (Anti-Starvation)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 7, Question 4 (7d) [Also referenced as Lab 3b]
- Topic          : Kernel Scheduling & Priority Management
- Test Program   : user/priotest.c
- Shell Command  : $ priotest

Objective:
Implement a Priority-Based Preemptive Scheduler with anti-starvation Aging in xv6:
1. Priority values: 1 (highest) to 20 (lowest). Default is 10.
2. System Calls:
   - `int set_priority(int pid, int priority)`: Sets priority of target process
     and resets its wait counter.
3. Preemptive Selection:
   Scheduler selects the `RUNNABLE` process with the lowest priority number. Ties
   are broken using round-robin order.
4. Starvation Prevention via Aging:
   If a `RUNNABLE` process waits for more than 30 ticks without CPU time, its
   priority number is decremented by 1 (promoting it towards 1), and its wait
   counter is reset.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Priority Inversion and Starvation:
   Without aging, low-priority processes can suffer indefinite postponement
   (starvation) if high-priority processes continually arrive or execute.
2. Aging Mechanism:
   Aging dynamically adjusts dynamic priority based on waiting duration:
   $Priority_{new} = \max(1, Priority_{old} - 1)$ when $WaitTicks > 30$.
3. Preemption:
   Timer interrupts continue to trigger `yield()` in `kernel/trap.c`. The
   scheduler always switches to higher priority jobs whenever they become ready.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Added int priority and uint wait_ticks to struct proc.
kernel/defs.h         | Modified | Declared set_priority() and update_proc_aging().
kernel/proc.c         | Modified | Initialized priority/wait_ticks; implemented
                      |          | set_priority(), update_proc_aging(), and scheduler().
kernel/trap.c         | Modified | Called update_proc_aging() inside clockintr() on CPU 0;
                      |          | retained timer yield() for preemptive scheduling.
kernel/syscall.h      | Modified | Defined SYS_set_priority (23).
kernel/syscall.c      | Modified | Registered sys_set_priority.
kernel/sysproc.c      | Modified | Implemented sys_set_priority() argument parsing.
user/user.h           | Modified | Declared set_priority(int, int) user function.
user/usys.pl          | Modified | Added entry("set_priority") to generate syscall assembly.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_priotest.
user/priotest.c       | Created  | Test program spawning 4 children with distinct priorities.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/proc.h:
   - `int priority;`: Integer priority between 1 and 20.
   - `uint wait_ticks;`: Waiting tick counter in RUNNABLE state.

b) kernel/proc.c:
   - In `allocproc()`: Set `p->priority = 10; p->wait_ticks = 0;`.
   - In `freeproc()`: Reset `p->priority = 10; p->wait_ticks = 0;`.
   - `int set_priority(int pid, int new_priority)`:
     Validates bounds `1 <= new_priority <= 20`, sets priority, and resets wait_ticks.
   - `void update_proc_aging(void)`:
     Iterates over `proc[]`. For every process in `RUNNABLE` state:
     Increments `wait_ticks`. If `wait_ticks > 30`, decrements `priority` (bounded at 1)
     and resets `wait_ticks = 0`.
   - In `scheduler()`:
     Finds `min_prio` among all `RUNNABLE` processes. Dispatches next process with
     `priority == min_prio` starting after `last_idx` (Round-Robin tie breaking).

c) kernel/trap.c:
   - In `clockintr()`: On CPU 0, calls `update_proc_aging()` on every timer interrupt.
   - In `usertrap()` and `kerneltrap()`: Calls `yield()` on timer interrupts (`which_dev == 2`).


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/priority

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ priotest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
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


7. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: What is the purpose of the aging mechanism in a priority scheduler?
A1: Aging prevents starvation (indefinite postponement) of low-priority processes
    by gradually increasing their priority the longer they wait in the ready queue.

Q2: How does preemption work in this priority scheduler?
A2: Timer interrupts call `yield()`. When the scheduler runs, it always picks the
    highest priority runnable job. If an arriving job has higher priority than the
    current running job, the running job yields and the new job takes over.

Q3: How are ties broken when multiple runnable processes share the highest priority?
A3: Using round-robin order. The scheduler tracks the index of the previously
    scheduled process and searches cyclically starting from `last_idx + 1`.

Q4: What is the difference between static priority and dynamic priority?
A4: Static priority remains unchanged throughout process lifetime. Dynamic priority
    is modified by the operating system at runtime (such as through aging or CPU
    burst history) to optimize turnaround time and ensure fairness.
================================================================================
