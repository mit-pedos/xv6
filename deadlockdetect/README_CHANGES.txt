================================================================================
Operating Systems Lab Assignment
Question 2: Deadlock Detection using Resource Allocation Graph (RAG)
================================================================================

1. OVERVIEW & DESIGN
--------------------
This program implements a deadlock detection algorithm based on the Resource
Allocation Graph (RAG) and Wait-For Graph (WFG) model in xv6 (user/deadlockdetect.c).

Key Components:
1. Matrix Representation:
   - Allocation[n][m]: Allocation matrix tracking which process holds which resource.
   - Request[n][m]: Request matrix tracking which process is currently requesting which resource.

2. Wait-For Graph (WFG) Construction:
   - Evaluates dependency relationships between processes.
   - A directed edge Pi -> Pj exists if process Pi is requesting resource Rk and
     resource Rk is currently held by process Pj (for all i != j and resources k).

3. Cycle Detection Algorithm (DFS-based):
   - Employs a 3-color Depth First Search (DFS):
     * COLOR_WHITE (0): Unvisited node.
     * COLOR_GRAY (1): Currently visiting node in DFS recursion path.
     * COLOR_BLACK (2): Completely visited node.
   - If DFS encounters a node with COLOR_GRAY, a back-edge is discovered, proving
     the existence of a cycle (deadlock).
   - Reconstructs the exact cycle using a parent pointer array (parent_node[])
     and prints the full chain of deadlocked processes (e.g. P0 -> P1 -> P2 -> P3 -> P0).

4. Test Scenarios:
   - Scenario 1 (No Deadlock):
     * 4 processes, 4 resources.
     * Edges: P0 -> P1, P1 -> P2, P3 -> P2.
     * Result: Graph is acyclic; no deadlock detected.
   - Scenario 2 (Circular Wait with 4 processes):
     * 4 processes, 4 resources.
     * P0 holds R0, requests R1 (held by P1).
     * P1 holds R1, requests R2 (held by P2).
     * P2 holds R2, requests R3 (held by P3).
     * P3 holds R3, requests R0 (held by P0).
     * Result: Cycle detected: P0 -> P1 -> P2 -> P3 -> P0.

2. FILES MODIFIED / ADDED
-------------------------
- user/deadlockdetect.c:
  * Implemented RAG matrix display, WFG graph construction, DFS cycle detection,
    cycle path reconstruction, and both test scenarios.
- Makefile:
  * Added $U/_deadlockdetect to UPROGS.
  * Cleared DETFLAGS to prevent compiler errors on paths with spaces.
  * Set CPUS := 1.

3. HOW TO BUILD AND RUN
-----------------------
Inside this directory:
    $ make clean
    $ make qemu

Once xv6 boots into the shell, run:
    $ deadlockdetect

To exit QEMU:
    Press Ctrl+A, then X.

4. SAMPLE OUTPUT VERIFICATION
-----------------------------
$ deadlockdetect
============================================================
   Deadlock Detection using Resource Allocation Graph (RAG) 
============================================================

============================================================
[Scenario 1] Acyclic Wait-For Graph (No Deadlock)
4 Processes (P0..P3), 4 Resources (R0..R3)
P0 holds R0, requests R1
P1 holds R1, requests R2
P2 holds R2 (no requests)
P3 holds R3, requests R2
============================================================
Resource Allocation Matrix:
Proc | R0 R1 R2 R3 
-----+------------
 P0  |  1  0  0  0 
 P1  |  0  1  0  0 
 P2  |  0  0  1  0 
 P3  |  0  0  0  1 

Resource Request Matrix:
Proc | R0 R1 R2 R3 
-----+------------
 P0  |  0  1  0  0 
 P1  |  0  0  1  0 
 P2  |  0  0  0  0 
 P3  |  0  0  1  0 

Constructed Wait-For Graph (WFG):
  Edge: P0 -> P1 (P0 waits for resource held by P1)
  Edge: P1 -> P2 (P1 waits for resource held by P2)
  Edge: P3 -> P2 (P3 waits for resource held by P2)
RESULT: NO DEADLOCK DETECTED.
The Wait-For Graph is acyclic. All processes can complete.

============================================================
[Scenario 2] Circular Wait among 4 processes (Deadlock Detected)
4 Processes (P0..P3), 4 Resources (R0..R3)
P0 holds R0, requests R1
P1 holds R1, requests R2
P2 holds R2, requests R3
P3 holds R3, requests R0
============================================================
Resource Allocation Matrix:
Proc | R0 R1 R2 R3 
-----+------------
 P0  |  1  0  0  0 
 P1  |  0  1  0  0 
 P2  |  0  0  1  0 
 P3  |  0  0  0  1 

Resource Request Matrix:
Proc | R0 R1 R2 R3 
-----+------------
 P0  |  0  1  0  0 
 P1  |  0  0  1  0 
 P2  |  0  0  0  1 
 P3  |  1  0  0  0 

Constructed Wait-For Graph (WFG):
  Edge: P0 -> P1 (P0 waits for resource held by P1)
  Edge: P1 -> P2 (P1 waits for resource held by P2)
  Edge: P2 -> P3 (P2 waits for resource held by P3)
  Edge: P3 -> P0 (P3 waits for resource held by P0)
RESULT: DEADLOCK DETECTED!
Cycle of deadlocked processes: P0 -> P1 -> P2 -> P3 -> P0

============================================================
Deadlock detection analysis finished.
============================================================
