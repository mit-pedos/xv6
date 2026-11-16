================================================================================
Question 1: Memory Management: Tracking Physical Memory Allocation in xv6
================================================================================

1. OVERVIEW & OBJECTIVES
--------------------------------------------------------------------------------
This implementation tracks physical memory allocation and deallocation across
the lifetime of the xv6 kernel:
a) Maintained Allocation Counters in `kernel/kalloc.c`:
   - `kmem.total_allocated`: Lifetime count of physical pages allocated by `kalloc()`.
   - `kmem.total_freed`: Lifetime count of physical pages freed by `kfree()`.
   - `kmem.available_pages`: Number of physical frames currently in the free list.
   - Initialized `kmem.initialized` flag to distinguish initial bootstrap page
     registration from runtime dynamic allocations.
b) System Call `int memstats(void)`:
   - Traverses `kmem` counters under `kmem.lock` and outputs formatted statistics:
     * Total Allocated Pages
     * Total Freed Pages
     * Currently Allocated Pages (`total_allocated - total_freed`)
     * Available Physical Memory Pages
c) User Test Program (`memtest.c`):
   - Measures initial baseline memory statistics.
   - Allocates 10 pages (40 KB) using `sbrk()`, touches the pages, and verifies:
     * `total_allocated` increases by 10.
     * `currently_allocated` increases by 10.
     * `available_pages` decreases by 10.
   - Deallocates the 10 pages using `sbrk(-40960)` and verifies:
     * `total_freed` increases by 10.
     * `currently_allocated` returns to baseline.
     * `available_pages` is fully restored.

2. FILES MODIFIED & ADDED
--------------------------------------------------------------------------------
- kernel/kalloc.c:
  * Added `total_allocated`, `total_freed`, `available_pages`, and `initialized`
    to `struct kmem`.
  * Updated counters in `kinit()`, `kfree()`, and `kalloc()` under `kmem.lock`.
  * Implemented `memstats()` printing formatted allocation metrics.
- kernel/defs.h:
  * Declared `int memstats(void);`.
  * Added `#define cprintf printk` and `#define printf printk`.
- kernel/sysproc.c:
  * Added system call handler `sys_memstats(void)`.
- kernel/syscall.h & kernel/syscall.c:
  * Registered system call `SYS_memstats` (23).
- user/user.h & user/usys.pl:
  * Declared and exported `int memstats(void);`.
- Makefile:
  * Cleared `DETFLAGS = ` to support paths containing spaces.
  * Added `$U/_memtest\` to `UPROGS`.
- user/memtest.c:
  * Verification program testing allocation, deallocation, and counter dynamics.

3. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
From this directory:
    $ make qemu

Inside xv6 shell:
    $ memtest

To exit QEMU:
    Press Ctrl-A then x

4. VERIFIED TEST OUTPUT LOG
--------------------------------------------------------------------------------
$ memtest
============================================================
  Physical Memory Allocation Tracking Test (memtest)        
============================================================

--- Step 1: Baseline Memory Statistics ---

--- Physical Memory Allocation Statistics ---
  Total Allocated Pages:     241 (964 KB)
  Total Freed Pages:         40 (160 KB)
  Currently Allocated Pages: 201 (804 KB)
  Available Physical Pages:  32531 (130124 KB)
---------------------------------------------

--- Step 2: Allocating 10 Pages (40 KB) using sbrk() ---
Memory allocated at virtual address: 0x0000000000005000
Updated statistics after allocation:

--- Physical Memory Allocation Statistics ---
  Total Allocated Pages:     251 (1004 KB)
  Total Freed Pages:         40 (160 KB)
  Currently Allocated Pages: 211 (844 KB)
  Available Physical Pages:  32521 (130084 KB)
---------------------------------------------

--- Step 3: Deallocating 10 Pages using sbrk(-40960) ---
Updated statistics after deallocation:

--- Physical Memory Allocation Statistics ---
  Total Allocated Pages:     251 (1004 KB)
  Total Freed Pages:         50 (200 KB)
  Currently Allocated Pages: 201 (804 KB)
  Available Physical Pages:  32531 (130124 KB)
---------------------------------------------

============================================================
memtest completed successfully!
Observations:
  1. Allocating 10 pages increased total & currently allocated counters
     and decreased available physical memory.
  2. Releasing the memory increased total freed pages and restored
     the available physical pages.
============================================================
