================================================================================
OPERATING SYSTEM LAB ASSIGNMENT 3 - QUESTION 1 (SET 2)
Shortest Remaining Time First (SRTF) Preemptive Scheduler
================================================================================

1. OVERVIEW
--------------------------------------------------------------------------------
This implementation introduces a preemptive Shortest Remaining Time First (SRTF) 
scheduler in xv6 using estimated burst times. The scheduler always dispatches 
the RUNNABLE process with the smallest remaining execution time (remaining_time).
On every timer interrupt in trap.c, the running process has its remaining_time 
decremented. If another RUNNABLE process has a strictly smaller remaining_time, 
the currently running process yields the CPU, allowing shorter newly arrived jobs 
to preempt longer-running jobs.


2. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
- kernel/proc.h       : Added burst_time and remaining_time fields to struct proc
- kernel/defs.h       : Declared set_burst() and srtf_timer_tick()
- kernel/proc.c       : Initialized fields in allocproc(), reset in freeproc(),
                        implemented set_burst(), srtf_timer_tick(), and
                        preemptive SRTF scheduler()
- kernel/trap.c       : Updated usertrap() and kerneltrap() to call srtf_timer_tick()
                        and call yield() when a shorter job is waiting
- kernel/syscall.h    : Defined SYS_set_burst (23)
- kernel/syscall.c    : Wired sys_set_burst into syscall table
- kernel/sysproc.c    : Implemented sys_set_burst()
- user/user.h         : Declared set_burst(int, int) user function
- user/usys.pl        : Added entry("set_burst")
- Makefile            : Added $U/_srtftest to UPROGS, set CPUS := 1,
                        cleared DETFLAGS for space-safe build path
- user/srtftest.c     : [NEW] User test program verifying preemption of long jobs


3. DETAILED CODE CHANGES
--------------------------------------------------------------------------------
a) kernel/proc.h:
   - Added `int burst_time;` (declared expected ticks).
   - Added `int remaining_time;` (remaining execution ticks).

b) kernel/proc.c:
   - In `allocproc()`: Set `p->burst_time = 0; p->remaining_time = 0;`.
   - In `freeproc()`: Reset `p->burst_time = 0; p->remaining_time = 0;`.
   - Implemented `int set_burst(int pid, int bt)`:
     Validates `bt >= 0`, finds process by PID (or current process if pid == 0),
     and initializes both `p->burst_time = bt;` and `p->remaining_time = bt;`.
   - Implemented `int srtf_timer_tick(void)`:
     Decrements `myproc()->remaining_time` by 1.
     Scans the process table: if any other `RUNNABLE` process has a strictly 
     smaller `remaining_time`, returns 1 (signals to yield).
   - In `scheduler()`:
     Picks the `RUNNABLE` process with the lowest `remaining_time` (where unset
     processes default to 0x7fffffff).

c) kernel/trap.c:
   - In `usertrap()` and `kerneltrap()`:
     On timer interrupt (`which_dev == 2`), calls `if (srtf_timer_tick()) yield();`.
     This directly implements preemptive context switches whenever a shorter job arrives.

d) System Calls:
   - Wired `set_burst(pid, bt)` through `syscall.h`, `syscall.c`, `sysproc.c`,
     `defs.h`, `user.h`, and `usys.pl`.

e) Makefile:
   - Set `CPUS := 1`, added `$U/_srtftest\`, and cleared `DETFLAGS`.

f) user/srtftest.c:
   - Forks 3 processes with staggered arrival: P1(Long=30), P2(Med=15), P3(Short=5).
   - P1 starts executing first. P2 arrives with smaller remaining time and preempts P1.
   - P3 arrives with even smaller remaining time and preempts P2.
   - P3 finishes first, P2 finishes second, and P1 finishes last.


4. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd "/home/udit/Desktop/os lab/srtf"

Step 2: Compile and launch xv6 inside QEMU:
        make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ srtftest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


5. EXPECTED OUTCOME
--------------------------------------------------------------------------------
- Child 3 (Short, BT=5) completes 1st.
- Child 2 (Medium, BT=15) completes 2nd.
- Child 1 (Long, BT=30) completes 3rd.
Newly arrived shorter jobs successfully preempt longer running jobs.
================================================================================
