# xv6 Operating Systems Lab Assignments

This repository contains standalone implementations of various Operating Systems lab assignments built on MIT xv6 RISC-V. Below is the mapping of each assignment question and the corresponding folder with its question description and implementation details.

---

## Lab 3: CPU Scheduling Algorithms (Set 1)

* **3a `fcfs`**
  * **Question**: First-Come, First-Served (FCFS) Non-Preemptive Scheduler
  * **Description**: Replaces the default xv6 round-robin scheduler with a non-preemptive FCFS scheduler. Processes are scheduled based on their creation time (`ctime`), running without preemption until completion, sleep, or exit.
  * **Test Program**: `user/fcfstest.c`

* **3b `priority_aging`**
  * **Question**: Priority-Based Preemptive Scheduler with Aging
  * **Description**: Replaces round-robin with a priority-based preemptive scheduler (priorities 1–20, where 1 is highest; default 10). Includes anti-starvation aging that raises a process's priority if it waits in `RUNNABLE` for more than 30 ticks. Adds `set_priority()` and `get_priority()` system calls.
  * **Test Program**: `user/priotest.c`

* **3c `sjf`**
  * **Question**: Shortest Job First (SJF) Non-Preemptive Scheduler
  * **Description**: Implements non-preemptive SJF scheduling using an estimated burst time declared by the process via a new `set_burst_time()` system call. The scheduler selects the runnable process with the shortest burst time.
  * **Test Program**: `user/sjftest.c`

---

## Lab 4: CPU Scheduling Algorithms (Set 2)

* **4a `srtf`**
  * **Question**: Shortest Remaining Time First (SRTF) Preemptive Scheduler
  * **Description**: Preemptive variant of SJF. Processes specify their total burst time via `set_burst_time()`. The scheduler dynamically tracks remaining runtime and preempts the running process when a new job with a shorter remaining time becomes runnable.
  * **Test Program**: `user/srtftest.c`

* **4b `hrrn`**
  * **Question**: Highest Response Ratio Next (HRRN) Non-Preemptive Scheduler
  * **Description**: Non-preemptive scheduler that selects processes based on Response Ratio: $R = \frac{W + S}{S}$ (where $W$ is waiting time and $S$ is estimated service/burst time). Prevents starvation of long jobs as their waiting time increases.
  * **Test Program**: `user/hrrntest.c`

* **4c `mlfq`**
  * **Question**: Multilevel Feedback Queue (MLFQ) Scheduler
  * **Description**: Implements a 3-level priority feedback queue (Queue 0: 1 tick, Queue 1: 2 ticks, Queue 2: 4 ticks). New processes enter Queue 0; CPU-bound processes that exhaust their time slice are demoted to lower queues. Includes periodic priority boosting to prevent starvation.
  * **Test Program**: `user/mlfqtest.c`

---

## Lab 5: Process Concurrency & Synchronization

* **5a `peterson`**
  * **Question**: Mutual Exclusion using Peterson's Algorithm
  * **Description**: Demonstrates mutual exclusion between two processes (parent and child) sharing a critical section purely in software using Peterson's algorithm (`flag` array and `turn` variable) without kernel lock primitives.
  * **Test Program**: `user/peterson.c`

* **5b `prodcons`**
  * **Question**: Producer-Consumer Problem with Bounded Buffer using Semaphores
  * **Description**: Implements the classic bounded-buffer problem using kernel counting semaphores (`empty`, `full`, `mutex`) coordinating multiple producer and consumer processes accessing a shared circular buffer.
  * **Test Program**: `user/prodcons.c`

* **5c `readwrite`**
  * **Question**: Readers-Writers Problem using Semaphores
  * **Description**: Solves the readers-writers synchronization problem (reader-preference) using kernel counting semaphores (`mutex`, `wrt`) and shared reader count (`readcount`) allowing concurrent readers while ensuring exclusive writer access.
  * **Test Program**: `user/readwrite.c`

