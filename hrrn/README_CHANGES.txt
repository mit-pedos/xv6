================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 4 - QUESTION 2
Highest Response Ratio Next (HRRN) Non-Preemptive Scheduler
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 4, Question 2 (4b)
- Topic          : CPU Scheduling Algorithms (Set 2)
- Test Program   : user/hrrntest.c
- Shell Command  : $ hrrntest

Objective:
Implement a non-preemptive Highest Response Ratio Next (HRRN) scheduler in xv6:
1. Eliminates the starvation problem of SJF while retaining its bias towards short jobs.
2. For each `RUNNABLE` process, dynamically calculates the Response Ratio:
   $$R = \frac{W + S}{S} = 1 + \frac{W}{S}$$
   where $W$ is the waiting time in the ready queue, and $S$ is the estimated service
   (burst) time declared via `set_burst(int pid, int burst_time)`.
3. The scheduler selects the process with the maximum response ratio.
4. Execution is non-preemptive: selected processes run to completion without timer preemption.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Response Ratio Mechanics:
   - When a job first arrives, $W = 0 \implies R = 1.0$.
   - For shorter jobs ($S$ is small), $W / S$ increases rapidly as it waits, giving
     it high priority quickly.
   - For longer jobs ($S$ is large), $W / S$ grows more slowly, but as $W$ grows
     large enough, its response ratio eventually overcomes new short arrivals.
   - Hence, HRRN mathematically prevents starvation without requiring a separate
     arbitrary aging parameter.
2. Kernel Fixed-Point Arithmetic:
   - The xv6 kernel operates without floating-point hardware support.
   - To compute ratios accurately using integers, the ratio is scaled by 100:
     $$Ratio = \frac{(W + S) \times 100}{S}$$


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Added uint ctime and int burst_time to struct proc.
kernel/defs.h         | Modified | Declared set_burst(int, int).
kernel/proc.c         | Modified | Initialized ctime=ticks in allocproc(); reset in
                      |          | freeproc(); implemented set_burst() and HRRN
                      |          | selection loop in scheduler().
kernel/trap.c         | Modified | Commented out timer yield() in usertrap() and
                      |          | kerneltrap() to enforce non-preemptive execution.
kernel/syscall.h      | Modified | Defined SYS_set_burst (23).
kernel/syscall.c      | Modified | Registered sys_set_burst in syscalls table.
kernel/sysproc.c      | Modified | Implemented sys_set_burst() argument parsing.
user/user.h           | Modified | Declared int set_burst(int, int) user function.
user/usys.pl          | Modified | Added entry("set_burst") syscall stub generator.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_hrrntest.
user/hrrntest.c       | Created  | Test program spawning 4 processes with varied burst
                      |          | times (80, 10, 10, 30) demonstrating starvation prevention.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/proc.h:
   ```c
   struct proc {
     ...
     uint ctime;         // Process creation tick timestamp
     int burst_time;     // Declared burst time (service time S)
     ...
   };
   ```

b) kernel/proc.c:
   1. `allocproc()`:
      ```c
      acquire(&tickslock);
      p->ctime = ticks;
      release(&tickslock);
      p->burst_time = 0;
      ```
   2. `set_burst(int pid, int bt)`:
      Sets `p->burst_time = bt` for target `pid` (or current proc if `pid == 0`).
   3. `scheduler()`:
      Evaluates all `RUNNABLE` processes:
      ```c
      struct proc *best_p = 0;
      int max_ratio = -1;

      acquire(&tickslock);
      uint cur_ticks = ticks;
      release(&tickslock);

      for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == RUNNABLE) {
          int bt = (p->burst_time > 0) ? p->burst_time : 10;
          int w = (cur_ticks >= p->ctime) ? (cur_ticks - p->ctime) : 0;
          int ratio = ((w + bt) * 100) / bt;

          if (best_p == 0 || ratio > max_ratio) {
            if (best_p)
              release(&best_p->lock);
            max_ratio = ratio;
            best_p = p;
            continue;
          }
        }
        release(&p->lock);
      }
      ```

c) kernel/trap.c:
   Non-preemption is enforced by commenting out `yield()` on timer interrupts:
   ```c
   // if(which_dev == 2)
   //   yield();
   ```


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
int set_burst(int pid, int burst_time)
- Syscall Number: SYS_set_burst (23)
- Parameters:
  * `pid`: Target PID (0 = calling process).
  * `burst_time`: Declared burst time in ticks.
- Return Value: 0 on success, -1 on invalid argument or process not found.


6. USER TEST HARNESS WALKTHROUGH (user/hrrntest.c)
--------------------------------------------------------------------------------
1. Forks 4 processes simultaneously:
   - Child 1: Very Long Job (BT = 80 ticks)
   - Child 2: Short Job     (BT = 10 ticks)
   - Child 3: Short Job     (BT = 10 ticks)
   - Child 4: Medium Job    (BT = 30 ticks)
2. At the beginning, all have $W = 0$. Short jobs (Child 2 and 3) have the smallest
   denominators, so their response ratios escalate quickly.
3. Child 2 and Child 3 run first.
4. Meanwhile, Child 4 (Medium) and Child 1 (Long) accumulate waiting time $W$.
5. Child 4 runs next. Finally, Child 1's wait time has grown so large that its
   ratio surpasses any newly arriving job, ensuring it runs to completion without
   starvation.


7. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/hrrn

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ hrrntest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


8. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
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

Analysis:
- Child 2 (PID 5, BT=10) and Child 3 (PID 6, BT=10) completed first at ticks 12 and 14.
- Child 4 (PID 7, BT=30) completed next at tick 21.
- Child 1 (PID 4, BT=80), despite having a massive 80-tick burst, accumulated
  wait time until tick 21 ($W = 12$), raising its ratio and allowing it to finish
  at tick 34 with zero starvation.


9. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: What is the Response Ratio formula in HRRN?
A1: $R = \frac{W + S}{S}$, where $W$ is waiting time and $S$ is service (burst) time.

Q2: Why does HRRN prevent starvation?
A2: Because waiting time $W$ is in the numerator. As a long process waits in the
    RUNNABLE state, $W$ monotonically increases. Eventually, $\frac{W + S}{S}$ grows
    large enough that the long job's ratio exceeds that of newly arriving short jobs.

Q3: Is HRRN preemptive or non-preemptive?
A3: Classical HRRN is non-preemptive. Once a job is chosen, it runs to completion.
    This avoids frequent context switches while still preventing starvation.

Q4: How does HRRN compare to SJF and FCFS?
A4: When all $W \approx 0$, HRRN behaves like SJF (shorter jobs have higher ratio).
    When $W$ is very large relative to $S$, HRRN behaves like FCFS (older arrivals
    dominate). Thus, it combines the advantages of both.

Q5: Why did we scale the ratio by 100 in the kernel?
A5: The RISC-V kernel does not use floating-point math. Integer division `(W + S) / S`
    would truncate fractions, losing granularity. Scaling by 100 preserves 2 decimal
    places of precision.
================================================================================
