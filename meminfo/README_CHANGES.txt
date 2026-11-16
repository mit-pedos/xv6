================================================================================
Question 2: Physical Memory Statistics (getmeminfo) in xv6
================================================================================

1. OVERVIEW & OBJECTIVES
--------------------------------------------------------------------------------
This implementation introduces physical memory instrumentation and tracking
into the xv6 kernel:
a) Free Physical Page Tracking in `kernel/kalloc.c`:
   - Maintains an exact count of free physical pages in `kmem.free_pages`.
   - Updated atomically in `kfree()` (incremented) and `kalloc()` (decremented)
     under `kmem.lock`.
   - Tracks total available physical pages in `kmem.total_pages`.
b) Page Table Memory Tracking in `kernel/vm.c`:
   - Implemented `count_pagetable_pages()` to traverse the calling process's
     active page tables (root level 2, level 1, and level 0 pages) and count
     all physical frames dedicated to page table structures.
c) System Call `int getmeminfo(struct meminfo *mi)`:
   - Fills user-provided `struct meminfo` with:
     * `total_pages`: Total physical frames in RAM.
     * `free_pages`: Currently available frames on the free list.
     * `user_pages`: Pages in user address space (`p->sz / PGSIZE`).
     * `pagetable_pages`: Physical pages used by the process's page tables.
d) User Verification Program (`memtest.c`):
   - Measures memory statistics across 4 key lifecycle phases:
     1. Baseline state.
     2. Heap growth with `sbrk(4 * 4096)`.
     3. Child process fork and reap, proving zero memory leak.
     4. Heap shrinking with `sbrk(-4 * 4096)`, restoring baseline free count.

2. FILES MODIFIED & ADDED
--------------------------------------------------------------------------------
- kernel/meminfo.h:
  * Defined `struct meminfo` holding total_pages, free_pages, user_pages, and
    pagetable_pages.
- kernel/kalloc.c:
  * Added `free_pages` and `total_pages` to `struct kmem`.
  * Updated count in `kinit()`, `kfree()`, and `kalloc()` under `kmem.lock`.
  * Implemented `get_free_pages()` and `get_total_pages()`.
- kernel/vm.c:
  * Implemented `count_pagetable_pages(pagetable_t)` to count allocated page
    table frames.
- kernel/defs.h:
  * Declared `get_free_pages()`, `get_total_pages()`, `count_pagetable_pages()`.
  * Added `#define cprintf printk` and `#define printf printk`.
- kernel/sysproc.c:
  * Implemented `sys_getmeminfo(void)` syscall handler.
- kernel/syscall.h & kernel/syscall.c:
  * Registered `SYS_getmeminfo` (23).
- user/user.h & user/usys.pl:
  * Declared and exported `int getmeminfo(struct meminfo *);`.
- Makefile:
  * Cleared `DETFLAGS = ` to support paths containing spaces.
  * Added `$U/_memtest\` to `UPROGS`.
- user/memtest.c:
  * Comprehensive memory statistics and leak verification program.

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
         Physical Memory Statistics Test (memtest)          
============================================================

[1. Initial Baseline]
  Total System Pages:      32732 (130928 KB)
  Free Physical Pages:     32531 (130124 KB)
  Process Address Pages:   5 (20 KB, sz/PGSIZE)
  Process Page-Table Pages:5 (20 KB)

--- Step 2: Allocate 4 Pages (16 KB) with sbrk(4 * 4096) ---
[2. After Heap Expansion (sbrk +4 pages)]
  Total System Pages:      32732 (130928 KB)
  Free Physical Pages:     32527 (130108 KB)
  Process Address Pages:   9 (36 KB, sz/PGSIZE)
  Process Page-Table Pages:5 (20 KB)
  -> Free pages change: 32531 -> 32527 (decreased by 4)
  -> Process user pages: 5 -> 9 (increased by 4)

--- Step 3: Forking a Child Process ---
[3a. Inside Child Process]
  Total System Pages:      32732 (130928 KB)
  Free Physical Pages:     32512 (130048 KB)
  Process Address Pages:   9 (36 KB, sz/PGSIZE)
  Process Page-Table Pages:5 (20 KB)
  -> Free pages dropped further due to child address space and page tables.
[3b. Parent After Reaping Child]
  Total System Pages:      32732 (130928 KB)
  Free Physical Pages:     32527 (130108 KB)
  Process Address Pages:   9 (36 KB, sz/PGSIZE)
  Process Page-Table Pages:5 (20 KB)

--- Verification: Memory Leak Check After Child Termination ---
  Free pages before fork: 32527
  Free pages after reap:  32527
  >> SUCCESS: Exact match! All child frames and page tables reaped (NO LEAK).

--- Step 4: Shrink Heap with sbrk(-4 * 4096) ---
[4. After Heap Shrinking]
  Total System Pages:      32732 (130928 KB)
  Free Physical Pages:     32531 (130124 KB)
  Process Address Pages:   5 (20 KB, sz/PGSIZE)
  Process Page-Table Pages:5 (20 KB)
  -> Free pages returned from 32527 to 32531 (restored 4 pages)

============================================================
memtest completed all tasks successfully!
============================================================
