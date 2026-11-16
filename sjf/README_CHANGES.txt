================================================================================
OPERATING SYSTEM LAB ASSIGNMENT 3 - QUESTION 3
Shortest Job First (SJF) Non-Preemptive Scheduler
================================================================================

1. OVERVIEW
--------------------------------------------------------------------------------
This implementation introduces a non-preemptive Shortest Job First (SJF) scheduler 
in xv6 using an estimated burst time declared by the process itself. The scheduler 
always dispatches the RUNNABLE process with the smallest declared burst_time value 
and runs it to completion without timer preemption. Processes without a declared 
burst time (default 0 or -1) are treated as lowest priority (scheduled last).


2. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
- kernel/proc.h       : Added burst_time field to struct proc
- kernel/defs.h       : Declared set_burst(int, int)
- kernel/proc.c       : Initialized burst_time in allocproc(), reset in freeproc(),
                        implemented set_burst(), and non-preemptive SJF scheduler()
- kernel/trap.c       : Disabled yield() on timer interrupts in usertrap() and
                        kerneltrap() to enforce non-preemption
- kernel/syscall.h    : Defined SYS_set_burst (23)
- kernel/syscall.c    : Wired sys_set_burst into syscalls table
- kernel/sysproc.c    : Implemented sys_set_burst() argument parsing
- user/user.h         : Declared set_burst(int, int) user function
- user/usys.pl        : Added entry("set_burst") to generate syscall assembly
- Makefile            : Added $U/_sjftest to UPROGS, set CPUS := 1,
                        cleared DETFLAGS for space-safe build path
- user/sjftest.c      : [NEW] User test program verifying SJF execution order


3. DETAILED CODE CHANGES
--------------------------------------------------------------------------------
a) kernel/proc.h:
   - Added `int burst_time;` to `struct proc` (default 0).

b) kernel/proc.c:
   - In `allocproc()`: Set `p->burst_time = 0;` (unset / lowest priority).
   - In `freeproc()`: Reset `p->burst_time = 0;`.
   - Implemented `int set_burst(int pid, int bt)`:
     Validates `bt >= 0`, finds process by PID (or current process if PID == 0),
     and records `p->burst_time = bt;`.
   - In `scheduler()`:
     Scans all `RUNNABLE` processes. Effective burst time:
     `eff_bt = (p->burst_time > 0) ? p->burst_time : 0x7fffffff;`
     Always selects the process with the smallest `eff_bt`. Unset processes
     (`eff_bt = 0x7fffffff`) run only when no declared jobs are waiting.

c) kernel/trap.c:
   - Commented out `yield()` on timer interrupts in both `usertrap()` and `kerneltrap()`.
     This ensures that once a job is scheduled, it runs to completion without preemption.

d) System Calls:
   - Wired `set_burst(pid, bt)` through `syscall.h`, `syscall.c`, `sysproc.c`,
     `defs.h`, `user.h`, and `usys.pl`.

e) Makefile:
   - Set `CPUS := 1`, added `$U/_sjftest\`, and cleared `DETFLAGS`.

f) user/sjftest.c:
   - Forks 4 processes in unsorted order with declared burst times: 40, 10, 30, 20 ticks.
   - Each process calls `set_burst()` and performs CPU computation.
   - Parent collects processes and verifies they finish strictly in ascending order
     of declared burst times (10 -> 20 -> 30 -> 40).


4. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd "/home/udit/Desktop/os lab/sjf"

Step 2: Compile and launch xv6 inside QEMU:
        make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ sjftest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


5. EXPECTED OUTCOME
--------------------------------------------------------------------------------
Processes complete strictly in ascending order of declared burst time:
- 1st completed: Process with BT = 10 ticks
- 2nd completed: Process with BT = 20 ticks
- 3rd completed: Process with BT = 30 ticks
- 4th completed: Process with BT = 40 ticks
(Despite being forked in order 40, 10, 30, 20).
================================================================================
