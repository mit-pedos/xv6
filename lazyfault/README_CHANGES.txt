================================================================================
  Question 2 — Handling Page Faults (Lazy Allocation / lazyfault)
================================================================================

1. OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
By default in traditional xv6, calling `sbrk()` immediately allocates physical
memory frames (`uvmalloc`) and maps them into the process's page table. If any
unmapped page is accessed, the process is abruptly terminated by the kernel.

In this implementation, memory allocation is made lazy (demand paging):
  - `sbrk()` only increases `p->sz` without allocating physical memory frames.
  - When the process accesses an unmapped address in its heap (`va < p->sz`),
    the hardware MMU generates a page fault exception.
  - The kernel trap handler intercepts the page fault, dynamically allocates
    a zeroed physical page with `kalloc()`, maps it into the page table,
    increments a per-process page fault counter (`p->page_faults`), and resumes
    user execution seamlessly.
  - If a process accesses an illegal address (`va >= p->sz`), the kernel prints
    the offending PID and address and terminates that process.
  - Implements a user test program `faulttest.c` and a new system call
    `int getfaults(void)` to verify that touching 5 distinct pages causes
    exactly 5 page faults.


2. ARCHITECTURE & IMPLEMENTATION DETAILS
--------------------------------------------------------------------------------
1. kernel/sysproc.c:
   - Modified `sys_sbrk()`:
     * For `n > 0`, lazily increases `myproc()->sz += n` without allocating
       physical frames or page tables. Returns the old `sz`.
     * For `n < 0`, deallocates mapped memory via `growproc(n)`.
   - Added `sys_getfaults(void)`:
     * Returns `myproc()->page_faults`.

2. kernel/proc.h & kernel/proc.c:
   - Added `uint64 page_faults;` field to `struct proc`.
   - Initialized `p->page_faults = 0;` in `allocproc()`.

3. kernel/trap.c:
   - Intercepts page fault exceptions (`scause == 15` (store) or `13` (load)).
   - Invokes `vmfault(p->pagetable, p->sz, r_stval(), ...)`.
   - On successful lazy allocation, increments `p->page_faults++`.
   - If the faulting address `stval >= p->sz`, prints:
     "Page fault: pid=%d address=0x%lx exceeds size=0x%lx (killed)"
     and invokes `setkilled(p)`.

4. kernel/vm.c:
   - `uvmunmap()` and `uvmcopy()` safely check `pte != 0 && (*pte & PTE_V)` and
     skip unallocated/unmapped lazy pages without causing panics.

5. Syscall Wiring:
   - kernel/syscall.h: Added `SYS_getfaults 23`.
   - kernel/syscall.c: Registered `sys_getfaults`.
   - user/user.h: Declared `int getfaults(void);`.
   - user/usys.pl: Added `entry("getfaults");`.

6. User Test Program (user/faulttest.c):
   - Requests 32 pages via `sbrk(32 * 4096)`. Verifies 0 new page faults immediately after.
   - Touches exactly 5 distinct pages (indices 0, 4, 11, 19, 31).
   - Verifies with `getfaults()` that exactly 5 new page faults occurred.
   - Forks a child process that accesses memory above `p->sz`. Verifies that only
     the child is killed by the kernel while the parent survives.


3. FILES MODIFIED / ADDED
--------------------------------------------------------------------------------
- kernel/proc.h      : Added page_faults counter to struct proc.
- kernel/proc.c      : Initialized page_faults in allocproc().
- kernel/sysproc.c   : Modified sys_sbrk() for lazy growth and added sys_getfaults().
- kernel/trap.c      : Updated page fault handler to count faults and handle out-of-bounds.
- kernel/defs.h      : Added printk portability macros.
- kernel/syscall.h   : Added SYS_getfaults (23).
- kernel/syscall.c   : Registered sys_getfaults.
- user/user.h        : Declared getfaults() user library wrapper.
- user/usys.pl       : Added syscall stub entry.
- Makefile           : Added $U/_faulttest to UPROGS and adjusted build flags.
- user/faulttest.c   : Test suite validating lazy allocation and getfaults().
- README_CHANGES.txt : This documentation.


4. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
To compile and launch xv6 in QEMU:
    $ cd "/home/udit/Desktop/os lab/lazyfault"
    $ make clean
    $ make qemu

Inside xv6 shell, run:
    $ faulttest

To exit QEMU:
    Press Ctrl-A followed by X.


5. VERIFIED EXECUTION OUTPUT
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
================================================================================
