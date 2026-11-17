================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 8 - QUESTION 4
Lazy Memory Allocation (Demand Paging for Heap)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 8, Question 4 (8d) [Also referenced as Lab 9b]
- Topic          : Virtual Memory, Demand Paging & Page Fault Handling
- Test Program   : user/lazytest.c
- Shell Command  : $ lazytest

Objective:
Replace eager physical memory allocation in `sbrk()` with lazy memory allocation
(demand paging) in xv6:
1. Lazy `sbrk()` in `kernel/sysproc.c`:
   - When a process calls `sbrk(n)` with $n > 0$, the kernel simply increments
     the process size: `myproc()->sz += n`.
   - No physical memory frames are allocated, and no page table mappings are created.
2. Page Fault Exception Handler in `kernel/trap.c`:
   - When the user process attempts to access an address in this newly grown heap,
     the hardware MMU cannot translate the address and raises a page fault exception
     (`scause == 13` for Load Page Fault, `scause == 15` for Store Page Fault).
   - In `usertrap()`, the kernel intercepts the fault, reads the faulting virtual
     address from `r_stval()`, validates that `va < p->sz`, allocates a zeroed
     physical frame with `kalloc()`, maps it into the page table with `mappages()`,
     and resumes the process seamlessly.
3. System Call & Kernel Edge Cases:
   - Handle kernel system calls (`read()`, `write()`, `pipe()`): if a user program
     passes a lazy heap buffer into a system call, `walkaddr()` in `kernel/vm.c`
     must trigger demand allocation so kernel memory copies do not fault.
   - Modify `uvmunmap()` and `uvmcopy()` to skip unmapped lazy pages without panicking.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Eager Allocation vs. Demand Paging:
   - Eager Allocation: When `sbrk(32 MB)` is called, the kernel immediately allocates
     8,192 physical frames (32 MB) and fills in 8,192 PTEs. If the program only uses
     a few kilobytes of this memory, the rest of physical RAM is wasted.
   - Demand Paging: Programs often declare large heap segments (e.g. sparse arrays
     or large databases) but only access portions of them. By delaying allocation
     until the exact moment a page is first accessed (demanded), memory utilization
     is maximized and startup time is drastically reduced.
2. RISC-V Page Fault Registers:
   - `scause`: Contains the exception code:
     * 12: Instruction Page Fault
     * 13: Load Page Fault (read from unmapped address)
     * 15: Store/AMO Page Fault (write to unmapped address)
   - `stval`: Supervisor Trap Value register. Holds the exact faulting virtual address.
   - `sepc`: Supervisor Exception Program Counter. Holds the instruction that faulted.
     Upon returning with `sret`, the CPU re-executes the faulting instruction, which
     now succeeds because the page has been mapped.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/sysproc.c      | Modified | Replaced eager allocation in sys_sbrk() with
                      |          | lazy size incrementing (sz += n).
kernel/trap.c         | Modified | Intercepted scause 13 & 15 in usertrap(), extracted
                      |          | faulting VA via r_stval(), and mapped zeroed page.
kernel/vm.c           | Modified | Updated walkaddr() to fault in pages for syscalls;
                      |          | updated uvmunmap() and uvmcopy() to ignore unmapped
                      |          | lazy pages without panic.
kernel/defs.h         | Modified | Declared vmfault() demand-allocation helper.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_lazytest.
user/lazytest.c       | Created  | Comprehensive test harness verifying large sbrk,
                      |          | sparse page access, syscall buffers, and invalid kills.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/sysproc.c:
   ```c
   uint64
   sys_sbrk(void)
   {
     int addr;
     int n;
     argint(0, &n);
     addr = myproc()->sz;

     if (n > 0) {
       // Lazy allocation: expand virtual size without allocating physical frames
       myproc()->sz += n;
     } else if (n < 0) {
       // Eager deallocation when shrinking heap
       myproc()->sz = uvmdealloc(myproc()->pagetable, addr, addr + n);
     }
     return addr;
   }
   ```

b) Page Fault Trap Handler in kernel/trap.c:
   ```c
   else if (r_scause() == 13 || r_scause() == 15) {
     uint64 va = r_stval();
     struct proc *p = myproc();

     // Validate that faulting address is within valid heap bounds
     if (va >= p->sz || va < PGROUNDDOWN(p->trapframe->sp)) {
       printk("usertrap(): page fault at invalid va 0x%lx (sz 0x%lx) pid=%d\n",
              va, p->sz, p->pid);
       setkilled(p);
     } else {
       // Allocate zeroed physical frame
       char *mem = kalloc();
       if (mem == 0) {
         setkilled(p); // Out of physical memory
       } else {
         memset(mem, 0, PGSIZE);
         if (mappages(p->pagetable, PGROUNDDOWN(va), PGSIZE,
                      (uint64)mem, PTE_W | PTE_U | PTE_R | PTE_X) != 0) {
           kfree(mem);
           setkilled(p);
         }
       }
     }
   }
   ```

