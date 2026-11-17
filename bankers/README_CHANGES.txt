================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 6 - QUESTION 1
Banker's Algorithm Simulation for Deadlock Avoidance
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 6, Question 1 (6a)
- Topic          : Deadlock Avoidance
- Test Program   : user/bankers.c
- Shell Command  : $ bankers

Objective:
Implement Dijkstra's classic Banker's Algorithm for deadlock avoidance as a user-level
program in xv6:
1. Maintain vector and matrix data structures for $N = 5$ processes and $M = 3$
   resource types ($A, B, C$): `Available`, `Max`, `Allocation`, and `Need`.
2. Implement the Safety Algorithm (`is_safe()`) to determine whether the system is
   in a safe state and compute a valid safe execution sequence.
3. Implement the Resource-Request Algorithm (`request_resources()`) to dynamically
   evaluate hypothetical allocation requests:
   - Grant requests that leave the system in a safe state.
   - Reject/roll back requests that would lead to an unsafe state.
   - Delay requests that exceed currently available resources.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Safe State vs. Unsafe State vs. Deadlock:
   - Safe State: A state is safe if there exists at least one safe execution sequence
     $\langle P_0, P_1, \dots, P_{n-1} \rangle$ such that for each $P_i$, the resources
     that $P_i$ can still request can be satisfied by the currently available resources
     plus the resources already held by all preceding processes $P_j$ ($j < i$).
   - Unsafe State: An unsafe state is NOT necessarily a deadlock, but it carries the
     possibility of deadlock if processes request their maximum claims.
   - Deadlock State: A state where processes are actually in circular wait and cannot proceed.
   - Invariant:
       $$Deadlocked \subset Unsafe$$
       $$Safe \cap Deadlocked = \emptyset$$

2. Banker's Safety Algorithm Steps:
   Let $Work$ be a vector of length $M$ and $Finish$ be a boolean vector of length $N$:
   1. Initialize: $Work = Available$, and $Finish[i] = false$ for $i = 0, 1, \dots, N-1$.
   2. Find an index $i$ such that:
      a) $Finish[i] == false$
      b) $Need[i] \le Work$
      If no such $i$ exists, go to step 4.
   3. $Work = Work + Allocation[i]$, $Finish[i] = true$, append $P_i$ to Safe Sequence.
      Go back to step 2.
   4. If $Finish[i] == true$ for all $i$, the system is in a **SAFE** state. Otherwise, **UNSAFE**.

3. Resource-Request Algorithm Steps for Process $P_i$ requesting $Request_i$:
   1. If $Request_i \le Need_i$, proceed to step 2; else error (exceeded maximum claim).
   2. If $Request_i \le Available$, proceed to step 3; else $P_i$ must wait (insufficient resources).
   3. Pretend to allocate requested resources:
      $$Available = Available - Request_i$$
      $$Allocation_i = Allocation_i + Request_i$$
      $$Need_i = Need_i - Request_i$$
   4. Run `is_safe()`:
      - If Safe $\to$ grant request (commit new state).
      - If Unsafe $\to$ deny request and roll back (restore old $Available, Allocation, Need$).


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
user/bankers.c        | Created  | Implements Banker's safety and request algorithms
                      |          | with 4 test scenarios verifying safety, grant,
                      |          | delay, and unsafe rejection rollback.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_bankers.


4. DETAILED CODE WALKTHROUGH (user/bankers.c)
--------------------------------------------------------------------------------
a) Matrix Definitions:
   - 5 processes ($P0..P4$), 3 resource types ($A, B, C$).
   - `Allocation`: Initial matrix tracking instances currently held.
   - `Max`: Initial matrix declaring maximum demand.
   - `Available`: Initial vector `[3, 3, 2]`.
   - `Need[i][j] = Max[i][j] - Allocation[i][j]`.

b) Test Scenarios:
   - Scenario 1: Initial state check $\to$ Confirms safe sequence $\langle P1, P3, P4, P0, P2 \rangle$.
   - Scenario 2: $P1$ requests `[1, 0, 2]` $\to$ Simulated state is SAFE $\to$ GRANTED.
   - Scenario 3: $P4$ requests `[3, 3, 0]` $\to$ Exceeds Available `[2, 3, 0]` $\to$ WAITING.
   - Scenario 4: $P0$ requests `[0, 2, 0]` $\to$ Simulated state is UNSAFE $\to$ DENIED & ROLLED BACK.


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/bankers

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ bankers

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
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

Analysis:
- Initial state was verified safe with sequence <P1, P3, P4, P0, P2>.
- P1 request [1,0,2] reduced Available to [2,3,0] while remaining safe.
- P4 request [3,3,0] exceeded Available (3 > 2 for resource A) $\to$ forced to wait.
- P0 request [0,2,0] would leave Available as [2,1,0], making it impossible for
  any process to satisfy its Need, so it was properly rejected and rolled back.


7. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why is an unsafe state not necessarily a deadlocked state?
A1: An unsafe state means that if every process simultaneously requests its maximum
    declared claim, the system will deadlock. However, if processes finish and release
    resources before requesting their maximum, the system may run to completion
    without ever experiencing a deadlock. Avoidance algorithms conservatively prevent
    entering unsafe states.

Q2: What is the time complexity of the Banker's safety algorithm?
A2: $O(M \cdot N^2)$, where $N$ is the number of processes and $M$ is the number of
    resource types. In the worst case, $N$ processes are checked in each of the
    $N$ rounds, and each check compares $M$ resource vectors.

Q3: Why is the Banker's algorithm rarely used in modern general-purpose operating systems?
A3: Because processes rarely know their maximum resource requirements in advance,
    the number of processes and resources dynamically changes, and the $O(M \cdot N^2)$
    safety check on every allocation incurs high runtime overhead.

Q4: What data structure indicates how many more resources process $P_i$ may request?
A4: The `Need` matrix, defined as $Need[i][j] = Max[i][j] - Allocation[i][j]$.
================================================================================
