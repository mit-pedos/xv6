================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 7 - QUESTION 1
Kernel-Supported Lightweight Threads (clone & join)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 7, Question 1 (7a)
- Topic          : Kernel-Supported Threads & Lightweight Processes
- Test Program   : user/threadtest.c
- Shell Command  : $ threadtest

Objective:
Implement lightweight kernel-supported threads in xv6:
1. Thread Model:
   Lightweight execution entities that share the parent process's virtual address
   space (code, data, heap) while maintaining independent execution contexts
   (private CPU registers, private kernel stack, private user stack, and private trapframe).
2. System Calls:
   - `int clone(void(*fcn)(void*), void *arg, void *stack)`: Spawns a new thread
     starting at function pointer `fcn`, passing argument `arg`, and using user stack `stack`.
   - `int join(void)`: Reaps an exited child thread, frees its private resources,
     and returns its PID.
3. Memory Management:
   - Implement `uvmshare()`: Maps all physical memory pages of the parent process into
     the child thread's page table with identical permissions.
   - Modify `freeproc()`: When a thread exits, unmap its virtual pages without freeing
     the shared physical memory frames.
   - Modify `kwait()`: Prevent `wait()` from reaping thread children, leaving thread
     reaping exclusively to `join()`.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Process vs. Thread:
   - Process (Heavyweight): Has its own independent address space, page table, and
     isolated memory. Communication requires IPC (pipes, shared memory). High creation
     and context-switch cost.
   - Thread (Lightweight Process): Shares virtual address space, open file table, and
     process metadata with peer threads, but retains:
     * Private Program Counter / Instruction Pointer (`trapframe->epc`)
     * Private CPU General-Purpose Registers
     * Private Kernel Stack (`p->kstack`)
     * Private User Stack (`trapframe->sp`)
     * Private Trapframe (`p->trapframe` mapped at `TRAPFRAME = 0x3FFFFFFE000`)

2. RISC-V Architecture & Calling Convention:
   - The user stack pointer `sp` MUST be 16-byte aligned in the RISC-V calling convention.
   - When launching a thread function `void (*fcn)(void *)`:
     * Register `a0` holds the first function parameter (`arg`).
     * `epc` (Exception Program Counter) is set to `(uint64)fcn`.
     * `sp` points to the top of the allocated page: `(uint64)stack + PGSIZE`.
   - When `sret` returns to user space, the CPU loads `pc = epc` and starts executing `fcn(arg)`.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Added int is_thread field to struct proc.
kernel/defs.h         | Modified | Declared kclone() and kjoin().
kernel/proc.c         | Modified | Modified allocproc() to initialize is_thread=0;
                      |          | implemented uvmshare(), kclone(), and kjoin();
                      |          | updated freeproc() and kwait() for thread handling.
kernel/sysproc.c      | Modified | Added sys_clone() and sys_join() system call handlers.
kernel/syscall.h      | Modified | Registered SYS_clone (23) and SYS_join (24).
kernel/syscall.c      | Modified | Wired sys_clone and sys_join into syscall table.
user/user.h           | Modified | Declared clone() and join() in user API.
user/usys.pl          | Modified | Added syscall stubs.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_threadtest.
user/threadtest.c     | Created  | Test program cloning 3 threads incrementing a shared
                      |          | counter concurrently and verifying race conditions.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/proc.c:
   1. `uvmshare(pagetable_t old, pagetable_t new, uint64 sz)`:
      Iterates over the parent's user address space page by page:
      ```c
      for (uint64 va = 0; va < sz; va += PGSIZE) {
        pte_t *pte = walk(old, va, 0);
        uint64 pa = PTE2PA(*pte);
        uint flags = PTE_FLAGS(*pte);
        // Map exact same physical frame into new page table
        mappages(new, va, PGSIZE, pa, flags);
      }
      ```
   2. `kclone(void (*fcn)(void*), void *arg, void *stack)`:
      - Allocates process using `allocproc()`.
      - Calls `uvmshare()` to share user memory.
      - Sets up execution context:
        ```c
        np->trapframe->epc = (uint64)fcn;           // Start at thread function
        np->trapframe->a0 = (uint64)arg;            // Pass argument
        np->trapframe->sp = (uint64)stack + PGSIZE; // Top of user stack (16-byte aligned)
        np->is_thread = 1;
        np->parent = myproc();
        ```
      - Changes state to `RUNNABLE` and returns `np->pid`.

   3. `kjoin(void)`:
      - Scans `proc[]` looking for a zombie thread belonging to the caller:
        `if (pp->parent == p && pp->is_thread)`
      - Reaps the thread: cleans up via `freeproc()`, returns thread's PID.
      - If threads are still running, sleeps on `p` until woken up.

   4. `freeproc()`:
      - If `p->is_thread == 1`:
        Calls `uvmunmap(p->pagetable, 0, PGROUNDUP(p->sz)/PGSIZE, 0)` with `do_free = 0`.
        This unmaps the page table entries without freeing the physical pages.
        Only the thread's private trapframe, trampoline, and page directory nodes are freed.

   5. `kwait()`:
      - Ignores threads: `if (pp->parent == p && !pp->is_thread)`.


5. SYSTEM CALL SPECIFICATIONS
--------------------------------------------------------------------------------
1. int clone(void(*fcn)(void*), void *arg, void *stack)
   - Syscall Number: SYS_clone (23)
   - Parameters:
     * `fcn`: Function pointer for thread entry.
     * `arg`: Void pointer argument passed to `fcn`.
     * `stack`: Pointer to allocated page for user stack.
   - Return Value: PID of cloned thread on success, -1 on failure.

2. int join(void)
   - Syscall Number: SYS_join (24)
   - Return Value: PID of reaped thread on success, -1 if no threads exist.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/threads

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ threadtest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
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

Analysis:
- All 3 threads were successfully spawned and shared the exact same address space.
- Because `counter++` is not atomic, concurrent unsynchronized increments caused
  lost updates (final count = 100000 vs expected 300000), proving thread sharing
  and illustrating the classic race condition solved by semaphores in Question 2.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why do threads share page tables, but require separate user stacks and trapframes?
A1: Sharing the page table allows threads to read and modify shared code, globals, and
    heap. However, each thread executes different functions or function calls with
    its own call chain, local variables, and return addresses, requiring a private
    user stack. Similarly, interrupts and system calls save CPU registers into the
    trapframe; sharing a trapframe would cause threads to overwrite each other's
    registers during context switches.

Q2: Why does `freeproc()` call `uvmunmap()` with `do_free = 0` when `p->is_thread == 1`?
A2: Because the physical memory frames are shared with the parent and peer threads.
    If the exiting thread called `kfree()` on those physical frames, the surviving
    threads would suffer page faults or memory corruption. The physical pages must
    only be freed when the main parent process exits.

Q3: Why must the user stack pointer be 16-byte aligned in RISC-V?
A3: The RISC-V calling convention mandates that the stack pointer `sp` be a multiple
    of 16 bytes at function entry. This ensures proper alignment for 128-bit types,
    floating point registers, and SIMD vectors.

Q4: How does `join()` differentiate between normal child processes and thread children?
A4: `struct proc` contains the flag `is_thread`. In `kjoin()`, the search checks
    `pp->parent == p && pp->is_thread`. In `kwait()`, the search checks
    `pp->parent == p && !pp->is_thread`.
================================================================================
