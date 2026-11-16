================================================================================
Operating Systems Lab Assignment
Question 1: Implementing User-Level / Kernel-Supported Threads (clone & join)
================================================================================

1. OVERVIEW & DESIGN
--------------------
This question implements kernel-supported threads sharing the same user address space
(page table mappings) while retaining independent execution contexts (registers,
kernel stack, user stack).

Key System Calls Implemented:
1. int clone(void(*fcn)(void*), void *arg, void *stack):
   - Allocates a new struct proc using allocproc().
   - Shares the parent's user address space:
     Uses uvmshare() to map all physical pages of the parent process into the thread's
     page table with identical permissions (PTE_R | PTE_W | PTE_U | PTE_X).
     This guarantees all threads read and write the exact same physical memory.
   - Preserves independent trapframe mappings: each thread retains its private trapframe
     mapped at TRAPFRAME (0x3FFFFFFE000) in its page table, preventing multi-core trapframe
     corruptions.
   - Sets user instruction pointer np->trapframe->epc = (uint64)fcn.
   - Sets thread argument np->trapframe->a0 = (uint64)arg.
   - Sets user stack pointer np->trapframe->sp = stack + PGSIZE (16-byte aligned).
   - Marks np->is_thread = 1 and sets np->parent = p.
   - Returns the thread's PID to the caller.

2. int join(void):
   - Scans the process table looking for exited ZOMBIE threads created by the calling process
     (pp->parent == p && pp->is_thread).
   - Once a thread exits, reaps it, cleans up its resources via freeproc(), and returns its PID.
   - If threads are still running, sleeps on sleep_prepare(p) until woken up by thread exit.

3. Process & Memory Management Modifications:
   - In freeproc(): If p->is_thread == 1, unmaps user virtual pages without freeing the
     physical memory (uvmunmap(..., do_free=0)), unmaps trampoline & trapframe, and frees
     only the thread's page directory nodes (uvmfree(p->pagetable, 0)). The physical pages
     remain intact until the parent process exits.
   - In kwait(): Checks pp->parent == p && !pp->is_thread, ensuring wait() only reaps
     normal child processes and leaves threads to join().

4. User Test Program (user/threadtest.c):
   - Clones 3 threads, all incrementing a shared global counter 100,000 times concurrently.
   - Waits for all threads to start using a shared barrier flag (start_flag = 1).
   - Reaps all 3 threads via join().
   - Demonstrates that all threads access the exact same shared memory.
   - Demonstrates the expected race condition: non-atomic increments lead to lost updates
     (counter < 300,000), motivating Question 2's synchronization using semaphores.

2. FILES MODIFIED / ADDED
-------------------------
- kernel/proc.h:
  * Added int is_thread field to struct proc.
- kernel/defs.h:
  * Added declarations for kclone() and kjoin().
- kernel/proc.c:
  * Modified allocproc() to initialize is_thread = 0.
  * Modified freeproc() to avoid freeing shared physical memory for threads.
  * Modified kwait() to exclude thread processes.
  * Implemented uvmshare() to map shared physical pages into the thread's page table.
  * Implemented kclone() and kjoin().
- kernel/sysproc.c:
  * Added sys_clone() and sys_join() wrappers.
- kernel/syscall.h & kernel/syscall.c:
  * Registered SYS_clone (23) and SYS_join (24).
- user/user.h & user/usys.pl:
  * Exported clone() and join() prototypes and stubs to user space.
- user/threadtest.c:
  * User-level test harness demonstrating clone(), join(), shared address space,
    and concurrent race condition.
- Makefile:
  * Added $U/_threadtest to UPROGS.
  * Cleared DETFLAGS to prevent compiler errors on paths containing spaces.

3. HOW TO BUILD AND RUN
-----------------------
Inside this directory:
    $ make clean
    $ make qemu

Once xv6 boots into the shell, run:
    $ threadtest

To exit QEMU:
    Press Ctrl+A, then X.

4. SAMPLE OUTPUT VERIFICATION
-----------------------------
$ threadtest
============================================================
   Kernel-Supported Threads Demonstration with clone()      
   Threads: 3, Increments per thread: 100000                   
   Expected counter without race condition: 300000              
============================================================
Main (pid 3): Cloned Thread 1 with pid 4
Main (pid 3): Cloned Thread 2 with pid 5
Main (pid 3): Cloned Thread 3 with pid 6
Main: Releasing all 3 threads concurrently...
Thread 1 (pid 4) finished 100000 increments.
Thread 2 (pid 5) finished 100000 increments.
Thread 3 (pid 6) finished 100000 increments.
Main: Reaped thread with pid 4 via join()
Main: Reaped thread with pid 5 via join()
Main: Reaped thread with pid 6 via join()
============================================================
Simulation Finished Summary:
All 3 threads executed in the SAME shared address space.
Expected Counter : 300000
Actual Counter   : 100000
Race condition confirmed: Lost updates = 200000 (66% loss).
Explanation: Unsynchronized concurrent execution caused
interleaved read-modify-write cycles. (Fixed in Question 2 using Semaphores).
============================================================
