================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 4 - QUESTION 1
Shortest Remaining Time First (SRTF) Preemptive Scheduler
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 4, Question 1 (4a)
- Topic          : CPU Scheduling Algorithms (Set 2)
- Test Program   : user/srtftest.c
- Shell Command  : $ srtftest

Objective:
Implement a preemptive Shortest Remaining Time First (SRTF) scheduler in xv6:
1. Each process declares its estimated total CPU burst duration using:
   `int set_burst(int pid, int burst_time)`.
2. The kernel tracks elapsed execution ticks and dynamic remaining execution time:
   `remaining_time = burst_time - rtime`.
3. Preemptive Scheduling:
   On every timer interrupt, the kernel checks whether another `RUNNABLE` process
   has a strictly smaller remaining time than the currently executing process.
   If so, the current process yields the CPU, allowing shorter newly arrived jobs
   to preempt longer running jobs.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Preemptive SJF (SRTF):
   - SRTF is the preemptive version of Shortest Job First.
   - Dispatch Rule: At any scheduling point $t$, choose process $p^*$ such that:
       $RemainingTime(p^*) = \min_{p \in RUNNABLE} (p->remaining\_time)$
   - Preemption Condition: If a new process $p_{new}$ arrives or transitions to
     `RUNNABLE` with $p_{new}->remaining\_time < p_{curr}->remaining\_time$,
     $p_{curr}$ is preempted immediately.
2. Advantages:
   - Yields the lowest possible average waiting time among all scheduling algorithms
     in the presence of dynamic process arrivals.
3. Drawbacks:
   - High context switching overhead if remaining times are close.
   - Long processes may suffer severe starvation if short processes keep arriving.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Added int burst_time and int remaining_time to
                      |          | struct proc.
kernel/defs.h         | Modified | Declared set_burst() and srtf_timer_tick().
kernel/proc.c         | Modified | Initialized burst/remaining times in allocproc();
                      |          | reset in freeproc(); implemented set_burst(),
                      |          | srtf_timer_tick(), and preemptive scheduler().
kernel/trap.c         | Modified | Called srtf_timer_tick() in usertrap() and
                      |          | kerneltrap() on timer interrupt to trigger yield().
kernel/syscall.h      | Modified | Defined SYS_set_burst (23).
kernel/syscall.c      | Modified | Registered sys_set_burst in syscalls table.
kernel/sysproc.c      | Modified | Implemented sys_set_burst() argument parsing.
user/user.h           | Modified | Declared int set_burst(int, int) user function.
user/usys.pl          | Modified | Added entry("set_burst") syscall stub generator.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_srtftest.
user/srtftest.c       | Created  | Test program spawning 3 staggered jobs demonstrating
                      |          | preemption and completion order.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/proc.h:
   ```c
   struct proc {
     ...
     int burst_time;       // Total declared execution ticks
     int remaining_time;   // Remaining ticks left to execute
     ...
   };
   ```

b) kernel/proc.c:
   1. `allocproc()`:
      ```c
      p->burst_time = 0;
      p->remaining_time = 0;
      ```
   2. `set_burst(int pid, int bt)`:
      Validates `bt >= 0`. Sets both `p->burst_time = bt;` and `p->remaining_time = bt;`.
   3. `srtf_timer_tick()`:
      Called on timer interrupt for the currently executing process:
      ```c
      int srtf_timer_tick(void) {
        struct proc *curr = myproc();
        if (curr && curr->state == RUNNING) {
          if (curr->remaining_time > 0)
            curr->remaining_time--;
          // Check if any other RUNNABLE process has shorter remaining time
          for (struct proc *p = proc; p < &proc[NPROC]; p++) {
            if (p != curr && p->state == RUNNABLE && p->remaining_time > 0) {
              if (p->remaining_time < curr->remaining_time)
                return 1; // Signal yield / preemption
            }
          }
        }
        return 0;
      }
      ```
   4. `scheduler()`:
      Finds the `RUNNABLE` process with minimum `remaining_time`. Unset processes
      default to `0x7fffffff`.

c) kernel/trap.c:
   In `usertrap()` and `kerneltrap()`:
   ```c
   if (which_dev == 2) {
     if (srtf_timer_tick())
       yield();
   }
   ```


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
int set_burst(int pid, int burst_time)
- Syscall Number: SYS_set_burst (23)
- Parameters:
  * `pid`: Target process ID (if `pid == 0`, targets calling process).
  * `burst_time`: Expected execution ticks.
