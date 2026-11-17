================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 10 - QUESTION 2
Virtual-to-Physical Address Translation (vtop)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 10, Question 2 (10b) [Also referenced as Lab 8a]
- Topic          : Virtual Memory & Address Translation
- Test Program   : user/vttest.c
- Shell Command  : $ vttest

Objective:
Implement a kernel system call `uint64 vtop(uint64 virtual_address)` that translates
any valid user virtual address into its physical RAM address:
1. Walk the per-process page table using `walkpgdir()` (exposing `walk()` from `kernel/vm.c`).
2. Verify that the Page Table Entry (PTE) is:
   - Valid and Present (`PTE_V == 1`).
   - Accessible from User mode (`PTE_U == 1`).
3. Compute the physical address:
   Extract the Physical Page Number (PPN) from the PTE, scale to base address,
   and combine with the 12-bit page offset ($va \ \& \ 0xFFF$).
4. Return `-1` if the virtual address is unmapped, invalid, or supervisor-only.
5. User Verification (`vttest.c`):
   Translate code, global data, stack, heap (`sbrk`), offset arithmetic, and
   unmapped addresses.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Virtual Memory Decoupling:
   - Virtual memory provides isolation: each process executes in its own contiguous
     address space starting at `0x0`, completely oblivious to the fragmented physical
     RAM layout.
   - Translation is performed in hardware by the Memory Management Unit (MMU) using
     page tables set up by the kernel.
2. Translation Mathematics (RISC-V Sv39):
   - Page Size: 4 KB ($2^{12} = 4096$ bytes).
   - Offset: Lower 12 bits of virtual address: $\text{Offset} = VA \ \& \ \text{0xFFF}$.
   - Leaf PTE Physical Base: $\text{Base PA} = \text{PTE2PA}(*pte) = (*pte \gg 10) \ll 12$.
   - Complete Physical Address:
     $$PA = \text{Base PA} \mid \text{Offset}$$


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/vm.c           | Modified | Implemented walkpgdir() and vtop().
kernel/defs.h         | Modified | Declared walkpgdir() and vtop().
kernel/sysproc.c      | Modified | Implemented sys_vtop() system call handler.
kernel/syscall.h      | Modified | Defined SYS_vtop (23).
kernel/syscall.c      | Modified | Registered sys_vtop in syscall table.
user/user.h           | Modified | Declared uint64 vtop(uint64) in user API.
user/usys.pl          | Modified | Added entry("vtop") syscall stub.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_vttest.
user/vttest.c         | Created  | Test program verifying code, data, stack, heap,
                      |          | and unmapped address translation.


4. DETAILED CODE WALKTHROUGH (kernel/vm.c)
--------------------------------------------------------------------------------
```c
pte_t *
walkpgdir(pagetable_t pagetable, uint64 va, int alloc)
{
  return walk(pagetable, va, alloc);
}

uint64
vtop(uint64 va)
{
  struct proc *p = myproc();
  if (va >= MAXVA)
    return -1;

  pte_t *pte = walkpgdir(p->pagetable, va, 0);
  if (pte == 0)
    return -1; // Unmapped page

  if ((*pte & PTE_V) == 0 || (*pte & PTE_U) == 0)
    return -1; // Invalid or supervisor-only page

  uint64 pa = PTE2PA(*pte) | (va & 0xFFF);
  return pa;
}
```


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
uint64 vtop(uint64 virtual_address)
- Syscall Number: SYS_vtop (23)
- Parameters: `virtual_address` (64-bit virtual memory address).
- Return Value: Translated 64-bit physical address, or -1 if unmapped/invalid.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/vtop

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ vttest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
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

Analysis:
- The base of heap page `0x5000` translated to `0x87F26000`.
- An interior offset `0x5100` (offset = `0x100`) translated to `0x87F26100`
  (`0x87F26000 | 0x100`), confirming accurate preservation of intra-page offsets.
- Invalid addresses (`0x7FFFFFF0` and `0x4005000`) returned -1 without faulting.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is the page offset identical between virtual and physical addresses?
A1: Because virtual memory paging maps entire 4 KB blocks (pages) into physical
    blocks (frames). Intra-page alignment is identical; only the frame number is
    translated by the page table.

Q2: What is the maximum virtual address accessible in user mode in xv6?
A2: `MAXVA = (1ULL << (9 + 9 + 9 + 12 - 1)) = 0x3FFFFFF000` (approximately 256 GB),
    though user processes in xv6 typically use addresses below `0x3FFFFFF000 - 2 * PGSIZE`
    (below the trampoline and trapframe).

Q3: Why does `vtop()` return -1 when `(*pte & PTE_U) == 0`?
A3: `PTE_U` indicates user accessibility. If `PTE_U` is 0, the page belongs to
    the kernel. A user-space program must not be permitted to inspect or infer
    kernel-only page mappings.
================================================================================
