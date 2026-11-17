================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 4 - QUESTION 3
Multilevel Feedback Queue (MLFQ) Scheduler
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 4, Question 3 (4c)
- Topic          : CPU Scheduling Algorithms (Set 2)
- Test Program   : user/mlfqtest.c
- Shell Command  : $ mlfqtest

Objective:
Implement a 3-level Multilevel Feedback Queue (MLFQ) scheduler in xv6:
1. Priority Queues & Time Slices:
   - Queue 0 (Top Priority) : Time slice = 1 timer tick.
   - Queue 1 (Medium Priority): Time slice = 2 timer ticks.
   - Queue 2 (Lowest Priority): Time slice = 4 timer ticks.
2. Queue Rules:
   - Rule 1: All newly created processes enter at Queue 0.
   - Rule 2: If Queue 0 has runnable processes, run them. If empty, run Queue 1.
     If Queue 1 is empty, run Queue 2.
   - Rule 3: Within the same queue level, Round-Robin scheduling is used.
   - Rule 4: If a process exhausts its allotted time slice without yielding (CPU-bound),
     it is demoted to the next lower queue level ($Q_{i} \to Q_{i+1}$).
   - Rule 5: If a process yields the CPU voluntarily before its quantum expires
     (e.g., for I/O or sleep), it retains its current priority queue level.
3. System Call:
   `int get_queue(int pid)`: Allows user programs to query the current queue level
   of a process.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Why MLFQ?
   - In most systems, the OS does not know whether a job is I/O-bound or CPU-bound
     in advance.
   - MLFQ learns from past behavior:
     * Interactive / I/O-bound jobs perform short bursts and yield often $\implies$
       they remain in high-priority queues with rapid response times.
     * Batch / CPU-bound jobs consume full quanta $\implies$ they sink to lower
       queues with longer time slices, minimizing context-switching overhead.
2. Anti-Starvation & Queue Demotion:
   - Time-slice tracking: `ticks_used` increments on every clock tick.
   - Demotion occurs only when `ticks_used >= quantum[queue_level]`.
   - Voluntary sleep resets `ticks_used = 0` without demoting `queue_level`.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Added int queue_level and int ticks_used to
                      |          | struct proc.
kernel/defs.h         | Modified | Declared mlfq_timer_tick() and get_queue(int).
kernel/proc.c         | Modified | Initialized queue_level=0 & ticks_used=0 in
                      |          | allocproc(); reset in freeproc() and sleep();
                      |          | implemented mlfq_timer_tick(), get_queue(),
                      |          | and 3-level strict queue scheduler().
kernel/trap.c         | Modified | Called mlfq_timer_tick() on timer interrupt to
                      |          | selectively yield when quantum expires.
kernel/syscall.h      | Modified | Defined SYS_get_queue (23).
kernel/syscall.c      | Modified | Registered sys_get_queue in syscall table.
kernel/sysproc.c      | Modified | Implemented sys_get_queue() argument parsing.
user/user.h           | Modified | Declared int get_queue(int) user function.
user/usys.pl          | Modified | Added entry("get_queue") syscall stub generator.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_mlfqtest.
user/mlfqtest.c       | Created  | Test program spawning CPU-bound and I/O-bound
                      |          | processes and asserting queue demotion vs retention.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/proc.h:
   ```c
   struct proc {
     ...
     int queue_level;    // Priority queue (0 = top, 1 = med, 2 = lowest)
     int ticks_used;     // Ticks consumed during the current queue quantum
     ...
   };
   ```

b) kernel/proc.c:
   1. `allocproc()`:
      ```c
      p->queue_level = 0; // Rule 1: Starts at top queue
      p->ticks_used = 0;
      ```
   2. `sleep()`:
      When a process blocks voluntarily (I/O burst), reset `ticks_used = 0`
      without demoting:
      ```c
      p->ticks_used = 0;
      ```
   3. `mlfq_timer_tick()`:
      Called on timer interrupt:
      ```c
      int mlfq_timer_tick(void) {
        struct proc *p = myproc();
        if (!p || p->state != RUNNING) return 0;

        p->ticks_used++;
        int quantum = (p->queue_level == 0) ? 1 :
                      (p->queue_level == 1) ? 2 : 4;

        if (p->ticks_used >= quantum) {
          p->ticks_used = 0;
          if (p->queue_level < 2)
            p->queue_level++; // Demote CPU-bound job
          return 1; // Signal preemption / yield
        }
        return 0; // Quantum not yet expired
      }
      ```
   4. `scheduler()`:
      Strict priority hierarchy over queues 0, 1, and 2:
      ```c
      for (int q = 0; q <= 2; q++) {
        for (p = proc; p < &proc[NPROC]; p++) {
          acquire(&p->lock);
          if (p->state == RUNNABLE && p->queue_level == q) {
            p->state = RUNNING;
            c->proc = p;
            swtch(&c->context, &p->context);
            c->proc = 0;
            release(&p->lock);
            goto found_process;
          }
          release(&p->lock);
        }
      }
      found_process: ;
      ```

