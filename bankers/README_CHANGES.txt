================================================================================
Operating Systems Lab Assignment
Question 1: Banker's Algorithm Simulation for Deadlock Avoidance
================================================================================

1. OVERVIEW & DESIGN
--------------------
This program implements the classic Banker's Algorithm for deadlock avoidance
as a user-level program in xv6 (user/bankers.c).

Key Data Structures:
- N = 5 processes (P0, P1, P2, P3, P4)
- M = 3 resource types (A, B, C)
- Allocation[N][M]: Resources currently allocated to each process.
- Max[N][M]: Maximum resource demand of each process.
- Available[M]: Currently available instances of each resource type.
- Need[N][M]: Calculated dynamically as Need[i][j] = Max[i][j] - Allocation[i][j].

Implemented Algorithms:
1. Safety Algorithm (is_safe):
   - Initializes Work = Available and Finish[i] = 0 for all i.
   - Iteratively searches for an unfinished process Pi whose Need[i] <= Work.
   - If found: assumes Pi finishes and returns its resources: Work += Allocation[i], Finish[i] = 1.
   - Adds Pi to the safe sequence array.
   - If all processes finish, returns 1 (Safe) with the safe sequence.
   - If no such process is found and some processes remain unfinished, returns 0 (Unsafe).

2. Resource Request Algorithm (request_resources):
   - Validates that Request[i] <= Need[i] (claim check).
   - Validates that Request[i] <= Available (availability check).
   - Simulates allocation:
       Available -= Request
       Allocation[i] += Request
       Need[i] -= Request
   - Invokes is_safe().
   - If safe: commits changes and outputs the resulting safe sequence.
   - If unsafe: rolls back the allocation (restores original Available, Allocation, Need)
     and prints "Request denied — would lead to unsafe state".

2. FILES MODIFIED / ADDED
-------------------------
- user/bankers.c:
  * Implemented Banker's Algorithm, Safety Algorithm, Resource Request Algorithm,
    and 4 comprehensive test scenarios.
- Makefile:
  * Added $U/_bankers to UPROGS.
  * Cleared DETFLAGS to prevent compiler errors on paths containing spaces.
  * Set CPUS := 1.

3. HOW TO BUILD AND RUN
-----------------------
Inside this directory:
    $ make clean
    $ make qemu

Once xv6 boots into the shell, run:
    $ bankers

To exit QEMU:
    Press Ctrl+A, then X.

4. SAMPLE OUTPUT VERIFICATION
-----------------------------
$ bankers
============================================================
      Banker's Algorithm Simulation for Deadlock Avoidance  
============================================================

--- Current System State ---
Process | Allocation |   Max      |   Need     
        | A   B   C  | A   B   C  | A   B   C  
-----------------------------------------------
  P0    | 0   1   0  | 7   5   3  | 7   4   3
  P1    | 2   0   0  | 3   2   2  | 1   2   2
  P2    | 3   0   2  | 9   0   2  | 6   0   0
  P3    | 2   1   1  | 2   2   2  | 0   1   1
  P4    | 0   0   2  | 4   3   3  | 4   3   1
Available: A = 3, B = 3, C = 2

[Test Case 1] Checking initial system safety...
System is in a SAFE state.
Safe execution sequence: <P1, P3, P4, P0, P2>

[Test Case 2] Process P1 requesting [1, 0, 2]...
>>> Process P1 requests resources: [A=1, B=0, C=2]
SUCCESS: Request granted safely!
Safe sequence: <P1, P3, P4, P0, P2>

--- Current System State ---
Process | Allocation |   Max      |   Need     
        | A   B   C  | A   B   C  | A   B   C  
-----------------------------------------------
  P0    | 0   1   0  | 7   5   3  | 7   4   3
  P1    | 3   0   2  | 3   2   2  | 0   2   0
  P2    | 3   0   2  | 9   0   2  | 6   0   0
  P3    | 2   1   1  | 2   2   2  | 0   1   1
  P4    | 0   0   2  | 4   3   3  | 4   3   1
Available: A = 2, B = 3, C = 0

[Test Case 3] Process P4 requesting [3, 3, 0]...
>>> Process P4 requests resources: [A=3, B=3, C=0]
Notice: Process P4 must wait. Not enough resources available (Request > Available).

[Test Case 4] Process P0 requesting [0, 2, 0]...
>>> Process P0 requests resources: [A=0, B=2, C=0]
DENIED: Request denied — would lead to unsafe state! Rolled back.

--- Current System State ---
Process | Allocation |   Max      |   Need     
        | A   B   C  | A   B   C  | A   B   C  
-----------------------------------------------
  P0    | 0   1   0  | 7   5   3  | 7   4   3
  P1    | 3   0   2  | 3   2   2  | 0   2   0
  P2    | 3   0   2  | 9   0   2  | 6   0   0
  P3    | 2   1   1  | 2   2   2  | 0   1   1
  P4    | 0   0   2  | 4   3   3  | 4   3   1
Available: A = 2, B = 3, C = 0

============================================================
Banker's Algorithm simulation completed successfully.
============================================================
