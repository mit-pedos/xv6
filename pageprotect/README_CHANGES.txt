================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 10 - QUESTION 3
Page Table Management and Page Protection (pageprotect)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 10, Question 3 (10c)
- Topic          : Virtual Memory, Page Protection & TLB Management
- Test Program   : user/pagetest.c
- Shell Command  : $ pagetest

Objective:
Implement fine-grained page protection system calls in xv6:
1. `int pageinfo(uint64 virtual_address)`:
   - Walks the process page table, extracts Physical Frame Number (PFN),
     Physical Address (PA), and permission flags: Present (`PTE_V`), Writable (`PTE_W`),
     and User (`PTE_U`).
2. `int setreadonly(uint64 virtual_address)`:
   - Clears the write permission bit (`PTE_W`) in the corresponding leaf Page Table
     Entry (`*pte &= ~PTE_W`).
   - Flushes the Translation Lookaside Buffer (TLB) using `sfence_vma()` so the CPU
     hardware immediately enforces read-only access.
3. User Verification (`pagetest.c`):
   - Allocate a heap page via `sbrk(4096)` and write "HI".
   - Check `pageinfo()`: verify `W=1`.
   - Call `setreadonly()`: verify `W=0`.
   - Verify that reading the page still succeeds.
   - Fork a child process that attempts a write to the read-only page.
   - Verify that the hardware MMU traps with a Store Page Fault (`scause 0xf = 15`),
     killing the child while leaving the parent unaffected.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Memory Protection at the Hardware Level:
   - Modern CPUs enforce security and stability using permission bits stored in
     the page table.
   - If an instruction attempts an operation contrary to the PTE flags (e.g. store
     to a page with `PTE_W == 0`, or user-mode execution of a page with `PTE_U == 0`),
     the MMU does not complete the memory cycle. Instead, it raises a synchronous
     hardware exception to the kernel.
2. The Translation Lookaside Buffer (TLB) and Invalidation:
   - The TLB is a high-speed on-chip cache holding recent virtual-to-physical translations.
   - If the OS modifies a PTE in memory (such as clearing `PTE_W`), the CPU may
     continue using the stale entry cached in the TLB!
   - Therefore, the OS MUST issue an `sfence_vma()` instruction (TLB shootdown/flush)
     immediately after altering any PTE.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/vm.c           | Modified | Implemented pageinfo() and setreadonly() with
                      |          | sfence_vma() TLB invalidation.
kernel/sysproc.c      | Modified | Implemented sys_pageinfo() and sys_setreadonly().
kernel/defs.h         | Modified | Declared pageinfo and setreadonly prototypes.
kernel/syscall.h      | Modified | Defined SYS_pageinfo (23) and SYS_setreadonly (24).
kernel/syscall.c      | Modified | Registered syscall handlers in dispatch table.
user/user.h           | Modified | Declared user-space library wrappers.
user/usys.pl          | Modified | Added syscall stubs.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_pagetest.
user/pagetest.c       | Created  | Test program verifying permission check, read success,
                      |          | child write fault termination, and unmapped errors.


4. DETAILED CODE WALKTHROUGH (kernel/vm.c)
--------------------------------------------------------------------------------
a) `pageinfo(uint64 va)`:
   ```c
   int pageinfo(uint64 va) {
     struct proc *p = myproc();
     if (va >= MAXVA) return -1;

     pte_t *pte = walk(p->pagetable, va, 0);
     if (!pte || !(*pte & PTE_V)) {
       printk("[pageinfo] VA: 0x%lx is NOT MAPPED\n", va);
       return -1;
     }

     uint64 pa = PTE2PA(*pte);
     uint64 pfn = pa / PGSIZE;
     int p_bit = (*pte & PTE_V) ? 1 : 0;
     int w_bit = (*pte & PTE_W) ? 1 : 0;
     int u_bit = (*pte & PTE_U) ? 1 : 0;

     printk("[pageinfo] VA: 0x%lx -> PFN: 0x%lx (PA: 0x%lx) | Present: %d, Writable: %d, User: %d\n",
            va, pfn, pa, p_bit, w_bit, u_bit);
     return 0;
   }
   ```

