================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 10 - QUESTION 1
Tracking Physical Memory Allocation (memstats)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 10, Question 1 (10a) [Also referenced as Lab 8b]
- Topic          : Physical Memory Management & Lifetime Allocation Statistics
- Test Program   : user/memtest.c
- Shell Command  : $ memtest

Objective:
Track cumulative physical memory allocation and deallocation across the lifetime
of the xv6 kernel:
1. Maintain Allocation Counters in `kernel/kalloc.c`:
   - `kmem.total_allocated`: Cumulative physical pages allocated by `kalloc()`.
   - `kmem.total_freed`: Cumulative physical pages freed by `kfree()`.
   - `kmem.available_pages`: Physical frames currently in the free list.
   - `kmem.initialized`: Flag to distinguish initial bootstrap free-page insertion
     from runtime deallocations.
2. System Call `int memstats(void)`:
   - Prints formatted statistics: Total Allocated Pages, Total Freed Pages,
     Currently Allocated Pages (`total_allocated - total_freed`), and Available Pages.
3. User Verification (`memtest.c`):
   - Measure initial baseline.
   - Allocate 10 pages via `sbrk(10 * 4096)`, verify `total_allocated += 10` and `available -= 10`.
   - Free 10 pages via `sbrk(-10 * 4096)`, verify `total_freed += 10` and `available` restored.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Memory Accounting Invariants:
   At any point in time $t$ after boot:
     $$\text{Currently Allocated Pages} = \text{Total Allocated} - \text{Total Freed}$$
     $$\text{Available Pages} = \text{Total Initial Free Pages} - \text{Currently Allocated}$$
2. The Bootstrap vs. Runtime Challenge:
   - During kernel startup (`kinit()`), xv6 populates its free list by calling
     `kfree()` on every physical frame from `end` to `PHYSTOP`.
   - If `kfree()` blindly incremented `total_freed` during boot, the statistics would
     be skewed. We introduce `kmem.initialized = 0` during boot. Only after `kinit()`
     finishes is `kmem.initialized = 1`, ensuring that `total_freed` strictly tracks
     runtime deallocations.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/kalloc.c       | Modified | Added total_allocated, total_freed, available_pages,
                      |          | and initialized flag to struct kmem; implemented
                      |          | memstats() printer.
kernel/defs.h         | Modified | Declared int memstats(void).
kernel/sysproc.c      | Modified | Implemented sys_memstats(void) syscall handler.
kernel/syscall.h      | Modified | Defined SYS_memstats (23).
kernel/syscall.c      | Modified | Registered sys_memstats in syscall table.
user/user.h           | Modified | Declared int memstats(void) in user API.
user/usys.pl          | Modified | Added entry("memstats") syscall stub.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_memtest.
user/memtest.c        | Created  | Memory allocation test program verifying +10 and
                      |          | -10 page dynamics.


4. DETAILED CODE WALKTHROUGH (kernel/kalloc.c)
--------------------------------------------------------------------------------
```c
struct {
  struct spinlock lock;
  struct run *freelist;
  uint64 total_allocated;  // Total pages allocated since boot
  uint64 total_freed;      // Total pages freed since boot
  uint64 available_pages;  // Current free pages in freelist
  int initialized;         // Set to 1 after boot completes
} kmem;

void *kalloc(void) {
  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r) {
    kmem.freelist = r->next;
    kmem.available_pages--;
    if (kmem.initialized)
      kmem.total_allocated++;
  }
  release(&kmem.lock);
  ...
}

void kfree(void *pa) {
  ...
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.available_pages++;
  if (kmem.initialized)
    kmem.total_freed++;
  release(&kmem.lock);
}
```


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
int memstats(void)
- Syscall Number: SYS_memstats (23)
- Return Value: 0 on success. Prints memory statistics to the console.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/memstats

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

Analysis:
- In Step 1, currently allocated was 201 pages. Available was 32531 pages.
- In Step 2, allocating 10 pages raised total allocated from 241 to 251, currently
  allocated from 201 to 211, and reduced available from 32531 to 32521.
- In Step 3, deallocating 10 pages raised total freed from 40 to 50, currently
  allocated returned to 201, and available restored to exactly 32531.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is the `initialized` flag necessary in `kmem`?
A1: When xv6 boots, `freerange()` calls `kfree()` on tens of thousands of pages to
    build the initial free list. Without the `initialized` flag, all bootstrap free
    calls would be counted as runtime "freed pages", corrupting the lifetime
    allocation statistics.

Q2: Why must `kmem.lock` be held when reading or modifying memory statistics?
A2: Multiple CPU cores can call `kalloc()` and `kfree()` concurrently. Accessing
    or updating the counters without holding `kmem.lock` would result in data races
    and inaccurate memory statistics.

Q3: How is Currently Allocated Pages calculated?
A3: $\text{Currently Allocated} = \text{Total Allocated} - \text{Total Freed}$.
================================================================================
