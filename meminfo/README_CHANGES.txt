================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 8 - QUESTION 2
Physical Memory Statistics & Free Page Tracking (getmeminfo)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 8, Question 2 (8b)
- Topic          : Physical Memory Management & Instrumentation
- Test Program   : user/memtest.c
- Shell Command  : $ memtest

Objective:
Implement physical memory instrumentation and reporting in the xv6 kernel:
1. Free Physical Page Tracking in `kernel/kalloc.c`:
   Maintain exact counters of total and free physical pages (`kmem.free_pages` and
   `kmem.total_pages`) under `kmem.lock`.
2. Page Table Overhead Tracking in `kernel/vm.c`:
   Implement `count_pagetable_pages()` to traverse the calling process's 3-level
   page table tree and count all physical frames dedicated to page directory nodes.
3. System Call `int getmeminfo(struct meminfo *mi)`:
   Populates a user-provided `struct meminfo` with:
   - `total_pages`: Total physical frames managed by the operating system.
   - `free_pages`: Currently available physical frames on the kernel free list.
   - `user_pages`: Memory pages in the process address space (`p->sz / PGSIZE`).
   - `pagetable_pages`: Physical pages consumed by process page table structures.
4. User Verification:
   Test heap expansion (`sbrk(+4)`), heap shrinking (`sbrk(-4)`), and child process
   fork/reap lifecycle to verify zero memory leaks.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Physical Memory Architecture in xv6:
   - Physical memory begins at `KERNBASE` (`0x80000000`) and ends at `PHYSTOP` (`0x88000000`),
     providing 128 MB of physical RAM.
   - Page Granularity: Pages are 4096 bytes ($2^{12}$). Total pages = 32,768 (128 MB / 4 KB).
   - Free Frame Management: Handled via a singly linked list (`struct run *freelist`)
     protected by `kmem.lock`.
2. Atomic Page Accounting:
   - When a page is freed via `kfree()`, it is prepended to `freelist` and `free_pages++`.
   - When a page is allocated via `kalloc()`, it is popped from `freelist` and `free_pages--`.
   - Performing this inside `acquire(&kmem.lock)` guarantees thread-safe, consistent
     accounting across all CPU cores.
3. Page Table Space Overhead:
   - For a 3-level page table, mapping even a single page requires at least:
     * 1 Level-2 root table page
     * 1 Level-1 intermediate table page
     * 1 Level-0 leaf table page
     Total = 3 page table pages (12 KB overhead) to map 4 KB of data!
   - `count_pagetable_pages()` quantifies this structural overhead.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/meminfo.h      | Created  | Defined struct meminfo.
kernel/kalloc.c       | Modified | Added free_pages & total_pages to struct kmem;
                      |          | updated counts in kinit, kfree, kalloc;
                      |          | implemented get_free_pages() and get_total_pages().
kernel/vm.c           | Modified | Implemented count_pagetable_pages(pagetable_t).
kernel/defs.h         | Modified | Declared memory tracking functions.
kernel/syscall.h      | Modified | Defined SYS_getmeminfo (23).
kernel/syscall.c      | Modified | Registered sys_getmeminfo in syscall table.
kernel/sysproc.c      | Modified | Implemented sys_getmeminfo() handler using copyout().
user/user.h           | Modified | Declared struct meminfo and getmeminfo().
user/usys.pl          | Modified | Added entry("getmeminfo") stub.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_memtest.
user/memtest.c        | Created  | Memory lifecycle test suite verifying allocation,
                      |          | deallocation, and leak-free child termination.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) kernel/meminfo.h:
   ```c
   struct meminfo {
     uint64 total_pages;      // Total system RAM pages
     uint64 free_pages;       // Free pages currently available
     uint64 user_pages;       // Pages allocated to calling process
     uint64 pagetable_pages;  // Pages consumed by page table hierarchy
   };
   ```

b) Page Counting in kernel/kalloc.c:
   ```c
   void kfree(void *pa) {
     ...
     acquire(&kmem.lock);
     r->next = kmem.freelist;
     kmem.freelist = r;
     kmem.free_pages++;
     release(&kmem.lock);
   }

   void *kalloc(void) {
     acquire(&kmem.lock);
     r = kmem.freelist;
     if (r) {
       kmem.freelist = r->next;
       kmem.free_pages--;
     }
     release(&kmem.lock);
     ...
   }
   ```

c) Page Table Tree Walker (kernel/vm.c):
   Recursively counts valid non-leaf and leaf page table directory pages.


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
int getmeminfo(struct meminfo *mi)
- Syscall Number: SYS_getmeminfo (23)
- Parameters: `mi` (pointer to user-space `struct meminfo`).
- Return Value: 0 on success, -1 on invalid user pointer (copyout failure).


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/meminfo

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ memtest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
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

Analysis:
- `sbrk(+4 * 4096)` caused free pages to drop by exactly 4 (32531 $\to$ 32527) and
  user pages to increase by 4 (5 $\to$ 9).
- When child was forked, child duplicated all 9 user pages and created 5 page-table
  pages, consuming 15 frames (32527 $\to$ 32512).
- When parent reaped the child via `wait()`, free pages restored to exactly 32527,
  proving zero memory leakage.
- Shrinking heap via `sbrk(-4 * 4096)` returned free pages back to baseline (32531).


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: How does xv6 organize free physical memory frames?
A1: xv6 maintains a linked list of free pages (`struct run *freelist`). Each free
    page stores the pointer to the next free page at the very beginning of its own
    unallocated memory (`r->next`), requiring zero separate memory allocation for
    the free list metadata.

Q2: Why must `copyout()` be used to pass `struct meminfo` to user space?
A2: User programs run in user virtual memory space. The kernel cannot write directly
    to a user pointer because user and kernel page tables are separate, and user
    addresses must be validated to prevent kernel memory corruption. `copyout()`
    safely checks permissions and writes to the user's page table.

Q3: Why did `Process Page-Table Pages` remain 5 even after allocating 4 heap pages?
A3: Because the newly allocated 4 pages resided within the same 2 MB virtual memory
    range covered by the existing Level 0 page table. A new page table page is only
    allocated when a process crosses a 2 MB or 1 GB boundary.

Q4: What causes a memory leak in an operating system kernel?
A4: Failing to call `kfree()` when memory is no longer needed (e.g. failing to free
    page table nodes when a process exits, or losing track of physical frames),
    permanently reducing the number of available pages on the free list.
================================================================================
