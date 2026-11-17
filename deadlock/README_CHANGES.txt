================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 7 - QUESTION 3
Deadlock Detection, Banker's Avoidance & Pipe Deadlock Simulation
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 7, Question 3 (7c)
- Topic          : Deadlock Algorithms & Real IPC Simulation
- Test Program   : user/deadlock.c
- Shell Command  : $ deadlock

Objective:
Implement a comprehensive deadlock analysis and testing suite in xv6 (user/deadlock.c):
1. Deadlock Detection Algorithm (Matrix Reduction for Multi-Instance Resources):
   Given `Allocation` and `Request` matrices, determines whether a deadlock exists
   and isolates the exact subset of deadlocked processes.
2. Banker's Algorithm (Deadlock Avoidance):
   Evaluates initial safety, tests hypothetical allocation requests, approves safe
   requests, and denies/rolls back unsafe requests.
3. IPC Pipe Circular-Wait Simulation:
   Simulates actual deadlock between two processes competing for two xv6 IPC pipes
   (Process A holds Pipe 1, waits on Pipe 2; Process B holds Pipe 2, waits on Pipe 1),
   models the system state in matrices, and confirms the circular wait.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Multi-Instance Deadlock Detection Algorithm (Vector/Matrix Reduction):
   Let $Work$ be a vector of length $M$ initialized to $Available$.
   Let $Finish$ be a boolean vector of length $N$:
   - For all $i$: if $Allocation[i] \neq 0$, $Finish[i] = false$; else $Finish[i] = true$.
   - Find an index $i$ such that $Finish[i] == false$ and $Request[i] \le Work$.
     If found: $Work = Work + Allocation[i]$, $Finish[i] = true$, repeat.
   - If $Finish[i] == false$ for any process when no further reduction is possible,
     then Process $i$ is **DEADLOCKED**.

2. Deadlock Detection vs. Banker's Avoidance:
   - Detection examines the *current actual requests* ($Request$). Processes only
     ask for what they need right now.
   - Avoidance (Banker's) examines *maximum future claims* ($Max$ and $Need = Max - Allocation$).
     It assumes worst-case behavior where all processes may demand their max claims simultaneously.

3. IPC Pipe Circular Wait:
   xv6 pipes are unidirectional byte streams with a bounded buffer (512 bytes).
   If Process A writes to Pipe 1 and blocks reading from Pipe 2, while Process B writes
   to Pipe 2 and blocks reading from Pipe 1, neither process can execute a write to
   satisfy the other's blocking read. This forms an unresolvable circular wait.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
user/deadlock.c       | Created  | Comprehensive test suite containing graph reduction
                      |          | detection, Banker's safety check with rollback, and
                      |          | real pipe circular wait modeling.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_deadlock.


4. DETAILED CODE WALKTHROUGH (user/deadlock.c)
--------------------------------------------------------------------------------
a) `detect_deadlock(n, m, alloc, req, avail, deadlocked_procs)`:
   - Implements graph reduction using `work` and `finish` arrays.
   - Identifies whether any process remains unfinished; if so, populates
     `deadlocked_procs` and returns the deadlocked count.

b) `is_safe_state(n, m, alloc, max, avail, safe_seq)`:
   - Computes `need[i][j] = max[i][j] - alloc[i][j]`.
   - Finds valid execution sequence satisfying $Need \le Work$.
   - Returns 1 if safe (with sequence), 0 if unsafe.

c) `simulate_pipe_deadlock()`:
   - Creates two pipes: `p1[2]` and `p2[2]`.
   - Models the two processes and their pipe handles in Allocation and Request matrices:
     * P0: Allocation `[1, 0]`, Request `[0, 1]`.
     * P1: Allocation `[0, 1]`, Request `[1, 0]`.
     * Available: `[0, 0]`.
   - Runs detection reduction to prove circular wait.


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/deadlock

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ deadlock

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ deadlock
============================================================
       Deadlock Detection & Avoidance Algorithms in xv6     
============================================================

============================================================
 TEST MATRIX 1: Deadlock Detection Algorithm (Acyclic / Safe)
 5 Processes (P0..P4), 3 Resource Types (R0..R2)
============================================================

--- System Resource State ---
Proc | Allocation | Request   
     | R0 R1 R2 | R0 R1 R2 
-----+----------+----------
 P0  |  0  1  0 |  0  0  0 
 P1  |  2  0  0 |  2  0  2 
 P2  |  3  0  3 |  0  0  0 
 P3  |  2  1  1 |  1  0  0 
 P4  |  0  0  2 |  0  0  2 
