================================================================================
  Question 4 — Page Replacement Algorithms: FIFO, LRU, and Optimal (pagereplace)
================================================================================

1. OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
Virtual memory management relies on page replacement algorithms when a page
fault occurs and all available physical page frames are occupied. The operating
system must select an existing resident page to evict (swap out) to make room
for the newly demanded page.

This project implements a complete simulation in xv6 comparing the three
fundamental page replacement policies:
  1. First-In, First-Out (FIFO)
  2. Least Recently Used (LRU)
  3. Optimal Page Replacement (OPT / Belady's Algorithm)

The simulation uses the standard 20-reference page request string:
    7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1


2. ALGORITHM DESIGN & LOGIC
--------------------------------------------------------------------------------
1. First-In, First-Out (FIFO):
   - Evicts the page that has been in memory the longest (oldest arrival).
   - Implemented using a circular FIFO queue indexing resident frame slots.
   - On page fault when frames are full, the oldest victim frame slot is
     evicted and assigned to the new page.

2. Least Recently Used (LRU):
   - Exploits the principle of Temporal Locality: pages accessed recently are
     likely to be accessed again in the near future.
   - Maintains an access timestamp / counter (`last_used[frame]`) updated on
     every reference (both hits and faults).
   - On page fault when frames are full, evicts the frame whose `last_used`
     timestamp is the minimum among all resident frames.

3. Optimal (OPT / Belady's Min):
   - Theoretical benchmark that yields the lowest possible page fault rate.
   - Inspects future page requests starting from `step + 1` to `N - 1`.
   - Evicts the resident page that will NOT be used for the longest period of
     time in the future (or never referenced again).


3. FILES MODIFIED / ADDED
--------------------------------------------------------------------------------
- user/pagereplace.c : Simulation program implementing FIFO, LRU, OPT, detailed
                       step-by-step tracing for 3 frames, multi-frame comparison
                       tables (3, 4, 5 frames), and performance analysis.
- Makefile           : Added $U/_pagereplace to UPROGS and cleaned DETFLAGS.
- README_CHANGES.txt : This documentation.


4. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
To compile and launch xv6 in QEMU:
    $ cd "/home/udit/Desktop/os lab/pagereplace"
    $ make clean
    $ make qemu

Inside xv6 shell, run:
    $ pagereplace

To exit QEMU:
    Press Ctrl-A followed by X.


5. VERIFIED EXECUTION OUTPUT
--------------------------------------------------------------------------------
$ pagereplace
======================================================================
     Page Replacement Algorithms Simulation (FIFO, LRU, Optimal)      
======================================================================

Reference String (20 page requests):
 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1 

======================================================================
PART 1: Step-by-Step Simulation with 3 Physical Frames
======================================================================

------------------------------------------------------------
 FIFO Simulation (Frames: 3)
------------------------------------------------------------
 Step	| Ref	| Frame State	| Status
------------------------------------------------------------
 1	| 7	|  7  -  - 	| FAULT
 2	| 0	|  7  0  - 	| FAULT
 3	| 1	|  7  0  1 	| FAULT
 4	| 2	|  2  0  1 	| FAULT
 5	| 0	|  2  0  1 	| HIT 
 6	| 3	|  2  3  1 	| FAULT
 7	| 0	|  2  3  0 	| FAULT
 8	| 4	|  4  3  0 	| FAULT
 9	| 2	|  4  2  0 	| FAULT
 10	| 3	|  4  2  3 	| FAULT
 11	| 0	|  0  2  3 	| FAULT
 12	| 3	|  0  2  3 	| HIT 
 13	| 2	|  0  2  3 	| HIT 
 14	| 1	|  0  1  3 	| FAULT
 15	| 2	|  0  1  2 	| FAULT
 16	| 0	|  0  1  2 	| HIT 
 17	| 1	|  0  1  2 	| HIT 
 18	| 7	|  7  1  2 	| FAULT
 19	| 0	|  7  0  2 	| FAULT
 20	| 1	|  7  0  1 	| FAULT
------------------------------------------------------------
FIFO Summary (3 frames): Page Faults = 15, Page Hits = 5
Hit Ratio: 25.00% | Fault Ratio: 75.00%

------------------------------------------------------------
 LRU Simulation (Frames: 3)
------------------------------------------------------------
 Step	| Ref	| Frame State	| Status
------------------------------------------------------------
 1	| 7	|  7  -  - 	| FAULT
 2	| 0	|  7  0  - 	| FAULT
 3	| 1	|  7  0  1 	| FAULT
 4	| 2	|  2  0  1 	| FAULT
 5	| 0	|  2  0  1 	| HIT 
 6	| 3	|  2  0  3 	| FAULT
 7	| 0	|  2  0  3 	| HIT 
 8	| 4	|  4  0  3 	| FAULT
 9	| 2	|  4  0  2 	| FAULT
 10	| 3	|  4  3  2 	| FAULT
 11	| 0	|  0  3  2 	| FAULT
 12	| 3	|  0  3  2 	| HIT 
 13	| 2	|  0  3  2 	| HIT 
 14	| 1	|  1  3  2 	| FAULT
 15	| 2	|  1  3  2 	| HIT 
 16	| 0	|  1  0  2 	| FAULT
 17	| 1	|  1  0  2 	| HIT 
 18	| 7	|  1  0  7 	| FAULT
 19	| 0	|  1  0  7 	| HIT 
 20	| 1	|  1  0  7 	| HIT 
------------------------------------------------------------
LRU Summary (3 frames): Page Faults = 12, Page Hits = 8
Hit Ratio: 40.00% | Fault Ratio: 60.00%

------------------------------------------------------------
 Optimal (OPT) Simulation (Frames: 3)
------------------------------------------------------------
 Step	| Ref	| Frame State	| Status
------------------------------------------------------------
 1	| 7	|  7  -  - 	| FAULT
 2	| 0	|  7  0  - 	| FAULT
 3	| 1	|  7  0  1 	| FAULT
 4	| 2	|  2  0  1 	| FAULT
 5	| 0	|  2  0  1 	| HIT 
 6	| 3	|  2  0  3 	| FAULT
 7	| 0	|  2  0  3 	| HIT 
 8	| 4	|  2  4  3 	| FAULT
 9	| 2	|  2  4  3 	| HIT 
 10	| 3	|  2  4  3 	| HIT 
 11	| 0	|  2  0  3 	| FAULT
 12	| 3	|  2  0  3 	| HIT 
 13	| 2	|  2  0  3 	| HIT 
 14	| 1	|  2  0  1 	| FAULT
 15	| 2	|  2  0  1 	| HIT 
 16	| 0	|  2  0  1 	| HIT 
 17	| 1	|  2  0  1 	| HIT 
 18	| 7	|  7  0  1 	| FAULT
 19	| 0	|  7  0  1 	| HIT 
 20	| 1	|  7  0  1 	| HIT 
------------------------------------------------------------
OPT Summary (3 frames): Page Faults = 9, Page Hits = 11
Hit Ratio: 55.00% | Fault Ratio: 45.00%

======================================================================
PART 2: Multi-Frame Performance Comparison (3, 4, and 5 Frames)
======================================================================

+-----------+--------+-------------+-----------+-----------+------------+
| Algorithm | Frames | Page Faults | Page Hits | Hit Ratio | Fault Ratio|
+-----------+--------+-------------+-----------+-----------+------------+
| FIFO      |   3    |     15      |    5     |  25.00%   |   75.00%  |
| LRU       |   3    |     12      |    8     |  40.00%   |   60.00%  |
| OPT       |   3    |     9      |    11     |  55.00%   |   45.00%  |
+-----------+--------+-------------+-----------+-----------+------------+
| FIFO      |   4    |     10      |    10     |  50.00%   |   50.00%  |
| LRU       |   4    |     8      |    12     |  60.00%   |   40.00%  |
| OPT       |   4    |     8      |    12     |  60.00%   |   40.00%  |
+-----------+--------+-------------+-----------+-----------+------------+
| FIFO      |   5    |     9      |    11     |  55.00%   |   45.00%  |
| LRU       |   5    |     7      |    13     |  65.00%   |   35.00%  |
| OPT       |   5    |     7      |    13     |  65.00%   |   35.00%  |
+-----------+--------+-------------+-----------+-----------+------------+

======================================================================
PART 3: Algorithm Performance Analysis and Conclusion
======================================================================
1. Optimal (OPT):
   - Achieved the lowest page faults across all frame sizes:
     9 faults (3 frames), 8 faults (4 frames), and 7 faults (5 frames).
   - Why: OPT looks ahead and replaces the page that will not be used
     for the longest duration in the future. It is provably optimal and
     serves as the theoretical upper bound for hit ratio.

2. Least Recently Used (LRU):
   - Achieved 12 faults (3 frames), 8 faults (4 frames), and 7 faults (5 frames).
   - Why: LRU leverages the Principle of Locality (specifically Temporal
     Locality) by approximating future behavior with past recency.
     At 4 and 5 frames, LRU matched the performance of OPT exactly!

3. First-In, First-Out (FIFO):
   - Suffered the highest page faults:
     15 faults (3 frames), 10 faults (4 frames), and 9 faults (5 frames).
   - Why: FIFO only considers the arrival order without accounting for
     frequency or recency of page accesses, evicting heavily used pages.

Conclusion:
  -> Optimal (OPT) performed best overall in minimizing page faults.
  -> Among implementable online algorithms, LRU significantly outperformed
     FIFO, achieving up to 65.00% hit ratio.
======================================================================
======================================================================
