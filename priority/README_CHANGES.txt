================================================================================
OPERATING SYSTEM LAB ASSIGNMENT 3 - QUESTION 2
Priority-Based Preemptive Scheduler with Aging
================================================================================

1. OVERVIEW
--------------------------------------------------------------------------------
This implementation introduces a priority-based preemptive scheduler with starvation
prevention via an aging mechanism in xv6. Processes have priority values from 
1 (highest) to 20 (lowest). The scheduler always picks the lowest numerical 
priority value, breaking ties using Round-Robin. If a RUNNABLE process waits for 
more than 30 ticks without running, aging raises its priority by 1.


2. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
- kernel/proc.h       : Added priority (default 10) and wait_ticks to struct proc
- kernel/defs.h       : Declared set_priority() and update_proc_aging()
- kernel/proc.c       : Initialized priority & wait_ticks in allocproc(), reset in freeproc(),
                        implemented set_priority(), update_proc_aging(), and
                        Round-Robin tie-breaking priority scheduler()
- kernel/trap.c       : Called update_proc_aging() inside clockintr() on CPU 0;
                        kept timer yield() enabled for preemptive scheduling
- kernel/syscall.h    : Defined SYS_set_priority (23)
- kernel/syscall.c    : Wired sys_set_priority into syscalls table
- kernel/sysproc.c    : Implemented sys_set_priority() argument parsing
- user/user.h         : Declared set_priority(int, int) user function
- user/usys.pl        : Added entry("set_priority") to generate syscall assembly
- Makefile            : Added $U/_priotest to UPROGS, set CPUS := 1,
                        cleared DETFLAGS for space-safe build path
- user/priotest.c     : [NEW] User test program verifying priority order & aging


3. DETAILED CODE CHANGES
--------------------------------------------------------------------------------
a) kernel/proc.h:
   - Added `int priority;` (range 1–20, default 10).
   - Added `uint wait_ticks;` to count elapsed ticks waiting in RUNNABLE state.

b) kernel/proc.c:
   - In `allocproc()`: Set `p->priority = 10;` and `p->wait_ticks = 0;`.
   - In `freeproc()`: Reset `p->priority = 10;` and `p->wait_ticks = 0;`.
   - Implemented `int set_priority(int pid, int priority)`:
     Validates priority is between 1 and 20; searches process table by PID;
     updates priority and resets `wait_ticks = 0`; returns previous priority.
   - Implemented `void update_proc_aging(void)`:
     Iterates through `proc` table; for every process with `state == RUNNABLE`,
     increments `p->wait_ticks++`. If `wait_ticks > 30`, decrements `p->priority--`
     (raising priority, bounded at 1) and resets `wait_ticks = 0`.
   - In `scheduler()`:
     Finds minimum priority number (`min_prio`) among all `RUNNABLE` processes.
     Selects next `RUNNABLE` process with `priority == min_prio` starting from
     `last_idx + 1` (Round-Robin tie-breaking). Resets `wait_ticks = 0` upon running.

c) kernel/trap.c:
   - In `clockintr()`: Calls `update_proc_aging()` on CPU 0 every timer tick.
   - Kept `yield()` on timer interrupts in `usertrap()` and `kerneltrap()` so
     scheduling remains fully preemptive.

d) System Calls:
   - Wired `set_priority(pid, priority)` through `syscall.h`, `syscall.c`,
     `sysproc.c`, `user.h`, and `usys.pl`.

e) Makefile:
   - Set `CPUS := 1`, added `$U/_priotest\`, and cleared `DETFLAGS`.

f) user/priotest.c:
   - Forks 4 processes with distinct priorities: 5 (High), 8 (Med-High), 12 (Med-Low), 20 (Lowest).
   - Each process runs CPU-bound loops and logs progress.
   - Verifies high-priority processes finish first and low-priority process runs due to aging.


4. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd "/home/udit/Desktop/os lab/priority"

Step 2: Compile and launch xv6 inside QEMU:
        make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ priotest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


5. EXPECTED OUTCOME
--------------------------------------------------------------------------------
- Child with priority 5 finishes first.
- Child with priority 8 finishes second.
- Child with priority 12 finishes third.
- Child with lowest initial priority (20) gets scheduled via aging and finishes
  without indefinite starvation.
================================================================================
