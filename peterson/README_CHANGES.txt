================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 5 - QUESTION 1
Mutual Exclusion using Peterson's Algorithm
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 5, Question 1 (5a)
- Topic          : Process Concurrency & Software Synchronization
- Test Program   : user/peterson.c
- Shell Command  : $ peterson

Objective:
Demonstrate mutual exclusion between two processes (Process 0 and Process 1)
sharing a critical section purely through software using Peterson's Algorithm,
without using any kernel-level lock or semaphore primitives.
1. Implement a shared memory allocation system call `void *shm_get(void)` so
   two distinct processes can access the same physical page in user space.
2. Coordinate entry into the critical section using two shared variables:
   - `volatile int flag[2]`: Process intent to enter the critical section.
   - `volatile int turn`: The tie-breaking turn variable.
3. Verify that both processes execute their critical sections in mutual exclusion
   and increment a shared counter to its exact expected value without lost updates.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Peterson's Algorithm Requirements:
   A valid solution to the critical-section problem must satisfy three properties:
   - Mutual Exclusion: If Process $i$ is executing in its critical section, then
     no other process can be executing in its critical section.
   - Progress: If no process is executing in its critical section and some processes
     wish to enter, only those processes that are not executing in their remainder
     sections can participate in deciding who enters next.
   - Bounded Waiting: There exists a bound on the number of times that other processes
     are allowed to enter their critical sections after a process has made a request
     to enter.

2. Peterson's Two-Process Protocol:
   For Process $i$ (where $j = 1 - i$):
   ```c
   // Entry Section
   flag[i] = 1;         // Declare intent
   turn = j;            // Be polite: give turn to other process
   __sync_synchronize();// Memory barrier
   while (flag[j] && turn == j) {
     // Busy wait
   }

   // Critical Section
   shared_counter++;

   // Exit Section
   flag[i] = 0;         // Relinquish intent
   __sync_synchronize();// Memory barrier

   // Remainder Section
   remainder_delay();
   ```

3. Memory Consistency & Barriers:
   On modern out-of-order execution processors and optimizing compilers, memory
   writes can be reordered. The `__sync_synchronize()` hardware memory fence is
   crucial to guarantee that `flag[i] = 1` commits to memory before `turn = j` is
   evaluated, preserving Peterson's correctness.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/memlayout.h    | Modified | Defined SHM_VA = (TRAPFRAME - PGSIZE) = 0x3FFFFFFE000.
kernel/defs.h         | Modified | Declared shminit(), shm_get(), and walk().
kernel/proc.c         | Modified | Propagated shared memory mapping to child in fork().
kernel/main.c         | Modified | Initialized shared memory subsystem via shminit().
kernel/sysproc.c      | Modified | Implemented shminit(), shm_get(), and sys_shm_get().
kernel/syscall.h      | Modified | Defined SYS_shm_get (23).
kernel/syscall.c      | Modified | Registered sys_shm_get in syscalls table.
user/user.h           | Modified | Declared void *shm_get(void) in user API.
user/usys.pl          | Modified | Added entry("shm_get") stub generator.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_peterson.
user/peterson.c       | Created  | User test program testing 2-process synchronization.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) Shared Memory Support (kernel/sysproc.c & kernel/proc.c):
   - `shminit()`: Allocates a zeroed physical page via `kalloc()` and initializes
     a spinlock `shmlock`.
   - `shm_get()`: Maps this shared physical page into the calling process's page table
     at virtual address `SHM_VA` (`0x3FFFFFFE000`) with permissions `PTE_R | PTE_W | PTE_U`.
   - In `fork()`: Checks if the parent has mapped `SHM_VA`. If present, maps the
     exact same physical frame into the child's page table.

b) Peterson Protocol Data Structure (user/peterson.c):
   ```c
   struct peterson_data {
     volatile int flag[2];       // Intent flags
     volatile int turn;          // Turn variable
     volatile int shared_counter;// Shared protected data
   };
   ```

c) Critical Section Execution (user/peterson.c):
   - Parent acts as Process 0; Child acts as Process 1.
   - Each process runs 10 iterations (`ITERATIONS = 10`).
   - Each enters through Peterson's entry protocol, increments `shared_counter`,
     logs its entry, exits through exit section, and executes a remainder delay.


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/peterson

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ peterson

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ peterson
--- Peterson's Algorithm Mutual Exclusion Test ---
Process 0 in CS, counter = 1
Process 0 in CS, counter = 2
Process 0 in CS, counter = 3
Process 1 in CS, counter = 4
Process 1 in CS, counter = 5
Process 1 in CS, counter = 6
Process 1 in CS, counter = 7
Process 1 in CS, counter = 8
Process 1 in CS, counter = 9
Process 1 in CS, counter = 10
Process 1 in CS, counter = 11
Process 1 in CS, counter = 12
Process 1 in CS, counter = 13
Process 0 in CS, counter = 14
Process 0 in CS, counter = 15
Process 0 in CS, counter = 16
Process 0 in CS, counter = 17
Process 0 in CS, counter = 18
Process 0 in CS, counter = 19
Process 0 in CS, counter = 20

Expected final counter = 20
Actual final counter   = 20
--- Peterson's Algorithm Test Passed: Mutual exclusion preserved, zero lost updates! ---

Analysis:
- Processes interleaved execution across the 20 iterations.
- At no point did both processes enter the critical section simultaneously.
- Every increment was strictly atomic; expected count (20) equals actual count (20).


7. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is Peterson's algorithm considered a software-only solution?
A1: Because it does not rely on hardware test-and-set instructions, atomic exchange,
    or operating system kernel lock/semaphore primitives. It achieves mutual exclusion
    purely using standard load/store instructions and shared memory variables.

Q2: What is the role of the `turn` variable?
A2: `turn` is the tie-breaker. When both processes set `flag[0] = 1` and `flag[1] = 1`
    simultaneously, the process that set `turn` second gives precedence to the other
    process, allowing the other process to proceed while it waits.

Q3: Why are the variables declared `volatile`?
A3: The `volatile` keyword instructs the C compiler not to optimize away or cache
    the variable in a CPU register. Every read or write must directly reference
    the physical RAM location, which is required when another process can modify it.

Q4: Why is `__sync_synchronize()` needed in RISC-V?
A4: Modern RISC-V processors employ weak memory consistency models that permit memory
    reordering. `__sync_synchronize()` issues a hardware memory barrier (`fence`),
    ensuring that writes to `flag` become visible before `turn` is written.

Q5: Can Peterson's algorithm be scaled to $N > 2$ processes?
A5: Yes, using the Filter algorithm (generalized Peterson's algorithm), which uses
    $N-1$ waiting stages, but its complexity and busy-waiting overhead make kernel
    semaphores or hardware spinlocks preferable for $N$ processes.
================================================================================