* **5d `dining`**
  * **Question**: Dining Philosophers Problem with Deadlock Avoidance using Semaphores
  * **Description**: Deadlock-free solution to the Dining Philosophers problem for 5 philosophers. Uses state tracking (`THINKING`, `HUNGRY`, `EATING`), an array of per-philosopher semaphores, and conditional grabbing of both chopsticks simultaneously to eliminate deadlock.
  * **Test Program**: `user/dining.c`

---

## Lab 6: Deadlock Avoidance, Detection & Prevention

* **6a `bankers`**
  * **Question**: Banker's Algorithm Simulation for Deadlock Avoidance
  * **Description**: User-level simulation of Dijkstra's Banker's Algorithm. Evaluates system safety, determines safe execution sequences, and dynamically approves or denies resource allocation requests by rolling back unsafe allocations.
  * **Test Program**: `user/bankers.c`

* **6b `deadlockdetect`**
  * **Question**: Deadlock Detection using Resource Allocation Graph (RAG)
  * **Description**: Implements deadlock detection using Allocation and Request matrices to construct a Wait-For Graph (WFG). Uses a 3-color Depth-First Search (DFS) cycle-detection algorithm to identify deadlocked processes.
  * **Test Program**: `user/deadlockdetect.c`

* **6c `resourceorder`**
  * **Question**: Deadlock Prevention via Resource Ordering (Hierarchical Allocation)
  * **Description**: Implements Havender's resource ordering technique on actual running xv6 processes. Assigns linear global order IDs to resources ($F(R_i) < F(R_j)$) so processes request resources in strictly increasing order, mathematically eliminating the Circular Wait condition.
  * **Test Program**: `user/resourceorder.c`

* **6d `syncdeadlock`**
  * **Question**: Combined Multi-Resource Synchronization & Deadlock Avoidance
  * **Description**: Manages 5 concurrent processes competing for 3 limited shared resource types (Printers, Scanners, Disks). Enforces hierarchical resource ordering and semaphore-based synchronization to prevent deadlocks and race conditions.
  * **Test Program**: `user/syncdeadlock.c`

---

## Lab 7: Kernel Threads, Sync, Deadlocks & Priority

* **7a `threads`**
  * **Question**: Implementing User-Level / Kernel-Supported Threads (`clone` & `join`)
  * **Description**: Implements kernel-supported lightweight threads in xv6. Adds `clone()` to spawn threads sharing the parent's address space (`uvmshare`) with independent stacks, and `join()` to reap finished threads.
  * **Test Program**: `user/threadtest.c`

* **7b `sync`**
  * **Question**: Process/Thread Synchronization using Counting Semaphores
  * **Description**: Introduces kernel-level counting semaphores (`sem_init`, `sem_wait`, `sem_signal`) with sleep/wakeup queues to eliminate race conditions among concurrent threads sharing address space.
  * **Test Program**: `user/synctest.c`

* **7c `deadlock`**
  * **Question**: Deadlock Detection and Banker's Deadlock Avoidance in xv6
  * **Description**: Comprehensive deadlock testing suite including vector/matrix deadlock detection reduction, Banker's avoidance safety check with rollback, and real IPC pipe circular-wait simulation.
  * **Test Program**: `user/deadlock.c`

* **7d `priority`**
  * **Question**: Priority-Based Preemptive Scheduler with Aging
  * **Description**: Implements priority scheduling (values 1–20) with timer-driven aging updates inside `clockintr()` and tie-breaking round-robin.
  * **Test Program**: `user/priotest.c`

---

## Lab 8: Virtual Memory & Page Table Management

* **8a `va2pa`**
  * **Question**: Virtual to Physical Address Translation
  * **Description**: Implements `int va2pa(int va)` system call that walks the process's page table using `walkpgdir()`/`walk()`, prints page directory index, page table index, offset, and flag bits (P, W, U), and returns the physical address (-1 if unmapped or invalid).
  * **Test Program**: `user/vatest.c`

