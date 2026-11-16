================================================================================
Question 1: Virtual to Physical Address Translation (va2pa) in xv6
================================================================================

1. OVERVIEW & OBJECTIVES
--------------------------------------------------------------------------------
This implementation introduces a new system call `va2pa` to expose virtual to
physical address translation to user processes:
a) `uint64 va2pa(uint64 va)`:
   - Walks the calling process's page table using `walkpgdir()` (exposing `walk()`
     from `kernel/vm.c`).
   - Verifies that the page is present (P bit / PTE_V set) and user-accessible
     (U bit / PTE_U set).
   - If unmapped, invalid, or supervisor-only, returns -1.
   - Otherwise computes and returns the physical address: (PTE2PA(*pte) | offset).
b) Kernel-side translation breakdown logging:
   - The kernel prints the translation components: Page Directory indices,
     Page Table index, page offset, and permission flag bits (P, W, U) along
     with the resulting physical address.
c) User-level test program (`vatest.c`):
   - Translates global variable, local stack variable, and heap allocated block.
   - Verifies that unmapped address `0x7FFFFFFF` returns -1.
   - Forks a child process, demonstrating that the child and parent share the
     same virtual address for a variable but map to distinct physical pages
     due to page duplication on `fork()`.

2. FILES MODIFIED & ADDED
--------------------------------------------------------------------------------
- kernel/defs.h:
  * Declared `walkpgdir()` and `va2pa()`.
  * Added `#define cprintf printk` and `#define printf printk` for kernel printing.
- kernel/vm.c:
  * Implemented `walkpgdir(pagetable_t, uint64, int)`.
  * Implemented `va2pa(uint64 va)` with page table lookup, validity/user checks,
    diagnostic breakdown logging, and physical address calculation.
- kernel/syscall.h & kernel/syscall.c:
  * Registered system call `SYS_va2pa` (23).
- kernel/sysproc.c:
  * Implemented syscall handler `sys_va2pa(void)`.
- user/user.h & user/usys.pl:
  * Declared and exported `uint64 va2pa(uint64);`.
- Makefile:
  * Cleared `DETFLAGS = ` to support directory paths with spaces.
  * Added `$U/_vatest\` to `UPROGS`.
- user/vatest.c:
  * Comprehensive test suite verifying all tasks.

3. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
From this directory:
    $ make qemu

Inside xv6 shell:
    $ vatest

To exit QEMU:
    Press Ctrl-A then x

4. VERIFIED TEST OUTPUT LOG
--------------------------------------------------------------------------------
$ vatest
============================================================
  Virtual to Physical Address Translation Test (vatest)     
============================================================

--- Step 1: Translate Global, Local (Stack), and Heap Variables ---
[cprintf/kernel] va2pa: VA 0x2000 -> PDE_idx:[0,0], PTE_idx:2, Offset:0x0, Flags:[P=1, W=1, U=1] -> PA:0x87f1e000
Global Variable:  VA = 0x2000  -->  PA = 0x87F1E000
[cprintf/kernel] va2pa: VA 0x4fac -> PDE_idx:[0,0], PTE_idx:4, Offset:0xfac, Flags:[P=1, W=1, U=1] -> PA:0x87f1cfac
Local Variable:   VA = 0x4FAC  -->  PA = 0x87F1CFAC
[cprintf/kernel] va2pa: VA 0x14f80 -> PDE_idx:[0,0], PTE_idx:20, Offset:0xf80, Flags:[P=1, W=1, U=1] -> PA:0x87f30f80
Heap Variable:    VA = 0x14F80  -->  PA = 0x87F30F80

--- Step 2: Unmapped Address Translation (0x7FFFFFFF) ---
Unmapped Address: VA = 0x7FFFFFFF  -->  Result = -1 (Expected: -1)
SUCCESS: va2pa correctly returned -1 for unmapped address.

--- Step 3: Forking Child to Demonstrate Distinct Physical Pages ---
[cprintf/kernel] va2pa: VA 0x2000 -> PDE_idx:[0,0], PTE_idx:2, Offset:0x0, Flags:[P=1, W=1, U=1] -> PA:0x87f45000
[Child PID 4]  VA = 0x2000  -->  PA = 0x87F45000
[cprintf/kernel] va2pa: VA 0x2000 -> PDE_idx:[0,0], PTE_idx:2, Offset:0x0, Flags:[P=1, W=1, U=1] -> PA:0x87f1e000
[Parent PID 3] VA = 0x2000  -->  PA = 0x87F1E000

VERIFICATION:
Same virtual address:   0x2000 (Parent & Child)
Separate physical page: PA copied on fork (Parent PA != Child PA)

============================================================
vatest passed all verification steps successfully!
============================================================
