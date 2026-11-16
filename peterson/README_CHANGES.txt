================================================================================
OPERATING SYSTEM LAB ASSIGNMENT 5 - QUESTION 1
Mutual Exclusion using Peterson's Algorithm
================================================================================

1. OVERVIEW
--------------------------------------------------------------------------------
This implementation achieves mutual exclusion between two processes (parent and
child) sharing a critical section purely through Peterson's Algorithm. No OS-level 
locks or semaphores are used for mutual exclusion. The coordination is enforced 
strictly using two shared variables:
  - int flag[2] : intent of each process to enter the critical section
  - int turn    : whose turn it is when both processes compete to enter


2. SHARED MEMORY MECHANISM
--------------------------------------------------------------------------------
Since xv6 processes have isolated address spaces by default, a shared memory
system call `void *shm_get(void)` was implemented:
- A dedicated virtual address `SHM_VA` is defined at `(TRAPFRAME - PGSIZE)`.
- When `shm_get()` is called, the kernel allocates a physical page (via kalloc())
  and maps it into the process's page table with user read/write permissions.
- In `kfork()` (kernel/proc.c), when the parent forks a child, the kernel checks 
  if `SHM_VA` is mapped. If so, it maps the exact same physical frame into the 
  child's page table.
- Both parent and child now read and modify identical physical memory variables.
- In `proc_freepagetable()`, `uvmunmap(pagetable, SHM_VA, 1, 0)` safely unmaps 
  the shared page without freeing the underlying physical frame.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
- kernel/memlayout.h  : Defined SHM_VA (TRAPFRAME - PGSIZE)
- kernel/defs.h       : Declared shminit(), shm_get(), and walk()
- kernel/main.c       : Called shminit() during system boot
- kernel/proc.c       : Inherited SHM_VA mapping in kfork() and unmapped in proc_freepagetable()
- kernel/sysproc.c    : Implemented shminit(), shm_get(), and sys_shm_get()
- kernel/syscall.h    : Defined SYS_shm_get (23)
- kernel/syscall.c    : Registered sys_shm_get in syscalls array
- user/user.h         : Declared void *shm_get(void)
- user/usys.pl        : Generated syscall stub for shm_get
- Makefile            : Added $U/_peterson to UPROGS, set CPUS := 1, cleared DETFLAGS
- user/peterson.c     : [NEW] Peterson's mutual exclusion implementation and verification


4. HOW PETERSON'S ALGORITHM WORKS IN USER SPACE (user/peterson.c)
--------------------------------------------------------------------------------
1. Struct peterson_data:
   Contains volatile int flag[2], volatile int turn, and volatile int shared_counter.
2. Entry Section:
   flag[id] = 1;
   turn = other;
   __sync_synchronize(); // RISC-V memory barrier preventing instruction reordering
   while (flag[other] && turn == other) { /* busy-wait */ }
3. Critical Section:
   shared_counter++;
   printf("Process %d in CS, counter = %d\n", id, shared_counter);
4. Exit Section:
   __sync_synchronize();
   flag[id] = 0;
5. Remainder Section:
   Performs a delay loop so both processes compete fairly.


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd "/home/udit/Desktop/os lab/peterson"

Step 2: Compile and launch xv6 inside QEMU:
        make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ peterson

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. EXPECTED OUTCOME
--------------------------------------------------------------------------------
- Both processes take turns entering the critical section.
- "in CS" print statements never interleave.
- Final counter value is exactly 20 (10 iterations x 2 processes), verifying zero lost updates.
================================================================================
