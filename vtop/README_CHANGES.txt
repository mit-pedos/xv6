================================================================================
  Question 2 — Virtual Memory: Virtual-to-Physical Address Translation (vtop)
================================================================================

1. OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
In modern operating systems, processes execute using virtual memory addresses.
The Memory Management Unit (MMU) translates virtual addresses (VA) to physical
addresses (PA) at runtime using multi-level page tables.

This implementation provides a kernel system call:
    uint64 vtop(uint64 virtual_address);
(also callable as `int vtop(int virtual_address)` or with 64-bit pointers)

The system call:
  - Walks the per-process page table using `walkpgdir()` (wrapping `walk()`).
  - Verifies that the page table entry (PTE) is valid, present (PTE_V), and
    accessible from user mode (PTE_U).
  - Extracts the physical frame base address and combines it with the page
    offset (low 12 bits) to calculate the complete physical address.
  - Returns -1 ((uint64)-1) if the virtual address is unmapped or invalid.


2. ARCHITECTURE & IMPLEMENTATION DETAILS
--------------------------------------------------------------------------------
1. kernel/vm.c:
   - Implemented `walkpgdir(pagetable_t pagetable, uint64 va, int alloc)`:
     Wraps the existing static `walk()` function in vm.c without modifying
     core paging invariant logic.
   - Implemented `vtop(uint64 va)`:
     * Obtains the current process page table (`myproc()->pagetable`).
     * Checks if `va >= MAXVA` and returns -1 if out of valid virtual address bounds.
     * Invokes `walkpgdir(p->pagetable, va, 0)`.
     * Checks if `pte == 0`, or if `(*pte & PTE_V) == 0`, or if `(*pte & PTE_U) == 0`.
       Returns -1 if any check fails.
     * Computes: `pa = PTE2PA(*pte) | (va & 0xFFF)`.
     * Returns `pa`.

2. kernel/sysproc.c:
   - Added `sys_vtop(void)`:
     Retrieves the virtual address argument via `argaddr(0, &va)` and returns
     the result of `vtop(va)`.

3. kernel/defs.h:
   - Declared `pte_t *walkpgdir(pagetable_t, uint64, int);`
   - Declared `uint64 vtop(uint64);`
   - Defined `cprintf` and `printf` aliases to `printk` for portability.

4. Syscall Registration:
   - kernel/syscall.h: Defined `SYS_vtop 23`
   - kernel/syscall.c: Registered `sys_vtop` in table
   - user/user.h: Declared `uint64 vtop(uint64);`
   - user/usys.pl: Added entry for `vtop`

5. User Test Program (user/vttest.c):
   - Tests translation of code segment (`main`).
   - Tests translation of global/data segment (`global_var`).
   - Tests translation of stack segment (`stack_var`).
   - Tests heap allocation via `sbrk(4096)` and verifies translation of base
     and offset addresses (`heap_ptr` and `heap_ptr + 256`).
   - Tests unmapped addresses (`0x7FFFFFF0` and addresses far beyond heap break),
     verifying that `vtop` correctly returns -1.


3. FILES MODIFIED / ADDED
--------------------------------------------------------------------------------
- kernel/vm.c        : Added walkpgdir() and vtop() translation function.
- kernel/sysproc.c   : Added sys_vtop() system call wrapper.
- kernel/defs.h      : Added walkpgdir() and vtop() function prototypes.
- kernel/syscall.h   : Added SYS_vtop syscall number (23).
- kernel/syscall.c   : Registered sys_vtop in syscall dispatch table.
- user/user.h        : Declared vtop() user library wrapper.
- user/usys.pl       : Added syscall entry for vtop.
- Makefile           : Added $U/_vttest to UPROGS and adjusted flags.
- user/vttest.c      : Created user-space test suite.
- README_CHANGES.txt : This documentation.


4. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
To compile and launch xv6 in QEMU:
    $ cd "/home/udit/Desktop/os lab/vtop"
    $ make clean
    $ make qemu

Inside xv6 shell, run:
    $ vttest

To exit QEMU:
    Press Ctrl-A followed by X.


5. VERIFIED EXECUTION OUTPUT
--------------------------------------------------------------------------------
$ vttest
============================================================
    Virtual-to-Physical Address Translation Test (vtop)     
============================================================

--- 1. Testing Code and Global Variables ---
[vtop] main() function code:
       VA: 0x4C  ->  PA: 0x87F2204C
[vtop] global_var (global data):
       VA: 0x2000  ->  PA: 0x87F1E000

--- 2. Testing Stack Variable ---
[vtop] stack_var (local stack variable):
       VA: 0x4FAC  ->  PA: 0x87F1CFAC

--- 3. Testing Heap Allocation via sbrk() ---
[vtop] heap_ptr (allocated heap page base):
       VA: 0x5000  ->  PA: 0x87F26000
[vtop] heap_ptr + 256 (offset within page):
       VA: 0x5100  ->  PA: 0x87F26100

--- 4. Testing Unmapped Virtual Addresses ---
[vtop] Unmapped Address 1 (arbitrary high address):
       VA: 0x7FFFFFF0  ->  PA: -1 (UNMAPPED / INVALID)
[vtop] Unmapped Address 2 (far above heap break):
       VA: 0x4005000  ->  PA: -1 (UNMAPPED / INVALID)

============================================================
vttest: All translation tests completed successfully!
============================================================
================================================================================
