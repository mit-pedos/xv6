================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 10 - QUESTION 4
Page Replacement Algorithms: FIFO, LRU, and Optimal (pagereplace)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 10, Question 4 (10d) [Also referenced as Lab 9c]
- Topic          : Virtual Memory, Demand Paging & Page Replacement Optimization
- Test Program   : user/pagereplace.c
- Shell Command  : $ pagereplace

Objective:
Implement a comprehensive simulation in xv6 comparing the three fundamental page
replacement policies:
1. First-In, First-Out (FIFO)
2. Least Recently Used (LRU)
3. Optimal Page Replacement (OPT / Belady's Min Algorithm)

Evaluate performance over the standard 20-reference page request string:
    7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
across physical frame allocations of 3, 4, and 5 frames:
- Output full step-by-step frame state traces for 3 frames.
- Generate multi-frame comparison tables showing Page Faults, Hits, Hit Ratio,
  and Fault Ratio.
- Provide theoretical analysis explaining why OPT provides the mathematical lower
  bound on page faults, and why LRU approximates OPT significantly better than FIFO.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. First-In, First-Out (FIFO):
   - Evicts the page that has been in physical memory for the longest time (earliest arrival).
   - Suffers from Belady's Anomaly (more frames can produce more faults).
   - Ignores frequency and recency, often evicting active heavily used pages.

2. Least Recently Used (LRU):
   - Approximates future reference behavior using past reference history.
   - Evicts the page that has not been accessed for the longest period of time.
   - Belongs to the class of Stack Algorithms (immune to Belady's Anomaly).

3. Optimal Page Replacement (OPT / Belady's Algorithm):
   - An offline / clairvoyant algorithm: inspects future requests from $t+1$ to $N-1$.
   - Evicts the resident page that will NOT be used for the longest period of time
     in the future (or never referenced again).
   - Provably guarantees the absolute minimum number of page faults for any fixed
     number of frames.
   - Cannot be implemented in a general-purpose OS because the OS cannot foresee future
     memory references. It serves as the gold-standard benchmark against which online
     algorithms are measured.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
user/pagereplace.c    | Created  | Simulation harness implementing FIFO, LRU, OPT,
                      |          | step-by-step state tracing, and multi-frame tables.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_pagereplace.


4. DETAILED CODE WALKTHROUGH (user/pagereplace.c)
--------------------------------------------------------------------------------
a) Optimal (OPT) Algorithm Logic:
   ```c
   // On page fault when all frames are full:
   int opt_victim = -1;
   int farthest_next_use = -1;

   for (int f = 0; f < num_frames; f++) {
     int next_use = -1;
     for (int j = step + 1; j < num_refs; j++) {
       if (ref_string[j] == frames[f]) {
         next_use = j;
         break;
       }
     }
     // If page is never used again in the future, evict immediately
     if (next_use == -1) {
       opt_victim = f;
       break;
     }
     if (next_use > farthest_next_use) {
       farthest_next_use = next_use;
       opt_victim = f;
     }
   }
   frames[opt_victim] = ref_string[step];
   ```

b) LRU and FIFO Logics:
   - FIFO advances a circular modulo index `(fifo_ptr + 1) % num_frames`.
   - LRU updates `last_used[frame] = step` on every reference and evicts the
     minimum timestamp.


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/pagereplace

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ pagereplace

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ pagereplace
======================================================================
  PAGE REPLACEMENT ALGORITHMS SIMULATION: FIFO, LRU, AND OPTIMAL     
======================================================================
Reference String (20 references):
  7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1

======================================================================
PART 1: Step-by-Step Simulation Traces for 3 Frames
======================================================================

------------------------------------------------------------
 OPTIMAL (OPT) Simulation (Frames: 3)
------------------------------------------------------------
 Step	| Page	| Frame Contents| Status
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
| FIFO      |   3    |     15      |    5      |  25.00%   |   75.00%   |
| LRU       |   3    |     12      |    8      |  40.00%   |   60.00%   |
| OPT       |   3    |      9      |   11      |  55.00%   |   45.00%   |
+-----------+--------+-------------+-----------+-----------+------------+
| FIFO      |   4    |     10      |   10      |  50.00%   |   50.00%   |
| LRU       |   4    |      8      |   12      |  60.00%   |   40.00%   |
| OPT       |   4    |      8      |   12      |  60.00%   |   40.00%   |
+-----------+--------+-------------+-----------+-----------+------------+
| FIFO      |   5    |      9      |   11      |  55.00%   |   45.00%   |
| LRU       |   5    |      7      |   13      |  65.00%   |   35.00%   |
| OPT       |   5    |      7      |   13      |  65.00%   |   35.00%   |
+-----------+--------+-------------+-----------+-----------+------------+

======================================================================
PART 3: Algorithm Performance Analysis and Conclusion
======================================================================
1. Optimal (OPT):
   - Achieved the lowest page faults across all frame sizes:
     9 faults (3 frames), 8 faults (4 frames), and 7 faults (5 frames).
   - Sets the theoretical upper bound for hit ratio (up to 65.00%).

2. Least Recently Used (LRU):
   - Achieved 12 faults (3 frames), 8 faults (4 frames), and 7 faults (5 frames).
   - At 4 and 5 frames, LRU matched the performance of OPT exactly!

3. First-In, First-Out (FIFO):
   - Suffered the highest page faults: 15 faults (3 frames), 10 faults (4 frames), 9 faults (5 frames).

Conclusion:
  -> Optimal (OPT) performed best overall in minimizing page faults.
  -> Among practical implementable algorithms, LRU significantly outperformed
     FIFO across all frame sizes.
======================================================================


7. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why cannot the Optimal Page Replacement algorithm be used in general-purpose OS?
A1: Because it requires future knowledge of memory references. An operating system
    cannot predict what instructions or data addresses a user program will access
    in the future. It is primarily used in post-execution analysis as a theoretical
    benchmark to evaluate the efficiency of practical online algorithms.

Q2: Why did LRU match OPT at 4 and 5 frames?
A2: Because this reference string exhibits strong temporal locality with a working
    set size of approximately 4 pages. At 4 and 5 frames, approximating future
    references using past recency was so accurate that LRU made the exact same
    eviction decisions as the omniscient OPT algorithm.

Q3: What is the relationship between Hit Ratio and Fault Ratio?
A3: $\text{Hit Ratio} + \text{Fault Ratio} = 100\%$, where:
    $$\text{Hit Ratio} = \frac{\text{Page Hits}}{\text{Total References}} \times 100\%$$
    $$\text{Fault Ratio} = \frac{\text{Page Faults}}{\text{Total References}} \times 100\%$$
================================================================================
