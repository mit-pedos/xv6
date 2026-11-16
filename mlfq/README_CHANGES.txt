================================================================================
OPERATING SYSTEM LAB ASSIGNMENT 3 - QUESTION 3 (SET 2)
Multilevel Feedback Queue (MLFQ) Scheduler
================================================================================

1. OVERVIEW
--------------------------------------------------------------------------------
This implementation introduces a simplified Multilevel Feedback Queue (MLFQ) 
scheduler with 3 priority queues (levels 0, 1, 2) in xv6:
- Level 0 (Highest priority) : Time slice = 1 tick
- Level 1 (Medium priority)  : Time slice = 2 ticks
- Level 2 (Lowest priority)  : Time slice = 4 ticks

All newly created processes enter at Level 0. The scheduler strictly dispatches 
runnable processes from the highest non-empty queue (0 first, then 1, then 2), 
breaking ties within the same queue using Round-Robin. If a process uses its entire 
allotted time without yielding, it is demoted to the next lower queue. Processes 
that yield/sleep voluntarily (I/O-bound) preserve their high queue level.


2. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
- kernel/proc.h       : Added queue_level (0–2) and ticks_used fields to struct proc
- kernel/defs.h       : Declared mlfq_timer_tick() and get_queue(int)
- kernel/proc.c       : Initialized queue_level=0 & ticks_used=0 in allocproc(),
                        reset in freeproc() and voluntary sleep(), implemented
                        mlfq_timer_tick(), get_queue(), and 3-level strict queue scheduler()
- kernel/trap.c       : Updated usertrap() and kerneltrap() to invoke mlfq_timer_tick()
                        and call yield() only when a queue quantum expires
- kernel/syscall.h    : Defined SYS_get_queue (23)
- kernel/syscall.c    : Wired sys_get_queue into syscall table
- kernel/sysproc.c    : Implemented sys_get_queue()
- user/user.h         : Declared get_queue(int) user function
- user/usys.pl        : Added entry("get_queue")
- Makefile            : Added $U/_mlfqtest to UPROGS, set CPUS := 1,
                        cleared DETFLAGS for space-safe build path
- user/mlfqtest.c     : [NEW] User test program verifying CPU demotion vs I/O retention


3. DETAILED CODE CHANGES
--------------------------------------------------------------------------------
a) kernel/proc.h:
   - Added `int queue_level;` (0 = highest priority, 1 = medium, 2 = lowest).
   - Added `int ticks_used;` (ticks consumed during current quantum).

b) kernel/proc.c:
   - In `allocproc()`: Set `p->queue_level = 0; p->ticks_used = 0;`.
   - In `freeproc()`: Reset `p->queue_level = 0; p->ticks_used = 0;`.
   - In `sleep()`: On voluntary sleep/pause, reset `p->ticks_used = 0;` without 
     demoting `queue_level`, ensuring I/O tasks remain at level 0.
   - Implemented `int mlfq_timer_tick(void)`:
     Increments `p->ticks_used`. Checks quantum threshold (1 for L0, 2 for L1, 4 for L2).
     If quantum expires, resets `ticks_used = 0`, demotes `queue_level` (up to level 2),
     and returns 1 (signals to yield).
   - Implemented `int get_queue(int pid)`:
     Queries the `queue_level` of process with given PID (or current proc if pid == 0).
   - In `scheduler()`:
     Checks queues strictly from 0 to 2. Dispatches first runnable process found in 
     the highest non-empty queue, breaking ties with Round-Robin.

c) kernel/trap.c:
   - In `usertrap()` and `kerneltrap()`:
     On timer interrupt (`which_dev == 2`), calls `if (mlfq_timer_tick()) yield();`.
     Processes run undisturbed until their specific queue quantum expires.

d) System Calls:
   - Wired `get_queue(pid)` through `syscall.h`, `syscall.c`, `sysproc.c`,
     `defs.h`, `user.h`, and `usys.pl`.

e) Makefile:
   - Set `CPUS := 1`, added `$U/_mlfqtest\`, and cleared `DETFLAGS`.

f) user/mlfqtest.c:
   - Forks a CPU-bound child (continuous computation) and an I/O-bound child (calls pause()).
   - Directly checks and prints `get_queue()` across milestones:
     * CPU-bound child starts at level 0, demotes to level 1, then sinks to level 2.
     * I/O-bound child yields voluntarily and retains top priority (level 0).


4. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd "/home/udit/Desktop/os lab/mlfq"

Step 2: Compile and launch xv6 inside QEMU:
        make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ mlfqtest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


5. EXPECTED OUTCOME
--------------------------------------------------------------------------------
- CPU-bound process sinks across steps: Level 0 -> Level 1 -> Level 2.
- I/O-bound process periodically pauses/yields and maintains Level 0 throughout.
================================================================================