Available: R0=0 R1=0 R2=0 

RESULT: NO DEADLOCK DETECTED.
All processes can eventually terminate and release resources.

============================================================
 TEST MATRIX 2: Deadlock Detection Algorithm (Deadlocked State)
 P2 requests [0, 0, 1] creating circular wait
============================================================

--- System Resource State ---
Proc | Allocation | Request   
     | R0 R1 R2 | R0 R1 R2 
-----+----------+----------
 P0  |  0  1  0 |  0  0  0 
 P1  |  2  0  0 |  2  0  2 
 P2  |  3  0  3 |  0  0  1 
 P3  |  2  1  1 |  1  0  0 
 P4  |  0  0  2 |  0  0  2 
Available: R0=0 R1=0 R2=0 

RESULT: DEADLOCK DETECTED!
Deadlocked processes (4): [ P1 P2 P3 P4 ]
Processes cannot proceed due to circular resource dependencies.

============================================================
 TEST MATRIX 3: Banker's Algorithm Deadlock Avoidance
 Testing initial safety & hypothetical request leading to UNSAFE
============================================================

--- System Resource State ---
Proc | Allocation | Need      
     | R0 R1 R2 | R0 R1 R2 
-----+----------+----------
 P0  |  0  1  0 |  7  4  3 
 P1  |  2  0  0 |  1  2  2 
 P2  |  3  0  2 |  6  0  0 
 P3  |  2  1  1 |  0  1  1 
 P4  |  0  0  2 |  4  3  1 
Available: R0=3 R1=3 R2=2 

Initial State Check: System is in a SAFE state.
Safe sequence: <P1, P3, P4, P0, P2>

--- Request Scenario A ---
Hypothetical Request: Process P1 requests [R0=1, R1=0, R2=2]...
Request Evaluation: GRANTED (System remains in safe state).
New Safe Sequence: <P1, P3, P4, P0, P2>

--- Request Scenario B (Unsafe Request) ---
Hypothetical Request: Process P0 requests [R0=0, R1=3, R2=0]...
Request Evaluation: DENIED! Would lead to an UNSAFE state.
State successfully rolled back to safe configuration.

--- Request Scenario C (Exceeds Available Resources) ---
Hypothetical Request: Process P0 requests [R0=4, R1=0, R2=0]...
Error: Process must wait, since resources are not available (Requested 4 > Available 3)!
Request Evaluation: REJECTED immediately (insufficient available resources).

============================================================
 Task (c): Real Process Pipe Deadlock Simulation & Detection
 Process A holds Pipe1 (write) and waits to read Pipe2
 Process B holds Pipe2 (write) and waits to read Pipe1
============================================================

--- System Resource State ---
Proc | Allocation | Request   
     | R0 R1 | R0 R1 
-----+-------+-------
 P0  |  1  0 |  0  1 
 P1  |  0  1 |  1  0 
Available: R0=0 R1=0 

ALGORITHM RESULT: DEADLOCK DETECTED among 2 processes!
Deadlocked processes: Process A (P0), Process B (P1)
Explanation: Process A holds Pipe 1 waiting for Pipe 2;
             Process B holds Pipe 2 waiting for Pipe 1 (Circular Wait).

============================================================
All Deadlock Detection and Avoidance tests completed.
============================================================


7. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: How does Deadlock Detection differ from Deadlock Avoidance?
A1: Detection algorithms run periodically or when resource utilization drops,
    inspecting the actual outstanding `Request` matrix to identify existing deadlocks
    so recovery can be triggered. Avoidance algorithms run proactively *before* every
    allocation, inspecting maximum declared claims (`Need`) to prevent the system
    from ever entering an unsafe state.

Q2: Why was Matrix 1 not deadlocked even though Available was [0, 0, 0]?
A2: Because P0 was holding resources without requesting any additional resources
    (`Request[0] = [0, 0, 0]`). Therefore, P0 terminates, releasing its allocation
    `[0, 1, 0]`, which enables P3 to finish, which in turn frees resources for P1,
    P4, and P2.

Q3: In the Pipe Deadlock scenario, how is circular wait broken in actual software design?
A3: By establishing non-blocking I/O (e.g. `O_NONBLOCK` on file descriptors) or by
    enforcing consistent lock/channel ordering such that both processes always read
    from the lower pipe descriptor first before writing to the higher one.
================================================================================
