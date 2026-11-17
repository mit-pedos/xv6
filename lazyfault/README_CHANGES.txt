================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 9 - QUESTION 2
Handling Page Faults & Page Fault Counting (lazyfault)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 9, Question 2 (9b) [Also referenced as Lab 8d]
- Topic          : Virtual Memory, Page Fault Tracking & Demand Paging
- Test Program   : user/faulttest.c
- Shell Command  : $ faulttest

Objective:
Implement demand paging with explicit per-process page fault counting in xv6:
1. Lazy Heap Growth:
   - Modify `sbrk()` so that positive requests only increment `p->sz` without
     allocating physical memory frames.
2. Page Fault Trapping:
   - Catch RISC-V page fault traps (`scause == 13 || scause == 15`) in `usertrap()`.
   - Validate that the faulting virtual address `va = r_stval()` lies within the
     allocated heap (`va < p->sz` and above stack pointer `sp`).
   - Allocate a zeroed physical frame with `kalloc()`, map it into the page table,
     and resume execution.
3. Page Fault Counter:
   - Add `uint64 page_faults;` to `struct proc`.
   - Increment `p->page_faults++` on every successfully handled page fault.
4. System Call:
   - Implement `int getfaults(void)` to return the calling process's page fault count.
5. User Verification:
   - Test program `faulttest.c` allocates 32 pages via `sbrk(32 * 4096)`, touches
     exactly 5 distinct pages, and asserts that `getfaults()` increased by exactly 5.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Demand Paging Life Cycle:
   1. User program executes an instruction referencing an unmapped virtual address:
      e.g. `sb a0, 0(a1)` at `va = 0x5000`.
   2. Hardware MMU checks Level 0 PTE for `0x5000`, finds `PTE_V == 0`.
   3. Hardware raises a Page Fault trap: sets `scause = 15` (Store Page Fault),
      saves `0x5000` into `stval`, saves PC into `sepc`, switches to supervisor mode,
      and jumps to `trampoline.S` $\to$ `usertrap()`.
   4. Operating system verifies that `va < p->sz` (valid logical address).
   5. OS allocates a free physical frame $PA$, clears it, maps $VA \to PA$ with
      `PTE_V | PTE_R | PTE_W | PTE_U`, and increments `p->page_faults++`.
   6. OS executes `sret`, restoring user mode and re-executing `sb a0, 0(a1)`.
   7. Hardware MMU now finds `PTE_V == 1`, succeeds, and user execution continues.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Added uint64 page_faults field to struct proc.
kernel/proc.c         | Modified | Initialized page_faults = 0 in allocproc().
kernel/sysproc.c      | Modified | Modified sys_sbrk() for lazy allocation;
                      |          | implemented sys_getfaults(void).
kernel/trap.c         | Modified | Intercepted page faults (scause 13, 15), mapped
                      |          | physical frames, and incremented page_faults.
kernel/vm.c           | Modified | Updated uvmunmap() to ignore unmapped lazy pages.
kernel/defs.h         | Modified | Added getfaults declarations.
kernel/syscall.h      | Modified | Defined SYS_getfaults (23).
kernel/syscall.c      | Modified | Registered sys_getfaults in syscall dispatch table.
user/user.h           | Modified | Declared int getfaults(void) in user API.
user/usys.pl          | Modified | Added entry("getfaults") syscall stub.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_faulttest.
user/faulttest.c      | Created  | User test suite verifying lazy sbrk, touching 5
                      |          | pages, and checking exact getfaults() counter delta.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/proc.h:
   ```c
   struct proc {
     ...
     uint64 page_faults;   // Cumulative count of handled page faults
     ...
   };
   ```

