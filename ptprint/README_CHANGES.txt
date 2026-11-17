================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 9 - QUESTION 1
Printing the Page Table of a Process (vmprint / ptprint)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 9, Question 1 (9a) [Also referenced as Lab 8c]
- Topic          : Virtual Memory & Page Table Enumeration
- Test Program   : user/pttest.c
- Shell Command  : $ pttest

Objective:
Implement a system call `int vmprint(void)` that walks and displays all valid
user virtual-to-physical address mappings in the calling process's page table:
1. Enumerate all resident pages below `KERNBASE`.
2. For each valid page, display:
   - Virtual Address (VA)
   - Physical Frame Address (PA)
   - Permission flag bits: Present (`PTE_V`), Writable (`PTE_W`), User (`PTE_U`).
3. Summarize memory consumption:
   - Total valid user pages mapped.
   - Total page-table pages allocated for directory nodes.
4. User Verification:
   Verify that calling `sbrk(4 * 4096)` increases the reported user page count
   by exactly 4.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Virtual Memory Translation in RISC-V:
   - Virtual Address Space: 39-bit virtual addresses structured into three 9-bit
     VPN levels ($VPN[2], VPN[1], VPN[0]$) and a 12-bit offset.
   - Address Reconstruction: Given directory indices $i, j, k$, the base virtual address is:
       $$VA = (i \ll 30) \mid (j \ll 21) \mid (k \ll 12)$$
   - Physical Frame Extraction:
       $$PA = \text{PTE2PA}(pte)$$
2. Permission Bits:
   - `PTE_V`: Page valid and present in memory.
   - `PTE_W`: Page is writable (read-only code pages have $W=0$).
   - `PTE_U`: Page accessible in user mode (supervisor-only kernel pages have $U=0$).


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/vm.c           | Modified | Implemented vmprint() walking 3-level page table,
                      |          | formatting address rows, and counting pages.
kernel/sysproc.c      | Modified | Implemented sys_vmprint(void) syscall handler.
kernel/defs.h         | Modified | Added int vmprint(void) declaration.
kernel/syscall.h      | Modified | Defined SYS_vmprint (23).
kernel/syscall.c      | Modified | Registered sys_vmprint in dispatch table.
user/user.h           | Modified | Declared int vmprint(void) in user API.
user/usys.pl          | Modified | Added entry("vmprint") syscall stub.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_pttest.
user/pttest.c         | Created  | Test program verifying baseline page dump and
                      |          | 4-page heap expansion.


4. DETAILED CODE WALKTHROUGH (kernel/vm.c)
--------------------------------------------------------------------------------
`vmprint(void)`:
1. Obtains current process `p = myproc()` and page table root `p->pagetable`.
2. Iterates over Level 2 table entries `i \in [0..511]`.
3. If entry is valid, increments `pt_pages` and descends into Level 1 `j \in [0..511]`.
4. If Level 1 entry is valid, increments `pt_pages` and descends into Level 0 `k \in [0..511]`.
5. For each leaf PTE with `(pte & PTE_V) && (pte & PTE_U)`:
   - Computes `va = (i << 30) | (j << 21) | (k << 12)`.
   - Computes `pa = PTE2PA(pte)`.
   - Prints formatted row: `va -> pa [P, W, U]`.
   - Increments `valid_pages`.
6. Prints totals and returns `valid_pages`.


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
int vmprint(void)
- Syscall Number: SYS_vmprint (23)
- Return Value: Total count of valid user pages.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/ptprint

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ pttest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ pttest
============================================================
         Page Table Printing Test Program (pttest)          
============================================================

--- 1. Calling vmprint() at initial start-up ---
=== Page Table Dump for PID 3 (pttest) ===
  Virtual Addr       Physical Addr      Flags [P, W, U]
  -----------------------------------------------------
  0x0  ->  0x87f22000    [P: 1, W: 0, U: 1]
  0x1000  ->  0x87f1f000    [P: 1, W: 0, U: 1]
  0x2000  ->  0x87f1e000    [P: 1, W: 1, U: 1]
  0x4000  ->  0x87f1c000    [P: 1, W: 1, U: 1]
  -----------------------------------------------------
  Total valid user pages : 4
  Total page-table pages : 5
=====================================================
Initial valid user pages reported by vmprint: 4

--- 2. Allocating 4 pages via sbrk(4 * 4096 = 16384 bytes) ---
sbrk allocated memory starting at: 0x5000
Touched all 4 newly allocated pages.

--- 3. Calling vmprint() after sbrk(4 * 4096) ---
=== Page Table Dump for PID 3 (pttest) ===
  Virtual Addr       Physical Addr      Flags [P, W, U]
  -----------------------------------------------------
  0x0  ->  0x87f22000    [P: 1, W: 0, U: 1]
  0x1000  ->  0x87f1f000    [P: 1, W: 0, U: 1]
  0x2000  ->  0x87f1e000    [P: 1, W: 1, U: 1]
  0x4000  ->  0x87f1c000    [P: 1, W: 1, U: 1]
  0x5000  ->  0x87f26000    [P: 1, W: 1, U: 1]
  0x6000  ->  0x87f54000    [P: 1, W: 1, U: 1]
  0x7000  ->  0x87f53000    [P: 1, W: 1, U: 1]
  0x8000  ->  0x87f49000    [P: 1, W: 1, U: 1]
  -----------------------------------------------------
  Total valid user pages : 8
  Total page-table pages : 5
=====================================================
New valid user pages reported by vmprint: 8

--- 4. Verification Analysis ---
Initial user pages : 4
After sbrk(+4)     : 8
Difference         : 4 pages
SUCCESS: Exactly four new pages appeared in the page table!
============================================================

Analysis:
- Initial state shows 4 user pages: code (0x0, W=0), rodata (0x1000, W=0),
  data (0x2000, W=1), and user stack (0x4000, W=1).
- Notice `0x3000` is omitted because it is the guard page (`PTE_U == 0`).
- After allocating 4 heap pages (`0x5000`, `0x6000`, `0x7000`, `0x8000`), exactly
  4 new user pages appear with write permissions `[P: 1, W: 1, U: 1]`.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is address 0x3000 not printed in the user-accessible table?
A1: Address `0x3000` is the stack guard page. In `exec()`, it is mapped with
    `PTE_U = 0` (user permission cleared). Because `vmprint()` filters for user-accessible
    pages (`(pte & PTE_U) != 0`), the guard page is appropriately excluded.

Q2: How is the virtual address reconstructed in `vmprint()`?
A2: Using bit shifts: `va = (i << 30) | (j << 21) | (k << 12)`. Bits 38:30 come from
    the root table index `i`, bits 29:21 from mid table index `j`, and bits 20:12 from
    leaf table index `k`.

Q3: Why are pages 0x0 and 0x1000 marked with `W = 0`?
A3: `0x0` contains the executable machine code (.text), and `0x1000` contains read-only
    string literals and constants (.rodata). Marking them read-only (`W = 0`) prevents
    accidental or malicious code corruption.
================================================================================
