================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 6 - QUESTION 2
Deadlock Detection using Resource Allocation Graph (RAG) & Wait-For Graph (WFG)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 6, Question 2 (6b)
- Topic          : Deadlock Detection & Graph Theory
- Test Program   : user/deadlockdetect.c
- Shell Command  : $ deadlockdetect

Objective:
Implement a graph-based deadlock detection algorithm in xv6 (user/deadlockdetect.c):
1. Input representation:
   - `Allocation[n][m]`: Tracks which resources are currently held by processes.
   - `Request[n][m]`: Tracks which resources are currently demanded by processes.
2. Wait-For Graph (WFG) Conversion:
   - For single-unit resources, collapse the bipartite Resource Allocation Graph (RAG)
     into a directed Wait-For Graph where nodes are strictly processes.
   - A directed edge $P_i \to P_j$ exists if $P_i$ is waiting for a resource currently
     held by $P_j$.
3. Cycle Detection via 3-Color Depth-First Search (DFS):
   - Traverse the WFG. If a back-edge to an ancestor node is found, detect the cycle.
   - Reconstruct and output the exact cycle path (e.g. $P_0 \to P_1 \to P_2 \to P_3 \to P_0$).


2. THEORETICAL BACKGROUND & GRAPH THEORY
--------------------------------------------------------------------------------
1. RAG vs. WFG:
   - Resource Allocation Graph (RAG): A directed bipartite graph with two types of
     nodes: Processes ($P$) and Resources ($R$).
     * Request edge: $P_i \to R_j$ (process requests resource).
     * Assignment edge: $R_j \to P_i$ (resource is allocated to process).
   - Wait-For Graph (WFG): Obtained by removing resource nodes and compressing edges.
     If $P_i \to R_k$ and $R_k \to P_j$, replace with $P_i \to P_j$.

2. Single-Instance vs. Multiple-Instance Theorems:
   - Single-Unit Resource Theorem: If every resource type has exactly one instance,
     a cycle in the Wait-For Graph is a **NECESSARY AND SUFFICIENT** condition for deadlock.
   - Multiple-Unit Resource Theorem: If resources have multiple instances, a cycle is
     a necessary condition, but NOT sufficient (a process outside the cycle might
     finish and release resources, breaking the block).

3. 3-Color DFS Cycle Detection Algorithm:
   - Each node is marked with one of three colors:
     * `COLOR_WHITE` (0): Unvisited node.
     * `COLOR_GRAY`  (1): Node currently on the active DFS recursion stack.
     * `COLOR_BLACK` (2): Node and all its descendants have been fully explored.
   - Invariant: A cycle exists if and only if DFS encounters a directed edge pointing
     to a `COLOR_GRAY` node (a back-edge in the DFS tree).
   - Cycle Path Reconstruction: By maintaining a `parent_node[]` array, we trace
     backwards from the current node to the ancestor where the cycle originated.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
user/deadlockdetect.c | Created  | Constructs WFG from Allocation and Request matrices,
                      |          | runs 3-color DFS cycle detection, prints matrices,
                      |          | and reconstructs deadlock cycle chains.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_deadlockdetect.


4. DETAILED CODE WALKTHROUGH (user/deadlockdetect.c)
--------------------------------------------------------------------------------
a) WFG Construction:
   ```c
   int wfg[MAX_PROC][MAX_PROC];
   // Build directed edge Pi -> Pj if Pi requests a resource Rk held by Pj
   for (int i = 0; i < n; i++) {
     for (int j = 0; j < n; j++) {
       if (i == j) continue;
       for (int k = 0; k < m; k++) {
         if (req[i][k] > 0 && alloc[j][k] > 0) {
           wfg[i][j] = 1;
           printf("  Edge: P%d -> P%d (P%d waits for resource held by P%d)\n", i, j, i, j);
         }
       }
     }
   }
   ```

b) DFS Cycle Detection:
   ```c
   int dfs(int u, int n, int wfg[MAX_PROC][MAX_PROC]) {
     color[u] = COLOR_GRAY; // Mark as actively being visited
     for (int v = 0; v < n; v++) {
       if (wfg[u][v]) {
         if (color[v] == COLOR_GRAY) { // Found back-edge!
           cycle_start = v;
           cycle_end = u;
           return 1; // Cycle detected
         }
         if (color[v] == COLOR_WHITE) {
           parent_node[v] = u;
           if (dfs(v, n, wfg)) return 1;
         }
       }
     }
     color[u] = COLOR_BLACK; // Fully explored
     return 0;
   }
   ```


5. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/deadlockdetect

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ deadlockdetect

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


6. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
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

Analysis:
- Scenario 1 formed a directed acyclic tree converging on P2. Because P2 holds R2
  without requesting anything, P2 terminates, releasing R2 so P1 and P3 can finish.
- Scenario 2 formed a closed directed loop $P0 \to P1 \to P2 \to P3 \to P0$.
  The DFS algorithm identified the back-edge, reconstructed the cycle path, and
  flagged the deadlock.


7. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why can a Wait-For Graph only be used when resources have single instances?
A1: In a WFG, an edge $P_i \to P_j$ implies $P_i$ is waiting specifically for $P_j$
    to release a resource. If multiple instances of a resource exist, $P_i$ might be
    waiting for ANY process holding that resource type, so the dependency cannot be
    represented as a simple single edge between two processes. For multi-instance
    resources, matrix reduction (Banker's detection algorithm) must be used.

Q2: What is the time complexity of cycle detection using DFS on a WFG?
A2: $O(V + E) = O(N + N^2) = O(N^2)$, where $N$ is the number of processes (vertices)
    and $E \le N^2$ is the number of wait-for edges.

Q3: What are the three node colors in DFS and what does each signify?
A3: - White: Unvisited.
    - Gray: Currently on the recursion stack (visited, but exploration of its
      descendants is incomplete).
    - Black: Completely explored (all descendants visited, no cycles found in subtree).

Q4: If a deadlock is detected in a live operating system, how can it be recovered?
A4: 1. Process Termination: Abort all deadlocked processes, or abort one at a time
       until the deadlock cycle is broken.
    2. Resource Preemption: Successively preempt resources from deadlocked processes
       and roll them back to a checkpointed safe state until the cycle is resolved.
================================================================================
