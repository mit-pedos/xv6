================================================================================
Question 3: Dumping the Page Table of a Process (vmprint) in xv6
================================================================================

1. OVERVIEW & OBJECTIVES
--------------------------------------------------------------------------------
This implementation adds page table inspection and hierarchical dumping to xv6:
a) System Call `int vmprint(void)`:
   - Prints the full page directory and page tables of the calling process.
   - For valid page directory entries, prints the directory index, physical
     address of the next level table, and permission flags.
   - For valid leaf page table entries, prints the index, virtual address mapped,
     physical address of the frame, and flag bits (P, W, U).
   - Indents hierarchical levels (`..dir`, `..mid`, `..pte`) to show page table
     depth cleanly.
   - Prints only user space entries below `KERNBASE`, followed by a summary of
     high-memory kernel entries (trampoline and trapframe).
b) Stack Guard Page Identification:
   - Automatically detects and tags the stack guard page created by `exec()`,
     which is a mapped page with `PTE_U == 0` preventing user stack overflow.
   - Verifies that the count of user-accessible pages equals `sz / PGSIZE` minus
     the guard page.
c) User Verification Program (`vmprinttest.c`):
   - Invokes `vmprint()` at initial startup.
   - Invokes `vmprint()` after growing heap by 8 pages via `sbrk(8 * 4096)`,
     confirming 8 new PTEs appear.
   - Invokes `vmprint()` after shrinking heap by 1 page via `sbrk(-4096)`,
     confirming the top PTE is unmapped and removed.

2. FILES MODIFIED & ADDED
--------------------------------------------------------------------------------
- kernel/vm.c:
  * Implemented `kvmprint(pagetable_t)` to perform the hierarchical page table
    dump, guard page detection, and kernel entry summary.
- kernel/defs.h:
  * Declared `int kvmprint(pagetable_t);`.
  * Added `#define cprintf printk` and `#define printf printk`.
- kernel/sysproc.c:
  * Implemented `sys_vmprint(void)` syscall handler.
- kernel/syscall.h & kernel/syscall.c:
  * Registered `SYS_vmprint` (23).
- user/user.h & user/usys.pl:
  * Declared and exported `int vmprint(void);`.
