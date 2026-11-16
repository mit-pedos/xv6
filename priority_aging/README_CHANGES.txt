================================================================================
Priority-Based Preemptive Scheduler with Aging in xv6
================================================================================

1. OVERVIEW & OBJECTIVES
--------------------------------------------------------------------------------
This implementation replaces the default xv6 round-robin scheduler with a
Priority-Based Preemptive Scheduler with Anti-Starvation Aging:
a) Priorities range from 1 (highest) to 20 (lowest). Default priority is 10.
b) System calls:
   - int set_priority(int pid, int priority): Sets the priority of the target
     process (or current process if pid == 0) and resets its wait counter.
   - int get_priority(int pid): Returns the current effective priority.
c) Preemptive Scheduling:
   scheduler() always selects the RUNNABLE process with the lowest priority
   number (highest priority). Ties are broken using Round-Robin order.
d) Anti-Starvation Aging:
   Every clock tick (clockintr()), wait_ticks is incremented for all RUNNABLE
   processes. If any RUNNABLE process waits for more than 30 ticks without
   running (wait_ticks > 30), its priority number is decremented by 1 (raising
   its priority, bounded at 1), and its wait_ticks is reset to 0. When a process
   is scheduled, its wait_ticks is also reset to 0.

2. FILES MODIFIED & ADDED
--------------------------------------------------------------------------------
- kernel/proc.h:
  * Added int priority (1 to 20, default 10) to struct proc.
  * Added uint wait_ticks to track time spent waiting in RUNNABLE state.
- kernel/proc.c:
  * allocproc(): Initialized p->priority = 10, p->wait_ticks = 0.
  * freeproc(): Reset p->priority = 10, p->wait_ticks = 0.
  * set_priority(pid, new_prio): Validates priority bounds (1..20), sets
    priority, and resets wait_ticks.
  * get_priority(pid): Looks up and returns process priority.
  * update_proc_aging(): Scans proc table, increments wait_ticks for RUNNABLE
    processes, decrements priority number when wait_ticks > 30, and resets wait_ticks.
  * scheduler(): Scans all RUNNABLE processes to find the minimum priority number,
    then round-robin selects the next RUNNABLE process with that minimum priority,
    resets its wait_ticks to 0, and context-switches to it.
- kernel/trap.c:
  * In clockintr() on CPU 0: Calls update_proc_aging() on every timer interrupt.
- kernel/syscall.h & kernel/syscall.c:
  * Registered SYS_set_priority (23) and SYS_get_priority (24).
- kernel/sysproc.c:
  * Added sys_set_priority() and sys_get_priority().
- kernel/defs.h:
  * Declared set_priority, get_priority, and update_proc_aging.
- user/user.h & user/usys.pl:
  * Exported set_priority() and get_priority() to user space.
- Makefile:
  * Set DETFLAGS = empty to support paths containing spaces.
  * Set CPUS := 1 to demonstrate clean single-core scheduling order.
  * Added $U/_priotest to UPROGS.
- user/priotest.c:
  * Forks 4 processes with initial priorities: 5 (High), 8 (Med-High),
    12 (Med-Low), and 20 (Lowest).
  * Measures execution progress, reports current priority during execution
    (showing dynamic aging up from 12 -> 11 and 20 -> 19), and confirms strict
    priority completion order.

3. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
From this directory:
    $ make qemu

Inside xv6 shell:
    $ priotest

To exit QEMU:
    Press Ctrl-A then x

4. VERIFIED TEST OUTPUT LOG
--------------------------------------------------------------------------------
$ priotest
============================================================
 Priority Preemptive Scheduler with Aging (Anti-Starvation) 
============================================================
Parent PID: 3
Forking 4 CPU-bound processes with priorities:
  Child 1: Priority 5 (High)
  Child 2: Priority 8 (Medium-High)
  Child 3: Priority 12 (Medium-Low)
  Child 4: Priority 20 (Lowest)

[Parent] All children forked. Waiting for completion...

[Child 1 | PID 4] Starting execution at tick 2 (Initial: 5, Current: 5)
[Child 1 | PID 4] 50% Workload Done (Initial: 5 -> Current: 5, tick: 11)
[Child 1 | PID 4] Workload Completed! (Initial: 5 -> Final: 5, tick: 21)
[Child 2 | PID 5] Starting execution at tick 21 (Initial: 8, Current: 8)
[Child 2 | PID 5] 50% Workload Done (Initial: 8 -> Current: 8, tick: 31)
[Child 2 | PID 5] Workload Completed! (Initial: 8 -> Final: 8, tick: 40)
>> Completion Order 1: PID 4 (Child 1, Initial Priority 5)
>> Completion Order 2: PID 5 (Child 2, Initial Priority 8)
[Child 3 | PID 6] Starting execution at tick 41 (Initial: 12, Current: 11)
[Child 3 | PID 6] 50% Workload Done (Initial: 12 -> Current: 11, tick: 50)
[Child 3 | PID 6] Workload Completed! (Initial: 12 -> Final: 11, tick: 60)
>> Completion Order 3: PID 6 (Child 3, Initial Priority 12)
[Child 4 | PID 7] Starting execution at tick 60 (Initial: 20, Current: 19)
[Child 4 | PID 7] 50% Workload Done (Initial: 20 -> Current: 19, tick: 70)
[Child 4 | PID 7] Workload Completed! (Initial: 20 -> Final: 19, tick: 79)
>> Completion Order 4: PID 7 (Child 4, Initial Priority 20)

============================================================
Test Summary:
1. Preemption & Priority: Higher priority processes (5, 8) completed first.
2. Anti-Starvation Aging: Lower priority processes (12, 20) had priority boosted
   dynamically as wait_ticks exceeded 30, preventing starvation.
============================================================
