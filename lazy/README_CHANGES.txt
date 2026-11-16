================================================================================
Question 4: Lazy Memory Allocation (Demand Paging) in xv6
================================================================================

1. OVERVIEW & OBJECTIVES
--------------------------------------------------------------------------------
This implementation replaces eager heap allocation with on-demand lazy memory
allocation (demand paging) in xv6:
a) Lazy `sbrk()` in `kernel/sysproc.c`:
   - When `n > 0`, `sys_sbrk()` immediately increments `myproc()->sz` by `n`
     without calling `growproc()` or mapping physical frames.
   - When `n < 0`, `sys_sbrk()` frees mapped pages in that range using
     `growproc(n)` / `uvmdealloc()`.
b) Page Fault Handler in `kernel/trap.c`:
   - Catches page fault traps (`scause == 13 || scause == 15 || scause == 12`).
   - Retrieves faulting virtual address from `r_stval()` (equivalent to x86 `rcr2`).
   - Validates that the faulting address lies inside the process heap:
     `va < p->sz` and above the stack/guard page (`va >= PGROUNDDOWN(p->trapframe->sp)`).
   - Allocates a zeroed physical frame with `kalloc()`, maps it via `mappages()`
     with permissions `PTE_W | PTE_U | PTE_R | PTE_X`, and resumes execution.
   - If address is invalid or out of memory, prints an error diagnostic and
     terminates the process (`setkilled(p)`).
c) Kernel System Call & Edge Case Handling in `kernel/vm.c`:
   - `walkaddr()`: If kernel system calls (such as `read()` or `write()`) receive
     a user buffer pointing to an untouched lazy page, `walkaddr()` automatically
     triggers demand allocation via `vmfault()`, mapping the page seamlessly.
   - `uvmunmap()`: Skips unallocated leaf entries without panicking when deallocating
     or freeing an address space with untouched lazy pages.
   - `uvmcopy()`: Skips unmapped lazy pages during `fork()`, copying only
     physically present frames.
d) User Verification Program (`lazytest.c`):
   - Verifies 32 MB `sbrk()` succeeds immediately with 0 physical frames consumed.
   - Verifies touching 3 widely spaced pages allocates only those touched pages.
   - Verifies system call (`pipe` / `read`) into an untouched buffer triggers
     demand paging correctly.
   - Verifies `fork()` and `exit()` function cleanly with lazy pages.
   - Verifies invalid accesses beyond `sz` terminate only the offending process
     without crashing the kernel.

2. FILES MODIFIED & ADDED
--------------------------------------------------------------------------------
- kernel/sysproc.c:
  * Modified `sys_sbrk()` to lazily increment `p->sz` for positive allocation requests.
  * Added `sys_getmeminfo()` to monitor physical memory changes.
- kernel/trap.c:
  * Handled page fault traps (`scause == 13/15/12`) with bounds validation and
    demand paging via `vmfault()`.
- kernel/vm.c:
  * Modified `walkaddr()` to demand-page user buffers passed to kernel syscalls.
  * Updated `vmfault()` to enforce stack guard boundary checks.
  * Added `count_pagetable_pages()` to track page table frame allocations.
- kernel/kalloc.c:
  * Maintained `kmem.free_pages` and `kmem.total_pages`.
- kernel/meminfo.h:
  * Defined `struct meminfo` structure.
- kernel/defs.h:
  * Declared memory instrumentation and paging helper functions.
  * Added `#define cprintf printk`, `#define printf printk`, and `#define rcr2() r_stval()`.
- kernel/syscall.h & kernel/syscall.c:
  * Registered `SYS_getmeminfo` (23).
- user/user.h & user/usys.pl:
  * Exported `getmeminfo()`.
- user/ulib.c:
  * Updated `sbrk()` wrapper to invoke lazy allocation by default.
- Makefile:
  * Cleared `DETFLAGS = ` to support paths containing spaces.
  * Added `$U/_lazytest\` to `UPROGS`.
- user/lazytest.c:
  * Comprehensive demand paging and edge-case verification test suite.

3. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
From this directory:
    $ make qemu

Inside xv6 shell:
    $ lazytest

To exit QEMU:
    Press Ctrl-A then x

4. VERIFIED TEST OUTPUT LOG
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
