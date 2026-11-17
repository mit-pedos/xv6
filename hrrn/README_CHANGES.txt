================================================================================
OPERATING SYSTEM LAB ASSIGNMENT 3 - QUESTION 2 (SET 2)
Highest Response Ratio Next (HRRN) Non-Preemptive Scheduler
================================================================================

1. OVERVIEW
--------------------------------------------------------------------------------
This implementation introduces a non-preemptive Highest Response Ratio Next 
(HRRN) scheduler in xv6. HRRN balances the execution priority between short jobs 
and long jobs to eliminate starvation. For each RUNNABLE process, the response 
ratio is calculated using integer arithmetic:
    Response Ratio = ((Wait Time + Burst Time) * 100) / Burst Time
where Wait Time = current global ticks - creation time (ctime).
The scheduler dispatches the process with the highest response ratio and lets it 
run to completion without preemption.


2. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
- kernel/proc.h       : Added ctime and burst_time fields to struct proc
- kernel/defs.h       : Declared set_burst(int, int)
- kernel/proc.c       : Initialized ctime & burst_time in allocproc(), reset in freeproc(),
                        implemented set_burst(), and HRRN ratio-based scheduler()
- kernel/trap.c       : Disabled yield() on timer interrupts in usertrap() and
                        kerneltrap() to enforce non-preemption
- kernel/syscall.h    : Defined SYS_set_burst (23)
- kernel/syscall.c    : Wired sys_set_burst into syscall table
- kernel/sysproc.c    : Implemented sys_set_burst()
- user/user.h         : Declared set_burst(int, int) user function
- user/usys.pl        : Added entry("set_burst")
- Makefile            : Added $U/_hrrntest to UPROGS, set CPUS := 1,
                        cleared DETFLAGS for space-safe build path
- user/hrrntest.c     : [NEW] User test program verifying starvation prevention


3. DETAILED CODE CHANGES
--------------------------------------------------------------------------------
a) kernel/proc.h:
   - Added `uint ctime;` (process creation timestamp in ticks).
   - Added `int burst_time;` (declared expected burst time in ticks).

b) kernel/proc.c:
   - In `allocproc()`: Set `p->ctime = ticks;` and `p->burst_time = 0;`.
   - In `freeproc()`: Reset `p->ctime = 0;` and `p->burst_time = 0;`.
   - Implemented `int set_burst(int pid, int bt)`:
     Validates `bt > 0`, searches for process by PID, and sets `p->burst_time = bt;`.
   - In `scheduler()`:
     Reads current `ticks` and evaluates all `RUNNABLE` processes.
     Computes `w = ticks - p->ctime` and `ratio = ((w + bt) * 100) / bt`.
     Selects the process with the maximum ratio (`max_ratio`).

c) kernel/trap.c:
   - Commented out `yield()` on timer interrupts in `usertrap()` and `kerneltrap()`.
     This ensures selected processes run non-preemptively to completion.

d) System Calls:
   - Wired `set_burst(pid, bt)` through `syscall.h`, `syscall.c`, `sysproc.c`,
     `defs.h`, `user.h`, and `usys.pl`.

e) Makefile:
   - Set `CPUS := 1`, added `$U/_hrrntest\`, and cleared `DETFLAGS`.

f) user/hrrntest.c:
   - Forks 4 processes: P1(Long=80), P2(Short=10), P3(Short=10), P4(Med=30).
   - Short jobs complete early because their ratio increases quickly.
   - Long jobs wait, steadily increasing their Wait Time (W), which naturally 
     elevates their response ratio until they run and complete without starvation.


4. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd "/home/udit/Desktop/os lab/hrrn"

Step 2: Compile and launch xv6 inside QEMU:
        make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ hrrntest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


5. EXPECTED OUTCOME
--------------------------------------------------------------------------------
- Shorter jobs (P2, P3) are prioritized and complete first.
- Medium and long jobs have their response ratios elevated over time due to aging wait time.
- All processes complete, verifying that long jobs never starve indefinitely.

6. VERIFIED TEST OUTPUT LOG
--------------------------------------------------------------------------------
$ hrrntest
============================================================
  Highest Response Ratio Next (HRRN) Non-Preemptive Test    
============================================================
Parent PID: 3
Forking 4 processes:
  Child 1: Very Long Job (BT = 80 ticks)
  Child 2: Short Job     (BT = 10 ticks)
  Child 3: Short Job     (BT = 10 ticks)
  Child 4: Medium Job    (BT = 30 ticks)

[Parent] Forked Child 1 (PID 4, declared BT = 80) at tick 9
[Parent] Forked Child 2 (PID 5, declared BT = 10) at tick 9
[Parent] Forked Child 3 (PID 6, declared BT = 10) at tick 9
[Parent] Forked Child 4 (PID 7, declared BT = 30) at tick 9

[Parent] All 4 children forked. Waiting for completion...

[Child 2 | PID 5 | BT 10] -> STARTED workload at tick 9
[Child 2 | PID 5 | BT 10] -> 50% progress reached at tick 10
[Child 2 | PID 5 | BT 10] -> COMPLETED workload at tick 11
>> Completion Order 1: PID 5 (Child 2, Burst Time 10 ticks, finished at tick 12)
[Child 3 | PID 6 | BT 10] -> STARTED workload at tick 12
[Child 3 | PID 6 | BT 10] -> 50% progress reached at tick 13
[Child 3 | PID 6 | BT 10] -> COMPLETED workload at tick 14
>> Completion Order 2: PID 6 (Child 3, Burst Time 10 ticks, finished at tick 14)
[Child 4 | PID 7 | BT 30] -> STARTED workload at tick 14
[Child 4 | PID 7 | BT 30] -> 50% progress reached at tick 17
[Child 4 | PID 7 | BT 30] -> COMPLETED workload at tick 20
>> Completion Order 3: PID 7 (Child 4, Burst Time 30 ticks, finished at tick 21)
[Child 1 | PID 4 | BT 80] -> STARTED workload at tick 21
[Child 1 | PID 4 | BT 80] -> 50% progress reached at tick 27
[Child 1 | PID 4 | BT 80] -> COMPLETED workload at tick 34
>> Completion Order 4: PID 4 (Child 1, Burst Time 80 ticks, finished at tick 34)

--- HRRN Test Passed: Short jobs ran early; long jobs completed without starvation! ---
================================================================================
