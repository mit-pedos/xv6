================================================================================
  Question 3 — Page Replacement Algorithms (FIFO and LRU / pagerepl)
================================================================================

1. OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
This implementation builds a user-space simulation program `pagerepl.c` in xv6
that simulates and compares two classic page replacement policies:
  1. First-In, First-Out (FIFO)
  2. Least Recently Used (LRU)

Features:
  - Accepts the number of frames and reference string as command-line input:
        $ pagerepl [num_frames] [page1 page2 ...]
    or runs the standard default 20-page reference sequence when invoked without
    arguments:
        $ pagerepl
  - Displays the physical frame state and whether each reference was a HIT or
    FAULT at every step.
  - Tests the reference string:
        7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1
    with 3 physical frames, verifying the exact theoretical outcomes:
      * FIFO: 15 page faults (Hit Ratio: 25.00%)
      * LRU:  12 page faults (Hit Ratio: 40.00%)
  - Evaluates performance across 3, 4, and 5 frames, printing a comparative
    summary table with fault counts and hit ratios.


2. ALGORITHM DESIGN & LOGIC
--------------------------------------------------------------------------------
1. FIFO Algorithm:
   - Tracks the arrival order of pages using a circular FIFO index queue.
   - On a page fault when frames are full, evicts the oldest arrived page
     regardless of recent access patterns.

2. LRU Algorithm:
   - Tracks the timestamp / step of the most recent access for each resident frame.
   - On a page hit, refreshes that frame's last-accessed timestamp.
   - On a page fault when frames are full, evicts the frame whose last-accessed
     timestamp is lowest (least recently used).

3. Enhanced xv6 Shell Support:
   - Updated `MAXARGS` in `user/sh.c` to 32 to support passing full reference
     strings directly via command-line arguments.
   - Enhanced `pagerepl.c` to parse both space-delimited argument lists and
     single quoted strings.


3. FILES MODIFIED / ADDED
--------------------------------------------------------------------------------
- user/pagerepl.c    : Complete simulation program with step-by-step trace and
                       multi-frame comparison tables.
- user/sh.c          : Increased MAXARGS to 32 to support longer CLI argument lists.
- Makefile           : Added $U/_pagerepl to UPROGS and cleaned DETFLAGS.
- README_CHANGES.txt : This documentation.


4. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
To compile and launch xv6 in QEMU:
    $ cd "/home/udit/Desktop/os lab/pagerepl"
    $ make clean
    $ make qemu

Inside xv6 shell:
- Run default verification test (3 frames & 3, 4, 5 frame comparison):
    $ pagerepl

- Run with custom frame size and reference string:
    $ pagerepl 3 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1
    $ pagerepl 4 1 2 3 4 1 2 5 1 2 3 4 5

To exit QEMU:
    Press Ctrl-A followed by X.


5. VERIFIED EXECUTION OUTPUT
--------------------------------------------------------------------------------
$ pagerepl
============================================================
     Page Replacement Algorithms Simulation (FIFO vs LRU)  
============================================================

Reference String (20 references):
  7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1 

============================================================
TASK B: Testing with 3 Frames (Step-by-Step Trace)
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

--- Verification Against Expected Results (3 Frames) ---
FIFO Faults: 15 (Expected: 15) -> VERIFIED
LRU  Faults: 12 (Expected: 12) -> VERIFIED

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
================================================================================