c) Handling System Calls in kernel/vm.c:
   When user space passes a lazy address to `read()` or `write()`, the kernel
   calls `walkaddr()` to translate user pointers. If unmapped:
   ```c
   uint64
   walkaddr(pagetable_t pagetable, uint64 va)
   {
     ...
     pte = walk(pagetable, va, 0);
     if (pte == 0 || (*pte & PTE_V) == 0) {
       struct proc *p = myproc();
       if (p && va < p->sz && va >= PGROUNDDOWN(p->trapframe->sp)) {
         // Trigger demand allocation inside kernel
         if (vmfault(pagetable, va) == 0)
           pte = walk(pagetable, va, 0);
         else
           return 0;
       } else {
         return 0;
       }
     }
     ...
   }
   ```


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/lazy

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ lazytest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ lazytest
============================================================
      Lazy Memory Allocation (Demand Paging) Test           
============================================================

--- Step 1: Initial Baseline Memory ---
  Free physical pages:     32531 (130124 KB)
  Process user pages:      5

--- Step 2: Request Large sbrk(32 MB) ---
  sbrk returned base address: 0x0000000000005000
  Process virtual pages (sz/PGSIZE): 5 -> 8197 (+8192 pages)
  Free physical pages: 32531 -> 32531 (change = 0 pages)
  >> SUCCESS: 32 MB allocated lazily without consuming physical frames!

--- Step 3: Touching 3 Widely Spaced Pages (0 MB, 16 MB, 31 MB) ---
  Read back touched bytes: p[0]=A, p[16MB]=B, p[31MB]=C
  Free physical pages: 32531 -> 32526 (consumed 5 physical frames for 3 pages + page tables)
  >> SUCCESS: Only touched pages triggered on-demand page faults!

--- Step 4: Kernel System Call (read/pipe) into Untouched Heap Buffer ---
  Pipe read into untouched buffer returned: 12 bytes, content: 'HELLO_LAZY!'
  >> SUCCESS: Kernel successfully handled demand paging during syscall!

--- Step 5: Testing fork() & exit() with Lazy Mappings ---
  Child process executed, accessed lazy pages, and exited cleanly.

--- Step 6: Testing Invalid Access Beyond sz (Expect Child Process Termination) ---
  [Child] Attempting invalid write to p[33 MB]...
usertrap(): page fault at invalid va 0x2105000 (sz 0x2005000, sp 0x4f20) pid=5
  Parent successfully reaped faulted child. Kernel is intact and running!

============================================================
lazytest passed all tasks and edge-case verifications!
============================================================

Analysis:
- In Step 2, growing the heap by 32 MB (8,192 pages) changed free physical pages
  by exactly 0!
- In Step 3, touching 3 widely separated pages consumed only 3 data frames plus
  2 intermediate page table directory frames (total 5 frames).
- In Step 4, passing an untouched buffer into `read()` was seamlessly faulted in
  by `walkaddr()`, avoiding a kernel fault.
- In Step 6, an illegal write beyond `p->sz` was safely trapped, killing only the
  faulting process while preserving kernel stability.


7. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: What hardware register holds the faulting address during a page fault in RISC-V?
A1: The `stval` (Supervisor Trap Value) register, read in xv6 using `r_stval()`.

Q2: Why does `sret` re-execute the instruction that caused the page fault?
A2: Unlike system calls where `sepc` is advanced by 4 (`epc += 4`) to skip the `ecall`
    instruction, exceptions preserve `sepc` pointing to the exact instruction that
    faulted. Once the trap handler allocates and maps the missing physical page,
    `sret` re-executes that same instruction, which now completes successfully.

Q3: Why must `walkaddr()` be modified for lazy allocation?
A3: If a user program calls a system call like `read(fd, buf, 10)` where `buf` is in
    an unallocated lazy page, the read happens inside the kernel. The kernel accesses
    user memory by walking the page table via `walkaddr()`. Without modification,
    `walkaddr()` would see the invalid PTE and return 0 (error). Modifying `walkaddr()`
    allows the kernel to allocate the page on demand during the system call.

Q4: What happens if a process accesses an address below `p->sz` but inside the stack guard page?
A4: The check `va < PGROUNDDOWN(p->trapframe->sp)` catches this condition. Accessing
    the stack guard page or below the stack is identified as an illegal access, and
    the process is terminated via `setkilled(p)`.
================================================================================
