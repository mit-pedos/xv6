================================================================================
  Question 1 — Printing the Page Table of a Process (vmprint / ptprint)
================================================================================

1. OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
This implementation introduces a new system call `int vmprint(void)` that walks
and displays the page table structure of the calling process:
  - Iterates through the process's page directory and page tables.
  - For every valid user-accessible page, prints its virtual address (VA),
    physical frame address (PA), and permission flags:
      * P: Present / Valid (PTE_V)
      * W: Writable (PTE_W)
      * U: User-accessible (PTE_U)
  - At the end, outputs the total number of valid user pages and the total number
    of page-table pages allocated for the process.
  - Implements user test program `pttest.c` demonstrating that exactly 4 new
    pages appear in the page table dump after invoking `sbrk(4 * 4096)`.


2. IMPLEMENTATION DETAILS
--------------------------------------------------------------------------------
1. kernel/vm.c:
   - Added `int vmprint(void)`:
     * Obtains `p = myproc()` and `pagetable = p->pagetable`.
     * Recursively / hierarchically walks the Sv39 3-level page table structure
       (root Level-2, Level-1, and leaf Level-0).
     * Tracks `pt_pages`: counts 1 for root table plus each allocated Level-1
       and Level-0 page table directory.
     * For each leaf entry with `(pte & PTE_V) && (pte & PTE_U)`:
       Reconstructs the virtual address from table indices:
           va = (i << 30) | (j << 21) | (k << 12);
       Extracts physical frame base `pa = PTE2PA(pte)`.
       Reads flags P, W, U.
       Prints the formatted row and increments `valid_pages`.
     * Prints final summary: total valid user pages and total page-table pages.
     * Returns total valid user pages.

2. kernel/sysproc.c:
   - Added `sys_vmprint(void)` invoking `vmprint()`.

3. kernel/defs.h:
   - Added `int vmprint(void);` prototype.
   - Added `#define cprintf printk` and `#define printf printk`.

4. Syscall Wiring:
   - kernel/syscall.h: Added `SYS_vmprint 23`
   - kernel/syscall.c: Added `sys_vmprint` to syscall table.
   - user/user.h: Declared `int vmprint(void);`
   - user/usys.pl: Added syscall entry `entry("vmprint");`

5. User Test Suite (user/pttest.c):
   - Calls `vmprint()` at startup, recording initial page count.
   - Calls `sbrk(4 * 4096)` to request 16 KB (4 pages).
   - Writes to each allocated page.
   - Calls `vmprint()` again and asserts `after_pages - before_pages == 4`.


3. FILES MODIFIED / ADDED
--------------------------------------------------------------------------------
- kernel/vm.c        : Implemented vmprint() walking page table.
- kernel/sysproc.c   : Implemented sys_vmprint() syscall handler.
- kernel/defs.h      : Added vmprint prototype.
- kernel/syscall.h   : Added SYS_vmprint (23).
- kernel/syscall.c   : Registered sys_vmprint in syscall dispatch table.
- user/user.h        : Declared vmprint() user library wrapper.
- user/usys.pl       : Added syscall stub entry.
- Makefile           : Added $U/_pttest to UPROGS and adjusted build flags.
- user/pttest.c      : User program verifying 4 new pages post-sbrk.
- README_CHANGES.txt : This documentation.


4. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
To compile and launch xv6 in QEMU:
    $ cd "/home/udit/Desktop/os lab/ptprint"
    $ make clean
    $ make qemu

Inside xv6 shell, run:
    $ pttest

To exit QEMU:
    Press Ctrl-A followed by X.


5. VERIFIED EXECUTION OUTPUT
--------------------------------------------------------------------------------
$ pttest
============================================================
         Page Table Printing Test Program (pttest)          
============================================================

--- 1. Calling vmprint() at initial start-up ---
=== Page Table Dump for PID 3 (pttest) ===
  Virtual Addr       Physical Addr      Flags [P, W, U]
  -----------------------------------------------------
  0x0  ->  0x87f22000    [P: 1, W: 0, U: 1]
  0x1000  ->  0x87f1f000    [P: 1, W: 0, U: 1]
  0x2000  ->  0x87f1e000    [P: 1, W: 1, U: 1]
  0x4000  ->  0x87f1c000    [P: 1, W: 1, U: 1]
  -----------------------------------------------------
  Total valid user pages : 4
  Total page-table pages : 5
=====================================================
Initial valid user pages reported by vmprint: 4

--- 2. Allocating 4 pages via sbrk(4 * 4096 = 16384 bytes) ---
sbrk allocated memory starting at: 0x5000
Touched all 4 newly allocated pages.

--- 3. Calling vmprint() after sbrk(4 * 4096) ---
=== Page Table Dump for PID 3 (pttest) ===
  Virtual Addr       Physical Addr      Flags [P, W, U]
  -----------------------------------------------------
  0x0  ->  0x87f22000    [P: 1, W: 0, U: 1]
  0x1000  ->  0x87f1f000    [P: 1, W: 0, U: 1]
  0x2000  ->  0x87f1e000    [P: 1, W: 1, U: 1]
  0x4000  ->  0x87f1c000    [P: 1, W: 1, U: 1]
  0x5000  ->  0x87f26000    [P: 1, W: 1, U: 1]
  0x6000  ->  0x87f54000    [P: 1, W: 1, U: 1]
  0x7000  ->  0x87f53000    [P: 1, W: 1, U: 1]
  0x8000  ->  0x87f49000    [P: 1, W: 1, U: 1]
  -----------------------------------------------------
  Total valid user pages : 8
  Total page-table pages : 5
=====================================================
New valid user pages reported by vmprint: 8

--- 4. Verification Analysis ---
Initial user pages : 4
After sbrk(+4)     : 8
Difference         : 4 pages
SUCCESS: Exactly four new pages appeared in the page table!
============================================================
================================================================================