* **8b `meminfo`**
  * **Question**: Physical Memory Statistics
  * **Description**: Tracks free physical pages in `kalloc.c` under `kmem.lock`. Implements system call `int getmeminfo(struct meminfo *mi)` reporting total pages, free pages, user-allocated pages (`sz / PGSIZE`), and page-table pages.
  * **Test Program**: `user/memtest.c`

* **8c `vmprint`**
  * **Question**: Dumping the Page Table of a Process
  * **Description**: Implements system call `int vmprint(void)` to print a hierarchical tree dump of the process's page directory and page tables below `KERNBASE`, displaying virtual addresses, physical frame addresses, flags (P, W, U), and the stack guard page.
  * **Test Program**: `user/vmprinttest.c`

* **8d `lazy`**
  * **Question**: Lazy Memory Allocation (Demand Paging for Heap)
  * **Description**: Replaces eager heap allocation in `sbrk()` so that `n > 0` only increments `p->sz`. Traps page faults (`T_PGFLT`) in `trap.c` to dynamically allocate and map zeroed physical frames on demand.
  * **Test Program**: `user/lazytest.c`

---

## Lab 9: Page Table Printing, Faults & Replacement (FIFO & LRU)

* **9a `ptprint`**
  * **Question**: Printing the Page Table of a Process (`vmprint` / `ptprint`)
  * **Description**: System call `int vmprint(void)` that walks the page directory and prints valid user virtual addresses, physical frames, permission flags (P, W, U), total user pages, and total page-table pages used.
  * **Test Program**: `user/pttest.c`

* **9b `lazyfault`**
  * **Question**: Handling Page Faults (Lazy Allocation / `lazyfault`)
  * **Description**: Lazy memory allocation where `sbrk()` increases `p->sz` without allocating frames. Intercepts page faults in `trap.c`, maps zeroed physical pages, counts page faults in `p->page_faults`, and exposes the count via `getfaults()`.
  * **Test Program**: `user/faulttest.c`

* **9c `pagerepl`**
  * **Question**: Page Replacement Algorithms (FIFO and LRU)
  * **Description**: User-space simulation comparing First-In First-Out (FIFO) and Least Recently Used (LRU) algorithms on reference string `7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1` across 3, 4, and 5 frames, calculating page faults and hit ratios.
  * **Test Program**: `user/pagerepl.c`

---

## Lab 10: Memory Management, Protection & Replacement (FIFO, LRU, Optimal)

* **10a `memstats`**
  * **Question**: Tracking Physical Memory Allocation
  * **Description**: Modifies `kalloc()` and `kfree()` to maintain lifetime counters for total allocated pages, total freed pages, and available physical memory. Exposes them to user programs via `int memstats(void)`.
  * **Test Program**: `user/memtest.c`

* **10b `vtop`**
  * **Question**: Virtual-to-Physical Address Translation (`vtop`)
  * **Description**: Implements system call `int vtop(int virtual_address)` that walks the page table, checks validity and presence, extracts the physical frame address, and combines it with the page offset to return the physical address.
  * **Test Program**: `user/vttest.c`

* **10c `pageprotect`**
  * **Question**: Page Table Management and Page Protection
  * **Description**: Implements system calls `int pageinfo(int va)` (displays physical frame number and P/W/U flags) and `int setreadonly(int va)` (removes `PTE_W` write permission from a page to trigger a page fault when written to).
  * **Test Program**: `user/pagetest.c`

* **10d `pagereplace`**
  * **Question**: Page Replacement Algorithms (FIFO, LRU, and Optimal)
  * **Description**: Simulates and compares three page replacement algorithms: FIFO, LRU, and Belady's Optimal (OPT) on reference string `7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1` across 3, 4, and 5 frames, generating a complete comparison table.
  * **Test Program**: `user/pagereplace.c`