c) kernel/trap.c:
   Timer interrupt handler only yields when the queue quantum expires:
   ```c
   if (which_dev == 2) {
     if (mlfq_timer_tick())
       yield();
   }
   ```


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
int get_queue(int pid)
- Syscall Number: SYS_get_queue (23)
- Parameters:
  * `pid`: Target PID (if `pid == 0`, targets calling process).
- Return Value: Queue level (0, 1, or 2) on success, -1 on failure.


6. USER TEST HARNESS WALKTHROUGH (user/mlfqtest.c)
--------------------------------------------------------------------------------
1. Spawns two child processes:
   - Child 1 (CPU-bound): Executes heavy continuous math loops without sleeping.
   - Child 2 (I/O-bound): Executes short compute bursts punctuated by `pause(1)`
     (voluntary sleep/yield).
2. Verifies the MLFQ learning invariant:
   - CPU-bound child starts at Level 0, demotes to Level 1 after 1 tick, and sinks
     to Level 2 after 2 more ticks.
   - I/O-bound child yields voluntarily before exhausting its 1-tick quantum,
     retaining top priority at Level 0 throughout its lifecycle.


7. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/mlfq

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ mlfqtest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


8. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ mlfqtest
--- Multilevel Feedback Queue (MLFQ) Scheduler Test ---
Parent PID: 3
Forking CPU-bound process (demotes to level 2) and I/O-bound process (remains at level 0)...

[CPU-bound | PID 4] Initial Queue Level: 0 (uptime: 2)
[I/O-bound | PID 5] Initial Queue Level: 0 (uptime: 3)
[CPU-bound | PID 4] After Step 1 computation: Queue Level = 1 (uptime: 3)
[CPU-bound | PID 4] After Step 2 computation: Queue Level = 1 (uptime: 4)
[I/O-bound | PID 5] After Step 1 I/O burst: Queue Level = 0 (remains at high priority!)
[CPU-bound | PID 4] After Step 3 computation: Queue Level = 2 (uptime: 5)
[CPU-bound | PID 4] Final Queue Level = 2 (successfully sank to lowest queue!)
[I/O-bound | PID 5] After Step 2 I/O burst: Queue Level = 0 (remains at high priority!)
[I/O-bound | PID 5] After Step 3 I/O burst: Queue Level = 0 (remains at high priority!)
[I/O-bound | PID 5] After Step 4 I/O burst: Queue Level = 0 (remains at high priority!)
[I/O-bound | PID 5] After Step 5 I/O burst: Queue Level = 0 (remains at high priority!)
[I/O-bound | PID 5] Final Queue Level = 0 (retained top priority!)

--- MLFQ Test Passed: CPU-bound sank to lowest queue; I/O process remained in top queue! ---

Analysis:
- PID 4 (CPU-bound) starts at Queue 0, sinks to Queue 1, and eventually settles at Queue 2.
- PID 5 (I/O-bound) stays at Queue 0 across all 5 observation steps.
- Behavior confirms proper distinction between CPU-bound and interactive processes.


9. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: What are the main rules governing an MLFQ scheduler?
A1: (1) Newly created processes enter the highest priority queue (Q0).
    (2) High priority queues preempt lower ones.
    (3) Jobs within the same queue are scheduled via Round-Robin.
    (4) A process that exhausts its time slice is demoted to a lower queue.
    (5) A process that yields before its time slice expires maintains its queue.

Q2: Why do higher queues have shorter time slices and lower queues longer time slices?
A2: Higher queues cater to interactive jobs where low response time is critical.
    Lower queues cater to long CPU-bound jobs where longer time slices reduce context
    switch overhead and improve CPU throughput.

Q3: What is "gaming the scheduler" in MLFQ?
A3: If a malicious process runs for 99% of its time slice and then issues an I/O
    request just before the quantum ends, it keeps yielding without demotion,
    unfairly monopolizing high CPU share in the top queue.

Q4: What is Priority Boost and why is it used in production MLFQ schedulers?
A4: Priority Boost periodically (e.g. every $S$ seconds) moves all processes back
    to the highest queue. This resets game players and guarantees that CPU-bound
    processes do not starve if new interactive jobs arrive continuously.

Q5: Why did we reset `ticks_used = 0` inside `sleep()`?
A5: Because when a process sleeps voluntarily for I/O, it relinquishes the CPU
    before its quantum expires. Resetting `ticks_used` ensures it gets a full fresh
    quantum when it wakes up.
================================================================================
