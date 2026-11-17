================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 8 - QUESTION 1
Virtual to Physical Address Translation (va2pa)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 8, Question 1 (8a)
- Topic          : Virtual Memory & Page Table Walking
- Test Program   : user/vatest.c
- Shell Command  : $ vatest

Objective:
Implement a new system call `uint64 va2pa(uint64 va)` in xv6 that translates a
user virtual address into its corresponding physical address:
1. Walk the calling process's multi-level page table using the kernel page table
   walker (`walk()`).
2. Verify validity and permissions:
   - Check that the Present bit (`PTE_V`) is set.
   - Check that the User-accessible bit (`PTE_U`) is set.
   - If the page is unmapped, invalid, or supervisor-only, return -1.
3. Diagnostic Logging:
   - Print the translation breakdown: Page Directory indices (`VPN[2]`, `VPN[1]`),
     Page Table index (`VPN[0]`), page offset, permission flag bits (P, W, U),
     and the computed physical address.
4. User Verification:
   - Test global data, local stack, and heap addresses.
   - Verify unmapped address handling (`0x7FFFFFFF`).
   - Fork a child process to prove that parent and child share the same virtual
     address but map to distinct physical pages due to page duplication in `fork()`.


2. THEORETICAL BACKGROUND & RISC-V SV39 PAGING
--------------------------------------------------------------------------------
1. RISC-V Sv39 Paging Architecture:
   - The xv6 kernel on RISC-V operates in 64-bit Sv39 paging mode:
     * Virtual Address (39 bits):
       - VPN[2] (bits 38..30, 9 bits): Index into Level 2 Page Directory (Root).
       - VPN[1] (bits 29..21, 9 bits): Index into Level 1 Page Directory.
       - VPN[0] (bits 20..12, 9 bits): Index into Level 0 Page Table.
       - Offset (bits 11..0, 12 bits): Byte offset within the 4 KB page ($2^{12} = 4096$).
     * Physical Address (56 bits):
       - PPN (bits 55..12, 44 bits): Physical Page Number.
       - Offset (bits 11..0, 12 bits): Page offset directly copied from the virtual address.

2. Page Table Entry (PTE) Format (64 bits):
   - Bits 53..10 : Physical Page Number (PPN).
   - Bit 0 (`PTE_V`): Valid / Present bit (1 if page is resident).
   - Bit 1 (`PTE_R`): Read permission.
   - Bit 2 (`PTE_W`): Write permission.
   - Bit 3 (`PTE_X`): Execute permission.
   - Bit 4 (`PTE_U`): User accessible (must be 1 for user programs to access).

3. Translation Formula:
   $$PA = \text{PTE2PA}(*pte) \mid (va \ \& \ 0xFFF)$$


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/defs.h         | Modified | Declared walkpgdir() and va2pa().
kernel/vm.c           | Modified | Implemented walkpgdir() and va2pa() with validity
                      |          | checks, diagnostic breakdown, and PA calculation.
kernel/syscall.h      | Modified | Defined SYS_va2pa (23).
kernel/syscall.c      | Modified | Registered sys_va2pa in syscall table.
kernel/sysproc.c      | Modified | Implemented sys_va2pa() handler.
user/user.h           | Modified | Declared uint64 va2pa(uint64) in user API.
user/usys.pl          | Modified | Added entry("va2pa") syscall stub.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_vatest.
user/vatest.c         | Created  | Comprehensive test suite translating global, stack,
                      |          | heap, unmapped, and fork duplicate pages.


4. DETAILED CODE WALKTHROUGH (kernel/vm.c)
--------------------------------------------------------------------------------
Implementation of `va2pa`:
```c
uint64
va2pa(uint64 va)
{
  struct proc *p = myproc();
  if (va >= MAXVA)
    return -1;

  // Extract page directory and table indices
  int vpn2 = (va >> 30) & 0x1FF;
  int vpn1 = (va >> 21) & 0x1FF;
  int vpn0 = (va >> 12) & 0x1FF;
  int offset = va & 0xFFF;

  // Walk page table to find Level 0 PTE
  pte_t *pte = walk(p->pagetable, va, 0);
  if (pte == 0)
    return -1; // Unmapped page

  // Check validity and user accessibility
  if ((*pte & PTE_V) == 0 || (*pte & PTE_U) == 0)
    return -1; // Invalid or kernel-only

  int flag_p = (*pte & PTE_V) ? 1 : 0;
  int flag_w = (*pte & PTE_W) ? 1 : 0;
  int flag_u = (*pte & PTE_U) ? 1 : 0;

  uint64 pa = PTE2PA(*pte) | offset;

  // Print diagnostic translation breakdown
  printk("[cprintf/kernel] va2pa: VA 0x%lx -> PDE_idx:[%d,%d], PTE_idx:%d, Offset:0x%x, Flags:[P=%d, W=%d, U=%d] -> PA:0x%lx\n",
         va, vpn2, vpn1, vpn0, offset, flag_p, flag_w, flag_u, pa);

  return pa;
}
```


5. SYSTEM CALL SPECIFICATION
--------------------------------------------------------------------------------
uint64 va2pa(uint64 virtual_address)
- Syscall Number: SYS_va2pa (23)
- Parameters: `virtual_address` (64-bit user virtual address).
- Return Value: Physical address (PA) on success, -1 on failure (unmapped or invalid).


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/va2pa

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ vatest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
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

Analysis:
- The global variable at `0x2000` mapped to physical frame `0x87F1E000`.
- The stack variable at `0x4FAC` mapped to `0x87F1CFAC` with identical offset `0xFAC`.
- Unmapped address `0x7FFFFFFF` returned -1 cleanly without kernel panic.
- On `fork()`, Child virtual address `0x2000` mapped to physical address `0x87F45000`,
  while Parent mapped to `0x87F1E000`, proving deep physical page duplication.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: How does the RISC-V MMU translate virtual addresses to physical addresses?
A1: The hardware MMU uses the root page table address stored in the `satp` CSR.
    It takes `VPN[2]` (bits 38:30) to index the Level 2 root table, obtaining the
    Level 1 table address. It uses `VPN[1]` (bits 29:21) to obtain the Level 0
    table address. It uses `VPN[0]` (bits 20:12) to retrieve the leaf PTE. The
    PPN from the leaf PTE is concatenated with the 12-bit offset to produce the PA.

Q2: What is the purpose of the `alloc` argument in xv6's `walk()` function?
A2: In `walk(pagetable, va, alloc)`: if `alloc == 1`, intermediate page directory
    tables are dynamically allocated using `kalloc()` if they do not yet exist.
    In `va2pa`, we pass `alloc = 0` because address translation is a read-only query
    that must never allocate memory.

Q3: Why did Parent and Child have the same virtual address but different physical addresses?
A3: In xv6, `fork()` calls `uvmcopy()`, which iterates over the parent's address
    space, allocates fresh physical memory frames for each page, copies the memory
    contents, and maps them into the child's new page table at the identical virtual
    addresses.

Q4: What happens if `PTE_U` is 0 when a user program accesses that virtual address?
A4: The RISC-V hardware MMU triggers a Page Fault exception (Load/Store Page Fault,
    `scause == 13` or `15`) because user-mode code is prohibited from accessing
    supervisor-only (kernel) pages.
================================================================================