b) `setreadonly(uint64 va)`:
   ```c
   int setreadonly(uint64 va) {
     struct proc *p = myproc();
     if (va >= MAXVA) return -1;

     pte_t *pte = walk(p->pagetable, va, 0);
     if (!pte || !(*pte & PTE_V))
       return -1;

     *pte &= ~PTE_W;     // Clear write permission
     sfence_vma();       // Flush Translation Lookaside Buffer (TLB)
     return 0;
   }
   ```


5. SYSTEM CALL SPECIFICATIONS
--------------------------------------------------------------------------------
1. int pageinfo(uint64 virtual_address)
   - Syscall Number: SYS_pageinfo (23)
   - Return Value: 0 on success, -1 if unmapped. Prints PFN and P/W/U bits.
2. int setreadonly(uint64 virtual_address)
   - Syscall Number: SYS_setreadonly (24)
   - Return Value: 0 on success, -1 if unmapped.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/pageprotect

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ pagetest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ pagetest
============================================================
  Page Table Management & Page Protection Test (pagetest)   
============================================================

--- 1. Allocating a physical page using sbrk(4096) ---
Allocated page at Virtual Address: 0x5000

--- 2. Writing initial data to the allocated page ---
Successfully wrote "HI" to page at 0x5000

--- 3. Checking pageinfo BEFORE setreadonly ---
[pageinfo] VA: 0x5000 -> PFN: 0x87f26 (PA: 0x87f26000) | Present: 1, Writable: 1, User: 1
Expected: Present=1, Writable=1, User=1

--- 4. Invoking setreadonly() to mark page READ-ONLY ---
setreadonly() returned success.

--- 5. Checking pageinfo AFTER setreadonly ---
[pageinfo] VA: 0x5000 -> PFN: 0x87f26 (PA: 0x87f26000) | Present: 1, Writable: 0, User: 1
Expected: Present=1, Writable=0, User=1 (Write permission removed!)

--- 6. Verifying READ operations still succeed ---
Read value from read-only page: 'HI'
Read verification passed!

--- 7. Attempting WRITE to read-only page in a child process ---
[Child pid 4] Attempting to write 'X' to read-only address 0x5000...
usertrap(): unexpected scause 0xf pid=4
            sepc=0x146 stval=0x5000
[Parent] Child process terminated with status -1.
[Parent] SUCCESS: Child process was killed by kernel due to page fault exception!

--- 8. Testing unmapped address error handling ---
Calling pageinfo on unmapped address 0x7FFFFFF0:
[pageinfo] VA: 0x7ffffff0 is NOT MAPPED
pageinfo returned: -1 (expected -1)
Calling setreadonly on unmapped address 0x7FFFFFF0:
setreadonly returned: -1 (expected -1)

============================================================
pagetest: All page protection tests completed successfully!
============================================================

Analysis:
- The allocated heap page at `0x5000` started with `Writable: 1`.
- After calling `setreadonly()`, `Writable` changed to `0`.
- Reading the page succeeded cleanly.
- When child process attempted to write to `0x5000`, the MMU trapped with
  `scause 0xf` (15 = Store/AMO Page Fault). The kernel killed only the offending child.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is `sfence_vma()` necessary after clearing `PTE_W`?
A1: The CPU caches address translations and permissions in the TLB. If `PTE_W` is
    cleared in memory without invalidating the TLB, the CPU might continue using
    its cached translation that still has write permission enabled, allowing writes
    to proceed until an unrelated context switch happens. `sfence_vma()` immediately
    invalidates the TLB entry.

Q2: What is the exception code `scause == 0xf`?
A2: In the RISC-V Privileged Architecture, exception code 15 (`0xf`) is a Store/AMO
    Page Fault, triggered when an instruction attempts to store data to an address
    whose PTE has `PTE_W == 0` or `PTE_V == 0`.

Q3: How is page protection used to implement Copy-on-Write (COW) fork?
A3: When `fork()` is called, instead of duplicating physical pages, the kernel maps
    the parent's physical pages into the child's page table and marks both parent
    and child PTEs as read-only (`PTE_W = 0`). When either process attempts to write,
    a page fault occurs; the kernel allocates a new physical frame, copies the page,
    makes it writable, and resumes execution.
================================================================================
