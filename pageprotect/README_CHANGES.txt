================================================================================
  Question 3 — Paging: Page Table Management and Page Protection (pageprotect)
================================================================================

1. OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
Virtual memory paging enables modern operating systems to enforce fine-grained
memory access permissions on individual pages. In RISC-V Sv39 paging architecture,
Page Table Entries (PTEs) contain control and protection bits including:
  - PTE_V: Valid / Present bit (bit 0)
  - PTE_R: Read permission bit (bit 1)
  - PTE_W: Write permission bit (bit 2)
  - PTE_X: Execute permission bit (bit 3)
  - PTE_U: User accessible bit (bit 4)

This implementation provides two new system calls:
1. int pageinfo(uint64 virtual_address):
   Inspects the page containing the given virtual address. If mapped, extracts
   and displays the Physical Frame Number (PFN), Physical Address (PA), and
   status bits: Present (P), Writable (W), and User/Supervisor (U).
   Returns 0 on success, or -1 if unmapped / invalid.

2. int setreadonly(uint64 virtual_address):
   Clears the write permission bit (PTE_W) in the corresponding PTE and executes
   `sfence_vma()` to flush the Translation Lookaside Buffer (TLB), immediately
   enforcing read-only memory protection.
   Returns 0 on success, or -1 if unmapped / invalid.


2. ARCHITECTURE & IMPLEMENTATION DETAILS
--------------------------------------------------------------------------------
1. kernel/vm.c:
   - Added `pageinfo(uint64 va)`:
     * Obtains calling process page table (`myproc()->pagetable`).
     * Validates `va < MAXVA`.
     * Walks the page table hierarchy using `walk()`.
     * If PTE exists and `PTE_V` is set:
       Computes PFN = `PTE2PA(*pte) / PGSIZE`.
       Extracts Present bit `(*pte & PTE_V)`, Writable bit `(*pte & PTE_W)`,
       and User bit `(*pte & PTE_U)`.
       Outputs metrics via `printk()`.
     * Returns 0 if mapped, -1 if unmapped.

   - Added `setreadonly(uint64 va)`:
     * Validates `va < MAXVA`.
     * Walks page table using `walk()`.
     * Clears writable flag: `*pte &= ~PTE_W`.
     * Invokes `sfence_vma()` to invalidate cached TLB address translations
       across all harts/CPUs.
     * Returns 0 on success, -1 on failure.

2. kernel/sysproc.c:
   - Implemented `sys_pageinfo()` and `sys_setreadonly()` system call wrappers
     using `argaddr(0, &va)`.

3. kernel/defs.h:
   - Added function declarations for `pageinfo()` and `setreadonly()`.
   - Defined `cprintf` and `printf` portability macros mapping to `printk`.

4. Syscall Registration:
   - kernel/syscall.h: Defined `SYS_pageinfo 23` and `SYS_setreadonly 24`.
   - kernel/syscall.c: Added handler entries to `syscalls[]` array.
   - user/user.h: Declared library prototypes `pageinfo(uint64)` and `setreadonly(uint64)`.
   - user/usys.pl: Added entry points for `pageinfo` and `setreadonly`.

5. User Test Suite (user/pagetest.c):
   - Allocates a 4 KB page using `sbrk(4096)`.
   - Writes test data to the page ("HI").
   - Calls `pageinfo()` before `setreadonly()`: verifies `W=1`.
   - Calls `setreadonly()`.
   - Calls `pageinfo()` after `setreadonly()`: verifies `W=0`.
   - Reads from the read-only page to confirm read access remains functional.
   - Forks a child process that attempts a write to the protected page.
   - The hardware MMU triggers a store page fault (`scause 0xf / 15`).
   - The kernel kills the child process, and parent reaps it via `wait()`.
   - Tests unmapped virtual addresses, verifying proper `-1` error returns.


3. FILES MODIFIED / ADDED
--------------------------------------------------------------------------------
- kernel/vm.c        : Implemented pageinfo() and setreadonly() with TLB flush.
- kernel/sysproc.c   : Implemented sys_pageinfo() and sys_setreadonly().
- kernel/defs.h      : Added prototypes and printk defines.
- kernel/syscall.h   : Added SYS_pageinfo (23) and SYS_setreadonly (24).
- kernel/syscall.c   : Added syscall table entries.
- user/user.h        : Declared user-space syscall wrappers.
- user/usys.pl       : Added syscall stub entries.
- Makefile           : Added $U/_pagetest to UPROGS and adjusted build flags.
- user/pagetest.c    : Created test program verifying page protection.
- README_CHANGES.txt : This documentation.


4. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
To compile and launch xv6 in QEMU:
    $ cd "/home/udit/Desktop/os lab/pageprotect"
    $ make clean
    $ make qemu

Inside xv6 shell, run:
    $ pagetest

To exit QEMU:
    Press Ctrl-A followed by X.


5. VERIFIED EXECUTION OUTPUT
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
================================================================================
