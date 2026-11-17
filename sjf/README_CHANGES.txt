================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 3 - QUESTION 3
Shortest Job First (SJF) Non-Preemptive Scheduler
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 3, Question 3 (3c)
- Topic          : CPU Scheduling Algorithms (Set 1)
- Test Program   : user/sjftest.c
- Shell Command  : $ sjftest

Objective:
Implement a Non-Preemptive Shortest Job First (SJF) scheduler in xv6:
1. Each process specifies its estimated CPU burst time using a new system call:
   `int set_burst(int pid, int burst_time)`.
2. The scheduler always dispatches the `RUNNABLE` process with the shortest
   declared burst time (`min burst_time`).
3. Execution is non-preemptive: once a process is scheduled, it runs to completion
   without timer preemption.
4. Processes that have not declared a burst time (default 0) are treated as lowest
   priority and run only when no declared jobs are waiting.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Optimality of SJF:
   - Mathematical Property: SJF is provably optimal in minimizing the average
     waiting time for a given set of stationary processes.
   - Proof Concept: By executing shorter jobs first, they release waiting time
     penalties earlier, reducing the sum of waiting times across all processes.
2. Non-Preemptive vs Preemptive:
   - Non-preemptive SJF: Once a process starts execution, it holds the CPU until it
     terminates or executes a blocking system call. Even if a shorter process
     arrives later, the currently running process is NOT preempted.
   - Preemptive SJF (SRTF): If a newly arrived process has a remaining execution
     time shorter than the current job's remaining time, the current job is preempted.
3. Burst Time Determination:
   - In real-world OS, future CPU burst length is estimated using exponential
     smoothing: $\tau_{n+1} = \alpha t_n + (1 - \alpha) \tau_n$.
   - In this assignment, processes declare their expected burst duration explicitly
     via `set_burst()`.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Added int burst_time to struct proc.
kernel/defs.h         | Modified | Declared set_burst(int, int).
kernel/proc.c         | Modified | Initialized burst_time=0 in allocproc(), reset in
                      |          | freeproc(), implemented set_burst(), and SJF
                      |          | selection loop in scheduler().
kernel/trap.c         | Modified | Commented out yield() on timer interrupts in
                      |          | usertrap() and kerneltrap() to enforce non-preemption.
kernel/syscall.h      | Modified | Defined SYS_set_burst (23).
kernel/syscall.c      | Modified | Registered sys_set_burst in syscalls table.
kernel/sysproc.c      | Modified | Implemented sys_set_burst() argument parsing.
user/user.h           | Modified | Declared int set_burst(int, int) user function.
user/usys.pl          | Modified | Added entry("set_burst") syscall stub generator.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_sjftest.
user/sjftest.c        | Created  | Test program spawning 4 children in unsorted order
                      |          | (40, 10, 30, 20) and verifying SJF completion order.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/proc.h:
   ```c
   struct proc {
     ...
     int burst_time;      // Declared CPU burst time in ticks (default = 0)
     ...
   };
   ```

b) kernel/proc.c:
   1. `allocproc()`:
      ```c
      p->burst_time = 0;  // 0 indicates undeclared / lowest priority
      ```
   2. `freeproc()`:
      ```c
      p->burst_time = 0;
      ```
   3. `set_burst(int pid, int bt)`:
      Validates that `bt >= 0`. Searches `proc[]` for `pid` (or current process
      if `pid == 0`), updates `p->burst_time = bt`, and returns 0 on success.
   4. `scheduler()`:
      Finds the `RUNNABLE` process with the lowest effective burst time:
      ```c
      struct proc *shortest_p = 0;
      int min_bt = 0x7fffffff;

      for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == RUNNABLE) {
          int eff_bt = (p->burst_time > 0) ? p->burst_time : 0x7fffffff;
          if (shortest_p == 0 || eff_bt < min_bt) {
            if (shortest_p)
              release(&shortest_p->lock);
            min_bt = eff_bt;
            shortest_p = p;
            continue; // keep lock held
          }
        }
        release(&p->lock);
      }

      if (shortest_p) {
        shortest_p->state = RUNNING;
        c->proc = shortest_p;
        swtch(&c->context, &shortest_p->context);
        c->proc = 0;
        release(&shortest_p->lock);
      }
      ```