b) Page Fault Handler in kernel/trap.c:
   ```c
   else if (r_scause() == 13 || r_scause() == 15) {
     uint64 va = r_stval();
     struct proc *p = myproc();

     if (va >= p->sz || va < PGROUNDDOWN(p->trapframe->sp)) {
       printk("Page fault: pid=%d address=0x%lx exceeds size=0x%lx (killed)\n",
              p->pid, va, p->sz);
       setkilled(p);
     } else {
       char *mem = kalloc();
       if (mem == 0) {
         setkilled(p);
       } else {
         memset(mem, 0, PGSIZE);
         if (mappages(p->pagetable, PGROUNDDOWN(va), PGSIZE,
                      (uint64)mem, PTE_W | PTE_U | PTE_R) != 0) {
           kfree(mem);
           setkilled(p);
         } else {
           p->page_faults++; // Increment successfully handled page fault counter
         }
       }
     }
   }
   ```

c) kernel/sysproc.c:
   ```c
   uint64
   sys_getfaults(void)
   {
     return myproc()->page_faults;
   }
   ```


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
int getfaults(void)
- Syscall Number: SYS_getfaults (23)
- Return Value: Number of page faults handled for the calling process.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/lazyfault

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ faulttest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ faulttest
============================================================
        Lazy Memory Allocation Test (faulttest)             
============================================================

--- 1. Baseline Page Fault Count ---
Initial handled page faults: 2

--- 2. Requesting 32 pages via sbrk(32 * 4096 = 131072 bytes) ---
sbrk returned base address: 0x5000
Handled page faults immediately after sbrk: 2
PASSED: No physical pages allocated yet (lazy allocation active!)

--- 3. Writing to exactly 5 distinct pages ---
  Touched page index 0 (VA 0x5000) with 'A'
  Touched page index 4 (VA 0x9000) with 'B'
  Touched page index 11 (VA 0x10000) with 'C'
  Touched page index 19 (VA 0x18000) with 'D'
  Touched page index 31 (VA 0x24000) with 'E'

--- 4. Checking Page Fault Count via getfaults() ---
Total handled page faults: 7 (New faults: 5)
SUCCESS: Exactly 5 page faults were handled by the kernel!

--- 5. Testing Access Above Allocated Memory Size ---
[Child pid 4] Attempting to access address above p->sz (VA 0x28000)...
Page fault: pid=4 address=0x28000 exceeds size=0x25000 (killed)
[Parent] Child process terminated with status -1.
[Parent] SUCCESS: Only the offending child was killed by the kernel!

============================================================
faulttest: All lazy allocation tests completed successfully!
============================================================

Analysis:
- Initial startup faults were 2 (due to initial stack setup / text load).
- After requesting 32 pages, `getfaults()` remained at 2 (0 pages eagerly allocated).
- Touching exactly 5 distinct pages (indices 0, 4, 11, 19, 31) caused exactly 5
  page faults (2 + 5 = 7).
- An out-of-bounds access beyond `p->sz` was caught and only the offending child
  process was terminated.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: What is the purpose of the `getfaults()` system call?
A1: It exposes the internal kernel counter `p->page_faults` to user applications,
    enabling profiling, performance benchmarking, and verifying that demand paging
    behaves lazily.

Q2: Why did touching 5 pages cause exactly 5 page faults?
A2: Because each of the 5 pages touched had a unique virtual page number (indices
    0, 4, 11, 19, 31). The first access to each unmapped page triggered an MMU
    exception. Subsequent accesses to those same 5 pages would not cause any further
    faults because the PTEs are now valid (`PTE_V == 1`).

Q3: Why must the allocated physical frame be zeroed using `memset(mem, 0, PGSIZE)`?
A3: Security and predictability: `kalloc()` returns previously used physical memory
    that may contain remnants of passwords, keys, or data from other terminated
    processes. Zeroing prevents information leakage and ensures BSS/heap memory
    starts initialized to 0.

Q4: What causes `scause == 13` vs `scause == 15` in RISC-V?
A4: `scause == 13` is a Load Page Fault, generated when an instruction attempts to
    read from an unmapped or non-readable page. `scause == 15` is a Store/AMO Page
    Fault, generated when an instruction attempts to write to an unmapped or
    non-writable page.
================================================================================