- Return Value: 0 on success, -1 on failure.


6. USER TEST HARNESS WALKTHROUGH (user/srtftest.c)
--------------------------------------------------------------------------------
1. Spawns 3 processes with staggered arrivals:
   - Child 1: Long Job (BT = 50 ticks), starts at tick 2.
   - Child 2: Medium Job (BT = 20 ticks), arrives at tick 8 to preempt Child 1.
   - Child 3: Short Job (BT = 5 ticks), arrives at tick 14 to preempt Child 2.
2. Demonstrates true preemption:
   - Child 1 starts first.
   - When Child 2 arrives, Child 1 still has ~44 ticks remaining. Because 20 < 44,
     Child 2 preempts Child 1.
   - When Child 3 arrives, Child 2 still has ~14 ticks remaining. Because 5 < 14,
     Child 3 preempts Child 2.
   - Child 3 finishes first, followed by Child 2, followed by Child 1.


7. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/srtf

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ srtftest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


8. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ srtftest
============================================================
  Shortest Remaining Time First (SRTF) Preemptive Test      
============================================================
Parent PID: 3
Forking 3 processes with staggered arrival:
  Child 1: Long Job   (BT = 50 ticks)
  Child 2: Medium Job (BT = 20 ticks, arrives at tick +6 to PREEMPT Child 1)
  Child 3: Short Job  (BT = 5 ticks,  arrives at tick +12 to PREEMPT Child 2)

[Parent] Forked Child 1 (PID 4, declared BT = 50) at tick 2
[Child 1 | PID 4 | BT 50] -> STARTED workload at tick 2
[Parent] Forked Child 2 (PID 5, declared BT = 20) at tick 8
[Child 2 | PID 5 | BT 20] -> STARTED workload at tick 8
[Child 2 | PID 5 | BT 20] -> 50% progress reached at tick 12
[Parent] Forked Child 3 (PID 6, declared BT = 5) at tick 14

[Parent] All 3 children forked. Waiting for completion...

[Child 3 | PID 6 | BT 5] -> STARTED workload at tick 14
[Child 3 | PID 6 | BT 5] -> 50% progress reached at tick 15
[Child 3 | PID 6 | BT 5] -> COMPLETED workload at tick 16
>> Completion Order 1: PID 6 (Child 3, Burst Time 5 ticks, finished at tick 16)
[Child 2 | PID 5 | BT 20] -> COMPLETED workload at tick 19
>> Completion Order 2: PID 5 (Child 2, Burst Time 20 ticks, finished at tick 19)
[Child 1 | PID 4 | BT 50] -> 50% progress reached at tick 24
[Child 1 | PID 4 | BT 50] -> COMPLETED workload at tick 35
>> Completion Order 3: PID 4 (Child 1, Burst Time 50 ticks, finished at tick 35)

--- SRTF Test Passed: Short jobs preempted longer jobs and finished first! ---

Analysis:
- Child 1 started first at tick 2.
- Child 2 arrived at tick 8 and immediately preempted Child 1.
- Child 3 arrived at tick 14 and immediately preempted Child 2.
- Child 3 finished at tick 16; Child 2 resumed and finished at tick 19; Child 1
  resumed and finished at tick 35. Preemption chain succeeded flawlessly.


9. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: What is the fundamental difference between SJF and SRTF?
A1: SJF is non-preemptive: once a job starts, it runs to completion. SRTF is
    preemptive: when a job arrives with remaining time less than the current job's
    remaining time, the running job is preempted.

Q2: How is preemption triggered in xv6 for SRTF?
A2: On every timer interrupt in `kernel/trap.c`, `srtf_timer_tick()` decrements
    the running process's `remaining_time` and scans the process table. If any
    runnable process has a strictly smaller remaining time, it returns 1, triggering
    `yield()`.

Q3: Can SRTF lead to starvation? How?
A3: Yes. If a steady stream of short burst jobs keeps entering the system, long
    jobs with large remaining times will be continuously preempted and may never
    finish (starvation).

Q4: What happens if `remaining_time` reaches 0 but the process has not exited?
A4: `remaining_time` stops decrementing at 0 to prevent underflow, and the scheduler
    continues to let it run with high priority so it finishes its remaining CPU work.

Q5: Why does SRTF have higher overhead than FCFS or SJF?
A5: SRTF involves frequent evaluations on every timer interrupt and more frequent
    context switches (`swtch`) whenever a shorter job arrives.
================================================================================