c) kernel/trap.c:
   Enforcing Non-Preemption:
   Timer interrupts do NOT preempt:
   ```c
   // if(which_dev == 2)
   //   yield();
   ```


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
int set_burst(int pid, int burst_time)
- Syscall Number: SYS_set_burst (23)
- Parameters:
  * `pid`: Target process ID (if `pid == 0`, targets calling process).
  * `burst_time`: Non-negative integer representing estimated execution ticks.
- Return Value: 0 on success, -1 on invalid argument (`bt < 0`) or process not found.


6. USER TEST HARNESS WALKTHROUGH (user/sjftest.c)
--------------------------------------------------------------------------------
1. Spawns 4 children with deliberately unsorted burst times:
   - Arrival order: Child 1 (BT = 40), Child 2 (BT = 10), Child 3 (BT = 30), Child 4 (BT = 20).
2. Each child sets its burst time and performs heavy CPU computation.
3. The parent reaps children in a loop using `wait(0)` and records the completion order.
4. Expected SJF Completion Order:
   - Order 1: Child 2 (BT = 10 ticks)
   - Order 2: Child 4 (BT = 20 ticks)
   - Order 3: Child 3 (BT = 30 ticks)
   - Order 4: Child 1 (BT = 40 ticks)


7. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/sjf

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ sjftest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


8. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ sjftest
--- Non-preemptive Shortest Job First (SJF) Test ---
Parent PID: 3
Forking 4 processes with declared burst times (40, 10, 30, 20 ticks)...

[Parent] Forked Child 1 (PID 4) with declared burst time = 40
[Parent] Forked Child 2 (PID 5) with declared burst time = 10
[Parent] Forked Child 3 (PID 6) with declared burst time = 30
[Parent] Forked Child 4 (PID 7) with declared burst time = 20

[Parent] All children forked. Waiting for completion...

[Child 2 | PID 5] Declared burst time = 10 ticks (uptime: 2)
[Child 2 | PID 5 | BT 10] Completed workload at tick 3
>> Completion Order 1: PID 5 (Child 2, Declared Burst Time 10 ticks)
[Child 4 | PID 7] Declared burst time = 20 ticks (uptime: 3)
[Child 4 | PID 7 | BT 20] Completed workload at tick 5
>> Completion Order 2: PID 7 (Child 4, Declared Burst Time 20 ticks)
[Child 3 | PID 6] Declared burst time = 30 ticks (uptime: 5)
[Child 3 | PID 6 | BT 30] Completed workload at tick 6
>> Completion Order 3: PID 6 (Child 3, Declared Burst Time 30 ticks)
[Child 1 | PID 4] Declared burst time = 40 ticks (uptime: 6)
[Child 1 | PID 4 | BT 40] Completed workload at tick 8
>> Completion Order 4: PID 4 (Child 1, Declared Burst Time 40 ticks)

--- SJF Test Passed: Processes completed strictly in ascending order of declared burst time! ---

Analysis:
- Although Child 1 (PID 4) was created first, its burst time was 40 ticks.
- SJF correctly prioritized Child 2 (BT=10), then Child 4 (BT=20), then Child 3 (BT=30),
  and finally Child 1 (BT=40).
- The completion order perfectly matches ascending burst time.


9. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is SJF optimal for average waiting time?
A1: Mathematical scheduling theory proves that ordering non-preemptive jobs in
    increasing order of processing times minimizes $\sum_{i=1}^n (n - i + 1) \cdot p_i$,
    which is the total cumulative waiting time.

Q2: What is the main practical limitation of SJF scheduling in general-purpose OS?
A2: The OS cannot know the future CPU burst time of a process in advance. It must
    either rely on estimation (exponential smoothing of past bursts) or user/application
    hints.

Q3: Can non-preemptive SJF cause starvation?
A3: Yes. If short jobs continuously arrive in the ready queue, long jobs with larger
    burst times will wait indefinitely (starvation).

Q4: How does non-preemption affect scheduling when a very short job arrives while a
    long job is running?
A4: Because it is non-preemptive, the running long job continues to hold the CPU until
    it finishes or yields. The short job must wait until the current job ends. (In
    preemptive SRTF, the short job would preempt immediately).

Q5: Why are undeclared processes assigned an effective burst time of `0x7fffffff`?
A5: `0x7fffffff` is the maximum 32-bit signed integer. Assigning this ensures that
    system daemons and shell processes without declared bursts yield scheduling
    preference to test jobs, running only when no declared jobs are waiting.
================================================================================
