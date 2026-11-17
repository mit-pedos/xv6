================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 9 - QUESTION 3
Page Replacement Algorithms: FIFO and LRU (pagerepl)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 9, Question 3 (9c) [Also referenced as Lab 10d]
- Topic          : Virtual Memory & Page Replacement Policies
- Test Program   : user/pagerepl.c
- Shell Command  : $ pagerepl

Objective:
Implement a complete simulation in xv6 comparing the two fundamental page
replacement policies:
1. First-In, First-Out (FIFO)
2. Least Recently Used (LRU)

The simulation tests the standard 20-reference page request string:
    7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
across physical frame capacities of 3, 4, and 5 frames, calculating page faults,
hits, and hit ratios, and proving that LRU consistently outperforms FIFO by
exploiting temporal locality.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Why Page Replacement is Needed:
   - When demand paging is used, a process may reference a page that is not in
     memory. The kernel issues a page fault and attempts to allocate a physical frame.
   - If physical memory is completely exhausted (all frames occupied), the OS must
     evict an existing resident page to swap space to make room for the newly demanded page.

2. First-In, First-Out (FIFO):
   - Evicts the page that has resided in memory the longest (oldest arrival timestamp).
   - Suffers from **Belady's Anomaly**: increasing the number of allocated physical
     frames can paradoxically *increase* the number of page faults.

3. Least Recently Used (LRU):
   - Exploits the Principle of Temporal Locality: pages referenced in the immediate
     past are highly likely to be referenced again soon.
   - Evicts the page that has not been accessed for the longest period of time.
   - Belongs to the class of **Stack Algorithms**: the set of pages in an $n$-frame
     memory is always a subset of the pages in an $(n+1)$-frame memory.
   - Therefore, LRU is mathematically immune to Belady's Anomaly.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
user/sh.c             | Modified | Increased MAXARGS to 32 to accept custom
                      |          | 20-page command-line reference strings.
user/pagerepl.c       | Created  | Complete simulation program implementing FIFO,
                      |          | LRU, step-by-step frame dump, and comparison tables.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_pagerepl.


4. DETAILED CODE WALKTHROUGH (user/pagerepl.c)
--------------------------------------------------------------------------------
a) FIFO Implementation:
   - Maintains a circular replacement pointer `fifo_ptr = 0`.
   - On page fault when all frames are full:
     ```c
     int victim = fifo_ptr;
     frames[victim] = page;
     fifo_ptr = (fifo_ptr + 1) % num_frames;
     ```

b) LRU Implementation:
   - Maintains an access timestamp array `last_used[num_frames]`.
   - On every access (both hits and faults):
     `last_used[frame_index] = current_step;`
   - On page fault when all frames are full:
     Scans `last_used[]` to find the frame with the minimum timestamp:
     ```c
     int lru_frame = 0;
     for (int f = 1; f < num_frames; f++) {
       if (last_used[f] < last_used[lru_frame])
         lru_frame = f;
     }
     frames[lru_frame] = page;
     last_used[lru_frame] = current_step;
     ```


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/pagerepl

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ pagerepl
        (or pass custom frames and pages: $ pagerepl 3 7 0 1 2 0 3 0 4)

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ pagerepl
============================================================
       Page Replacement Simulation: FIFO vs LRU             
============================================================

------------------------------------------------------------
 FIFO Simulation (Frames: 3)
------------------------------------------------------------
  Step	|  Page	|  Frame Contents	| Status
------------------------------------------------------------
  1	|   7	| [ 7  -  - ]	| FAULT
  2	|   0	| [ 7  0  - ]	| FAULT
  3	|   1	| [ 7  0  1 ]	| FAULT
  4	|   2	| [ 2  0  1 ]	| FAULT
  5	|   0	| [ 2  0  1 ]	| HIT 
  6	|   3	| [ 2  3  1 ]	| FAULT
  7	|   0	| [ 2  3  0 ]	| FAULT
  8	|   4	| [ 4  3  0 ]	| FAULT
  9	|   2	| [ 4  2  0 ]	| FAULT
  10	|   3	| [ 4  2  3 ]	| FAULT
  11	|   0	| [ 0  2  3 ]	| FAULT
  12	|   3	| [ 0  2  3 ]	| HIT 
  13	|   2	| [ 0  2  3 ]	| HIT 
  14	|   1	| [ 0  1  3 ]	| FAULT
  15	|   2	| [ 0  1  2 ]	| FAULT
  16	|   0	| [ 0  1  2 ]	| HIT 
  17	|   1	| [ 0  1  2 ]	| HIT 
  18	|   7	| [ 7  1  2 ]	| FAULT
  19	|   0	| [ 7  0  2 ]	| FAULT
  20	|   1	| [ 7  0  1 ]	| FAULT
