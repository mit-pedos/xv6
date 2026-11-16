================================================================================
OPERATING SYSTEM LAB ASSIGNMENT 3 - QUESTION 1
First-Come, First-Served (FCFS) Non-Preemptive Scheduler
================================================================================

1. OVERVIEW
--------------------------------------------------------------------------------
This implementation replaces the default xv6 preemptive Round-Robin scheduler 
with a non-preemptive First-Come, First-Served (FCFS) scheduler. The scheduler 
always dispatches the RUNNABLE process with the earliest creation timestamp (ctime) 
and allows it to run to completion without timer preemption.


2. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
- kernel/proc.h       : Added creation time (ctime) to struct proc
- kernel/proc.c       : Recorded ctime in allocproc(), reset in freeproc(),
                        implemented FCFS selection in scheduler()
- kernel/trap.c       : Disabled yield() on timer interrupts in usertrap()
                        and kerneltrap() to enforce non-preemption
- kernel/syscall.h    : Documented FCFS configuration
- kernel/syscall.c    : Documented FCFS configuration
- kernel/sysproc.c    : Documented FCFS configuration
- Makefile            : Added $U/_fcfstest to UPROGS, set CPUS := 1,
                        cleared DETFLAGS for space-safe build path
- user/fcfstest.c     : [NEW] User test program verifying creation vs completion order


3. DETAILED CODE CHANGES
--------------------------------------------------------------------------------
a) kernel/proc.h:
   - Added `uint ctime;` to `struct proc` to track the creation tick of each process.

b) kernel/proc.c:
   - In `allocproc()`:
     Acquired `tickslock` and set `p->ctime = ticks;` when allocating process state.
   - In `freeproc()`:
     Reset `p->ctime = 0;` when cleaning up a process.
   - In `scheduler()`:
     Replaced Round-Robin loop. It now scans all processes with `state == RUNNABLE`,
     finds the one with the smallest `ctime` value, and switches context to run it.

c) kernel/trap.c:
   - In `usertrap()`:
     Commented out `yield()` on timer interrupts (`which_dev == 2`).
   - In `kerneltrap()`:
     Commented out `yield()` on timer interrupts (`which_dev == 2`).
     This ensures running processes are never preempted by the timer.

d) Makefile:
   - Set `CPUS := 1` so scheduling ordering is strictly observed on a single CPU.
   - Added `$U/_fcfstest\` to `UPROGS` list.
   - Cleared `DETFLAGS` to support paths containing spaces.

e) user/fcfstest.c:
   - Staggers creation of 3 CPU-bound child processes using delay loops.
   - Each child performs heavy arithmetic computation.
   - Parent collects them with `wait()` and logs the completion order.


4. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd "/home/udit/Desktop/os lab/fcfs"

Step 2: Compile and launch xv6 inside QEMU:
        make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ fcfstest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


5. EXPECTED OUTCOME
--------------------------------------------------------------------------------
Processes complete strictly in the order they were created:
Child 1 (earliest ctime) finishes first, followed by Child 2, then Child 3.
================================================================================
