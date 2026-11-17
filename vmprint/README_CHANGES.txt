================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 8 - QUESTION 3
Dumping the Page Table of a Process (vmprint)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 8, Question 3 (8c) [Also referenced as Lab 9a]
- Topic          : Virtual Memory & Page Table Hierarchy
- Test Program   : user/vmprinttest.c
- Shell Command  : $ vmprinttest

Objective:
Implement a new system call `int vmprint(void)` to print a hierarchical tree dump
of the calling process's page directory and page tables:
1. Walk the 3-level Sv39 page directory below `KERNBASE`.
2. Format the tree depth using visual indentation:
   - Root directory: `..dir[index]: pa <addr>`
   - Mid directory : `  ..mid[index]: pa <addr>`
   - Leaf table PTE: `    ..pte[index]: va <va> -> pa <pa>, flags: [P=..., W=..., U=...]`
3. Stack Guard Page Identification:
   Automatically identify and tag the guard page allocated by `exec()`, which is a
   mapped page with `PTE_U == 0` that separates the stack from the heap.
4. User Verification:
   Test baseline page table layout, grow heap by 8 pages via `sbrk(+8*4096)`, and
   shrink heap by 1 page via `sbrk(-4096)` to verify page table responsiveness.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. RISC-V Sv39 Multi-Level Page Table Tree:
   - Why not a single flat page table?
     A 39-bit virtual address space has $2^{27}$ pages. A flat table would require
     $2^{27} \times 8 \text{ bytes} = 1 \text{ GB}$ of contiguous RAM per process!
   - Multi-Level Paging (Trie / Radix Tree):
     Tables are allocated only for active address regions. Unmapped virtual address
     ranges consume zero intermediate page tables, saving huge amounts of physical memory.
2. The Stack Guard Page:
   - In xv6's `exec()`, user memory is arranged as:
       `0x0` : ELF text (code)
       `0x1000` : read-only rodata
       `0x2000` : data / BSS
       `0x3000` : Guard page (`PTE_U = 0`)
       `0x4000` : User stack page
       `0x5000` : Start of heap (grows upward via `sbrk`)
   - The guard page at `0x3000` has the user bit cleared (`PTE_U == 0`). If the user
     stack overflows downward, the CPU MMU faults immediately rather than silently
     corrupting data pages.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/vm.c           | Modified | Implemented kvmprint(pagetable_t) with tree
                      |          | formatting, guard page detection, and statistics.
kernel/defs.h         | Modified | Declared int kvmprint(pagetable_t).
kernel/sysproc.c      | Modified | Implemented sys_vmprint(void) syscall handler.
kernel/syscall.h      | Modified | Defined SYS_vmprint (23).
kernel/syscall.c      | Modified | Registered sys_vmprint in syscall table.
user/user.h           | Modified | Declared int vmprint(void) in user API.
user/usys.pl          | Modified | Added entry("vmprint") syscall stub.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_vmprinttest.
user/vmprinttest.c    | Created  | Test harness verifying baseline dump, heap growth
                      |          | (+8 pages), and heap shrink (-1 page).


4. DETAILED CODE WALKTHROUGH (kernel/vm.c)
--------------------------------------------------------------------------------
`kvmprint(pagetable_t pagetable)`:
- Reads `sz = myproc()->sz`.
- Iterates `i` from 0 to 511 (Level 2 directory):
  * If valid, prints `..dir[i]: pa <pa>`.
  * Iterates `j` from 0 to 511 (Level 1 directory):
    - If valid, prints `..mid[j]: pa <pa>`.
    - Iterates `k` from 0 to 511 (Level 0 leaf table):
      * If valid, computes `va = (i << 30) | (j << 21) | (k << 12)`.
      * Checks flags: if `PTE_U == 0`, tags as `[STACK GUARD PAGE: PTE_U=0]`.
      * Prints `..pte[k]: va -> pa, flags: [P, W, U]`.


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
int vmprint(void)
- Syscall Number: SYS_vmprint (23)
- Return Value: Number of user-accessible pages on success.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/vmprint

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ vmprinttest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ vmprinttest
============================================================
       Hierarchical Page Table Dump Test (vmprint)          
============================================================

------------------------------------------------------------
Phase 1: Initial Baseline Page Table Layout
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


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is the stack guard page created with `PTE_U == 0`?
A1: The guard page is mapped below the user stack without user permission (`PTE_U = 0`).
    If a process uses more stack space than allocated (e.g. infinite recursion),
    the hardware MMU generates a page fault as soon as the stack pointer crosses
    into the guard page, allowing the kernel to terminate the program safely.

Q2: Why are high-memory kernel entries mapped at `..dir[255]`?
A2: In RISC-V Sv39, `MAXVA = 1 << 38 = 0x3FFFFFF000`. The trampoline and trapframe
    are mapped at the very top of virtual memory (`0x3FFFFFF000` and `0x3FFFFFE000`),
    which falls into the highest directory index `255` (`(MAXVA - PGSIZE) >> 30 = 255`).

Q3: How many page table entries (PTEs) does a single physical page table contain?
A3: Each page table is 4096 bytes ($2^{12}$). Each PTE is 8 bytes ($2^3 = 64$ bits).
    Therefore, each page table page contains exactly $4096 / 8 = 512$ entries ($2^9$).
    This is why each level of VPN is exactly 9 bits ($3 \times 9 = 27$ bits total VPN).
================================================================================