- Makefile:
  * Cleared `DETFLAGS = ` to support paths containing spaces.
  * Added `$U/_vmprinttest\` to `UPROGS`.
- user/vmprinttest.c:
  * Comprehensive test program testing initial dump, heap growth, and shrinking.

3. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
From this directory:
    $ make qemu

Inside xv6 shell:
    $ vmprinttest

To exit QEMU:
    Press Ctrl-A then x

4. VERIFIED TEST OUTPUT LOG
--------------------------------------------------------------------------------
$ vmprinttest
============================================================
          Process Page Table Dump Test (vmprint)            
============================================================

------------------------------------------------------------
Phase 1: Initial Page Table Dump at Process Startup
------------------------------------------------------------
page table 0x0000000087f25000 (pid 3, sz 20480):
..dir[0]: pa 0x87f21000, flags: [P=1, W=0, U=0]
  ..mid[0]: pa 0x87f20000, flags: [P=1, W=0, U=0]
    ..pte[0]: va 0x0 -> pa 0x87f22000, flags: [P=1, W=0, U=1]
    ..pte[1]: va 0x1000 -> pa 0x87f1f000, flags: [P=1, W=0, U=1]
    ..pte[2]: va 0x2000 -> pa 0x87f1e000, flags: [P=1, W=1, U=1]
    ..pte[3]: va 0x3000 -> pa 0x87f1d000, flags: [P=1, W=1, U=0] <-- [STACK GUARD PAGE: PTE_U=0]
    ..pte[4]: va 0x4000 -> pa 0x87f1c000, flags: [P=1, W=1, U=1]
..dir[255]: pa 0x87f24000, flags: [P=1, W=0, U=0]
  ..mid[511]: pa 0x87f23000, flags: [P=1, W=0, U=0]
Kernel entries summary: 2 high-memory kernel entries mapped (trampoline/trapframe).
User accessible pages: 4 (sz/PGSIZE=5 minus guard page=1)

------------------------------------------------------------
Phase 2: Growing Heap with sbrk(8 * 4096)
------------------------------------------------------------
Allocated 8 new pages (32 KB) in heap.
page table 0x0000000087f25000 (pid 3, sz 53248):
..dir[0]: pa 0x87f21000, flags: [P=1, W=0, U=0]
  ..mid[0]: pa 0x87f20000, flags: [P=1, W=0, U=0]
    ..pte[0]: va 0x0 -> pa 0x87f22000, flags: [P=1, W=0, U=1]
    ..pte[1]: va 0x1000 -> pa 0x87f1f000, flags: [P=1, W=0, U=1]
    ..pte[2]: va 0x2000 -> pa 0x87f1e000, flags: [P=1, W=1, U=1]
    ..pte[3]: va 0x3000 -> pa 0x87f1d000, flags: [P=1, W=1, U=0] <-- [STACK GUARD PAGE: PTE_U=0]
    ..pte[4]: va 0x4000 -> pa 0x87f1c000, flags: [P=1, W=1, U=1]
    ..pte[5]: va 0x5000 -> pa 0x87f26000, flags: [P=1, W=1, U=1]
    ..pte[6]: va 0x6000 -> pa 0x87f54000, flags: [P=1, W=1, U=1]
    ..pte[7]: va 0x7000 -> pa 0x87f53000, flags: [P=1, W=1, U=1]
    ..pte[8]: va 0x8000 -> pa 0x87f49000, flags: [P=1, W=1, U=1]
    ..pte[9]: va 0x9000 -> pa 0x87f46000, flags: [P=1, W=1, U=1]
    ..pte[10]: va 0xa000 -> pa 0x87f43000, flags: [P=1, W=1, U=1]
    ..pte[11]: va 0xb000 -> pa 0x87f27000, flags: [P=1, W=1, U=1]
    ..pte[12]: va 0xc000 -> pa 0x87f28000, flags: [P=1, W=1, U=1]
..dir[255]: pa 0x87f24000, flags: [P=1, W=0, U=0]
  ..mid[511]: pa 0x87f23000, flags: [P=1, W=0, U=0]
Kernel entries summary: 2 high-memory kernel entries mapped (trampoline/trapframe).
User accessible pages: 12 (sz/PGSIZE=13 minus guard page=1)

------------------------------------------------------------
Phase 3: Shrinking Heap with sbrk(-4096)
------------------------------------------------------------
Deallocated 1 page (4 KB) from heap.
page table 0x0000000087f25000 (pid 3, sz 49152):
..dir[0]: pa 0x87f21000, flags: [P=1, W=0, U=0]
  ..mid[0]: pa 0x87f20000, flags: [P=1, W=0, U=0]
    ..pte[0]: va 0x0 -> pa 0x87f22000, flags: [P=1, W=0, U=1]
    ..pte[1]: va 0x1000 -> pa 0x87f1f000, flags: [P=1, W=0, U=1]
    ..pte[2]: va 0x2000 -> pa 0x87f1e000, flags: [P=1, W=1, U=1]
    ..pte[3]: va 0x3000 -> pa 0x87f1d000, flags: [P=1, W=1, U=0] <-- [STACK GUARD PAGE: PTE_U=0]
    ..pte[4]: va 0x4000 -> pa 0x87f1c000, flags: [P=1, W=1, U=1]
    ..pte[5]: va 0x5000 -> pa 0x87f26000, flags: [P=1, W=1, U=1]
    ..pte[6]: va 0x6000 -> pa 0x87f54000, flags: [P=1, W=1, U=1]
    ..pte[7]: va 0x7000 -> pa 0x87f53000, flags: [P=1, W=1, U=1]
    ..pte[8]: va 0x8000 -> pa 0x87f49000, flags: [P=1, W=1, U=1]
    ..pte[9]: va 0x9000 -> pa 0x87f46000, flags: [P=1, W=1, U=1]
    ..pte[10]: va 0xa000 -> pa 0x87f43000, flags: [P=1, W=1, U=1]
    ..pte[11]: va 0xb000 -> pa 0x87f27000, flags: [P=1, W=1, U=1]
..dir[255]: pa 0x87f24000, flags: [P=1, W=0, U=0]
  ..mid[511]: pa 0x87f23000, flags: [P=1, W=0, U=0]
Kernel entries summary: 2 high-memory kernel entries mapped (trampoline/trapframe).
User accessible pages: 11 (sz/PGSIZE=12 minus guard page=1)

============================================================
Verification Summary:
1. Hierarchical two/three-level page table structure clearly dumped.
2. Stack guard page identified with PTE_U=0.
3. User-accessible pages count matches sz/PGSIZE minus guard page.
4. Heap expansion (+8 pages) and heap shrink (-1 page) verified.
============================================================