------------------------------------------------------------
FIFO Result (3 frames): Faults = 15, Hits = 5, Hit Ratio = 25.00%

------------------------------------------------------------
 LRU Simulation (Frames: 3)
------------------------------------------------------------
  Step	|  Page	|  Frame Contents	| Status
------------------------------------------------------------
  1	|   7	| [ 7  -  - ]	| FAULT
  2	|   0	| [ 7  0  - ]	| FAULT
  3	|   1	| [ 7  0  1 ]	| FAULT
  4	|   2	| [ 2  0  1 ]	| FAULT
  5	|   0	| [ 2  0  1 ]	| HIT 
  6	|   3	| [ 2  0  3 ]	| FAULT
  7	|   0	| [ 2  0  3 ]	| HIT 
  8	|   4	| [ 4  0  3 ]	| FAULT
  9	|   2	| [ 4  0  2 ]	| FAULT
  10	|   3	| [ 4  3  2 ]	| FAULT
  11	|   0	| [ 0  3  2 ]	| FAULT
  12	|   3	| [ 0  3  2 ]	| HIT 
  13	|   2	| [ 0  3  2 ]	| HIT 
  14	|   1	| [ 1  3  2 ]	| FAULT
  15	|   2	| [ 1  3  2 ]	| HIT 
  16	|   0	| [ 1  0  2 ]	| FAULT
  17	|   1	| [ 1  0  2 ]	| HIT 
  18	|   7	| [ 1  0  7 ]	| FAULT
  19	|   0	| [ 1  0  7 ]	| HIT 
  20	|   1	| [ 1  0  7 ]	| HIT 
------------------------------------------------------------
LRU Result (3 frames): Faults = 12, Hits = 8, Hit Ratio = 40.00%

============================================================
TASK C: Performance Comparison Across 3, 4, and 5 Frames
============================================================

+-----------+--------+-------------+-----------+-----------+
| Algorithm | Frames | Page Faults | Page Hits | Hit Ratio |
+-----------+--------+-------------+-----------+-----------+
| FIFO      |   3    |     15      |     5     |   25.00%  |
| LRU       |   3    |     12      |     8     |   40.00%  |
+-----------+--------+-------------+-----------+-----------+
| FIFO      |   4    |     10      |    10     |   50.00%  |
| LRU       |   4    |      8      |    12     |   60.00%  |
+-----------+--------+-------------+-----------+-----------+
| FIFO      |   5    |      9      |    11     |   55.00%  |
| LRU       |   5    |      7      |    13     |   65.00%  |
+-----------+--------+-------------+-----------+-----------+

============================================================
pagerepl: Simulation finished successfully!
============================================================

Analysis:
- For 3 frames: LRU yielded 12 faults (40.00% hit ratio) compared to FIFO's 15 faults (25.00%).
- For 4 frames: LRU yielded 8 faults vs FIFO's 10 faults.
- For 5 frames: LRU yielded 7 faults vs FIFO's 9 faults.
- Across every configuration, LRU strictly outperformed FIFO.


7. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: What is Belady's Anomaly and which page replacement algorithm exhibits it?
A1: Belady's Anomaly is the phenomenon where giving more physical frames to a process
    results in an increase in page faults rather than a decrease. FIFO exhibits
    Belady's Anomaly because it replaces the oldest page without regard to how frequently
    or recently it is accessed.

Q2: Why is LRU immune to Belady's Anomaly?
A2: LRU is a "stack algorithm". A page replacement algorithm is a stack algorithm if
    the set of pages resident in $N$ frames of memory is always a strict subset of
    the pages that would be resident in $N+1$ frames of memory. Stack algorithms can
    never suffer from Belady's Anomaly.

Q3: Why is true LRU difficult to implement in hardware?
A3: True LRU requires either updating a high-precision hardware clock counter on
    EVERY memory reference (read or write) and storing it in the PTE, or maintaining
    a doubly linked hardware stack of page references, both of which incur substantial
    hardware and performance overhead. Operating systems typically approximate LRU
    using the Second-Chance (Clock) algorithm with a reference bit (`PTE_A`).
================================================================================
