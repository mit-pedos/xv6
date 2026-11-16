#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_PROC 10
#define MAX_RES  10

#define COLOR_WHITE 0 // Unvisited
#define COLOR_GRAY  1 // Currently visiting (in DFS stack)
#define COLOR_BLACK 2 // Visited

int color[MAX_PROC];
int parent_node[MAX_PROC];
int cycle_start = -1;
int cycle_end = -1;

void print_matrices(int n, int m, int alloc[MAX_PROC][MAX_RES], int req[MAX_PROC][MAX_RES]) {
  printf("Resource Allocation Matrix:\n");
  printf("Proc | ");
  for (int j = 0; j < m; j++) printf("R%d ", j);
  printf("\n-----+");
  for (int j = 0; j < m; j++) printf("---");
  printf("\n");
  for (int i = 0; i < n; i++) {
    printf(" P%d  | ", i);
    for (int j = 0; j < m; j++) {
      printf(" %d ", alloc[i][j]);
    }
    printf("\n");
  }

  printf("\nResource Request Matrix:\n");
  printf("Proc | ");
  for (int j = 0; j < m; j++) printf("R%d ", j);
  printf("\n-----+");
  for (int j = 0; j < m; j++) printf("---");
  printf("\n");
  for (int i = 0; i < n; i++) {
    printf(" P%d  | ", i);
    for (int j = 0; j < m; j++) {
      printf(" %d ", req[i][j]);
    }
    printf("\n");
  }
}

void build_wait_for_graph(int n, int m, int alloc[MAX_PROC][MAX_RES],
                          int req[MAX_PROC][MAX_RES], int wfg[MAX_PROC][MAX_PROC]) {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      wfg[i][j] = 0;
    }
  }

  // An edge Pi -> Pj exists if Pi is waiting for a resource held by Pj
  for (int i = 0; i < n; i++) {
    for (int k = 0; k < m; k++) {
      if (req[i][k] > 0) {
        for (int j = 0; j < n; j++) {
          if (i != j && alloc[j][k] > 0) {
            wfg[i][j] = 1;
          }
        }
      }
    }
  }

  printf("\nConstructed Wait-For Graph (WFG):\n");
  int edge_count = 0;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (wfg[i][j]) {
        printf("  Edge: P%d -> P%d (P%d waits for resource held by P%d)\n",
               i, j, i, j);
        edge_count++;
      }
    }
  }
  if (edge_count == 0) {
    printf("  (No dependency edges in graph)\n");
  }
}

int dfs(int u, int n, int wfg[MAX_PROC][MAX_PROC]) {
  color[u] = COLOR_GRAY;

  for (int v = 0; v < n; v++) {
    if (wfg[u][v]) {
      if (color[v] == COLOR_GRAY) {
        // Back edge detected: cycle found!
        cycle_start = v;
        cycle_end = u;
        return 1;
      }
      if (color[v] == COLOR_WHITE) {
        parent_node[v] = u;
        if (dfs(v, n, wfg))
          return 1;
      }
    }
  }

  color[u] = COLOR_BLACK;
  return 0;
}

void detect_deadlock(int n, int wfg[MAX_PROC][MAX_PROC]) {
  for (int i = 0; i < n; i++) {
    color[i] = COLOR_WHITE;
    parent_node[i] = -1;
  }
  cycle_start = -1;
  cycle_end = -1;

  int has_cycle = 0;
  for (int i = 0; i < n; i++) {
    if (color[i] == COLOR_WHITE) {
      if (dfs(i, n, wfg)) {
        has_cycle = 1;
        break;
      }
    }
  }

  if (!has_cycle) {
    printf("RESULT: NO DEADLOCK DETECTED.\n");
    printf("The Wait-For Graph is acyclic. All processes can complete.\n");
  } else {
    printf("RESULT: DEADLOCK DETECTED!\n");
    printf("Cycle of deadlocked processes: ");

    // Reconstruct cycle: path from cycle_end back to cycle_start
    int cycle_nodes[MAX_PROC];
    int count = 0;
    int curr = cycle_end;
    cycle_nodes[count++] = curr;
    while (curr != cycle_start && curr != -1) {
      curr = parent_node[curr];
      cycle_nodes[count++] = curr;
    }

    // Print in forward order
    for (int i = count - 1; i >= 0; i--) {
      printf("P%d -> ", cycle_nodes[i]);
    }
    printf("P%d\n", cycle_start);
  }
}

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf("   Deadlock Detection using Resource Allocation Graph (RAG) \n");
  printf("============================================================\n");

  // -------------------------------------------------------------------------
  // Scenario 1: Acyclic graph - NO DEADLOCK
  // -------------------------------------------------------------------------
  printf("\n============================================================\n");
  printf("[Scenario 1] Acyclic Wait-For Graph (No Deadlock)\n");
  printf("4 Processes (P0..P3), 4 Resources (R0..R3)\n");
  printf("P0 holds R0, requests R1\n");
  printf("P1 holds R1, requests R2\n");
  printf("P2 holds R2 (no requests)\n");
  printf("P3 holds R3, requests R2\n");
  printf("============================================================\n");

  int n1 = 4, m1 = 4;
  int alloc1[MAX_PROC][MAX_RES] = {
    {1, 0, 0, 0}, // P0 holds R0
    {0, 1, 0, 0}, // P1 holds R1
    {0, 0, 1, 0}, // P2 holds R2
    {0, 0, 0, 1}  // P3 holds R3
  };
  int req1[MAX_PROC][MAX_RES] = {
    {0, 1, 0, 0}, // P0 wants R1
    {0, 0, 1, 0}, // P1 wants R2
    {0, 0, 0, 0}, // P2 wants nothing
    {0, 0, 1, 0}  // P3 wants R2
  };
  int wfg1[MAX_PROC][MAX_PROC];

  print_matrices(n1, m1, alloc1, req1);
  build_wait_for_graph(n1, m1, alloc1, req1, wfg1);
  detect_deadlock(n1, wfg1);

  // -------------------------------------------------------------------------
  // Scenario 2: Circular wait involving 4 processes - DEADLOCK DETECTED
  // -------------------------------------------------------------------------
  printf("\n============================================================\n");
  printf("[Scenario 2] Circular Wait among 4 processes (Deadlock Detected)\n");
  printf("4 Processes (P0..P3), 4 Resources (R0..R3)\n");
  printf("P0 holds R0, requests R1\n");
  printf("P1 holds R1, requests R2\n");
  printf("P2 holds R2, requests R3\n");
  printf("P3 holds R3, requests R0\n");
  printf("============================================================\n");

  int n2 = 4, m2 = 4;
  int alloc2[MAX_PROC][MAX_RES] = {
    {1, 0, 0, 0}, // P0 holds R0
    {0, 1, 0, 0}, // P1 holds R1
    {0, 0, 1, 0}, // P2 holds R2
    {0, 0, 0, 1}  // P3 holds R3
  };
  int req2[MAX_PROC][MAX_RES] = {
    {0, 1, 0, 0}, // P0 wants R1
    {0, 0, 1, 0}, // P1 wants R2
    {0, 0, 0, 1}, // P2 wants R3
    {1, 0, 0, 0}  // P3 wants R0 (completes circular wait!)
  };
  int wfg2[MAX_PROC][MAX_PROC];

  print_matrices(n2, m2, alloc2, req2);
  build_wait_for_graph(n2, m2, alloc2, req2, wfg2);
  detect_deadlock(n2, wfg2);

  printf("\n============================================================\n");
  printf("Deadlock detection analysis finished.\n");
  printf("============================================================\n");

  exit(0);
}
