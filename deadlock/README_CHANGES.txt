================================================================================
Deadlock Detection and Banker's Deadlock Avoidance in xv6
================================================================================

1. OVERVIEW & OBJECTIVES
--------------------------------------------------------------------------------
This implementation provides full user-space deadlock detection and deadlock
avoidance algorithms within xv6-riscv:
a) Deadlock Detection Algorithm (for systems with multiple resource instances):
   Uses vector/matrix reduction to locate deadlock sets or verify that all
   processes can terminate.
b) Banker's Algorithm (Deadlock Avoidance):
   Evaluates initial safety, tests hypothetical resource allocation requests,
   grants requests that lead to safe states, and rejects/rolls back requests
   that would lead to unsafe states.
c) Resource Allocation Modeling & Pipe Deadlock Simulation:
   Models real xv6 IPC primitives (pipes) in the Allocation/Request graph and
   detects classic circular waits.

2. FILES MODIFIED & ADDED
--------------------------------------------------------------------------------
- user/deadlock.c:
  * print_state(): Formats and prints Available vectors, Allocation, and
    Request/Need matrices.
  * detect_deadlock(): Graph-reduction deadlock detection algorithm. Tracks Work
    and Finish vectors. Returns count and process IDs of all deadlocked processes.
  * is_safe_state(): Banker's safety check. Verifies whether a valid execution
    sequence exists where every process can satisfy its maximum demand and exit.
  * simulate_pipe_deadlock(): Models circular wait between two processes sharing
    two pipes (Process A holds Pipe 1 and waits on Pipe 2; Process B holds Pipe 2
    and waits on Pipe 1).
  * main(): Comprehensive test runner executing:
    - Matrix 1: Acyclic / Safe state (5 procs, 3 resources) -> No deadlock.
    - Matrix 2: Circular dependency state -> Deadlock detected, P1-P4 identified.
    - Matrix 3: Banker's avoidance initial safety check -> Safe sequence printed.
    - Scenario A: Safe request from P1 -> Granted, new safe sequence printed.
    - Scenario B: Unsafe request from P0 -> Denied, rolled back.
    - Scenario C: Out-of-bounds request -> Exceeds available, rejected.
    - Pipe Deadlock: Graph reduction confirms circular wait.
- Makefile:
  * Cleared DETFLAGS = to support directory paths containing spaces.
  * Added $U/_deadlock to UPROGS.

3. HOW TO BUILD AND RUN
--------------------------------------------------------------------------------
From this directory:
    $ make qemu

Inside xv6 shell:
    $ deadlock

To exit QEMU:
    Press Ctrl-A then x

4. VERIFIED TEST OUTPUT LOG
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

--- System Resource State ---
Proc | Allocation | Need      
     | R0 R1 R2 | R0 R1 R2 
-----+----------+----------
 P0  |  0  4  0 |  7  1  3 
 P1  |  2  0  0 |  1  2  2 
 P2  |  3  0  2 |  6  0  0 
 P3  |  2  1  1 |  0  1  1 
 P4  |  0  0  2 |  4  3  1 
Available: R0=3 R1=0 R2=2 

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
