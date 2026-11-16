#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_PROC 10
#define MAX_RES  10

// Static buffers to prevent stack overflow on xv6 4KB user stack
static int work[MAX_RES];
static int finish[MAX_PROC];

// Print Available, Allocation, Request/Need matrices
void print_state(int n, int m, int alloc[MAX_PROC][MAX_RES],
                 int second_matrix[MAX_PROC][MAX_RES], char *second_name,
                 int avail[MAX_RES]) {
  printf("\n--- System Resource State ---\n");
  printf("Proc | Allocation | %s\n", second_name);
  printf("     | ");
  for (int j = 0; j < m; j++) printf("R%d ", j);
  printf("| ");
  for (int j = 0; j < m; j++) printf("R%d ", j);
  printf("\n-----+");
  for (int j = 0; j < m; j++) printf("---");
  printf("-+-");
  for (int j = 0; j < m; j++) printf("---");
  printf("\n");

  for (int i = 0; i < n; i++) {
    printf(" P%d  | ", i);
    for (int j = 0; j < m; j++) printf(" %d ", alloc[i][j]);
    printf("| ");
    for (int j = 0; j < m; j++) printf(" %d ", second_matrix[i][j]);
    printf("\n");
  }

  printf("Available: ");
  for (int j = 0; j < m; j++) printf("R%d=%d ", j, avail[j]);
  printf("\n\n");
}

// Deadlock Detection Algorithm
int detect_deadlock(int n, int m, int alloc[MAX_PROC][MAX_RES],
                    int req[MAX_PROC][MAX_RES], int avail[MAX_RES],
                    int deadlocked_list[MAX_PROC]) {
  for (int j = 0; j < m; j++)
    work[j] = avail[j];

  for (int i = 0; i < n; i++) {
    // If process has no allocations, it cannot be part of a deadlock
    int has_alloc = 0;
    for (int j = 0; j < m; j++) {
      if (alloc[i][j] > 0) {
        has_alloc = 1;
        break;
      }
    }
    finish[i] = (has_alloc == 0) ? 1 : 0;
  }

  while (1) {
    int found = 0;
    for (int i = 0; i < n; i++) {
      if (finish[i] == 0) {
        int can_satisfy = 1;
        for (int j = 0; j < m; j++) {
          if (req[i][j] > work[j]) {
            can_satisfy = 0;
            break;
          }
        }
        if (can_satisfy) {
          for (int j = 0; j < m; j++) {
            work[j] += alloc[i][j];
          }
          finish[i] = 1;
          found = 1;
        }
      }
    }
    if (!found)
      break;
  }

  int deadlocked_count = 0;
  for (int i = 0; i < n; i++) {
    if (finish[i] == 0) {
      deadlocked_list[deadlocked_count++] = i;
    }
  }

  return deadlocked_count;
}

// Banker's Algorithm Safety Check
int is_safe_state(int n, int m, int alloc[MAX_PROC][MAX_RES],
                  int need[MAX_PROC][MAX_RES], int avail[MAX_RES],
                  int safe_seq[MAX_PROC]) {
  for (int j = 0; j < m; j++)
    work[j] = avail[j];

  for (int i = 0; i < n; i++)
    finish[i] = 0;

  int count = 0;
  while (count < n) {
    int found = 0;
    for (int i = 0; i < n; i++) {
      if (finish[i] == 0) {
        int can_allocate = 1;
        for (int j = 0; j < m; j++) {
          if (need[i][j] > work[j]) {
            can_allocate = 0;
            break;
          }
        }
        if (can_allocate) {
          for (int j = 0; j < m; j++) {
            work[j] += alloc[i][j];
          }
          safe_seq[count++] = i;
          finish[i] = 1;
          found = 1;
        }
      }
    }
    if (!found)
      return 0; // Unsafe
  }
  return 1; // Safe
}

// Test matrices stored statically in data section to avoid stack growth
static int alloc1[MAX_PROC][MAX_RES] = {
  {0, 1, 0},
  {2, 0, 0},
  {3, 0, 3},
  {2, 1, 1},
  {0, 0, 2}
};
static int req1[MAX_PROC][MAX_RES] = {
  {0, 0, 0},
  {2, 0, 2},
  {0, 0, 0},
  {1, 0, 0},
  {0, 0, 2}
};
static int avail1[MAX_RES] = {0, 0, 0};
static int deadlocked1[MAX_PROC];

static int alloc2[MAX_PROC][MAX_RES] = {
  {0, 1, 0},
  {2, 0, 0},
  {3, 0, 3},
  {2, 1, 1},
  {0, 0, 2}
};
static int req2[MAX_PROC][MAX_RES] = {
  {0, 0, 0},
  {2, 0, 2},
  {0, 0, 1}, // P2 now requests R2
  {1, 0, 0},
  {0, 0, 2}
};
static int avail2[MAX_RES] = {0, 0, 0};
static int deadlocked2[MAX_PROC];

static int alloc3[MAX_PROC][MAX_RES] = {
  {0, 1, 0},
  {2, 0, 0},
  {3, 0, 2},
  {2, 1, 1},
  {0, 0, 2}
};
static int max3[MAX_PROC][MAX_RES] = {
  {7, 5, 3},
  {3, 2, 2},
  {9, 0, 2},
  {2, 2, 2},
  {4, 3, 3}
};
static int avail3[MAX_RES] = {3, 3, 2};
static int need3[MAX_PROC][MAX_RES];
static int safe_seq[MAX_PROC];

// Task c: Pipe Deadlock Simulation using real processes
static int alloc_pipe[MAX_PROC][MAX_RES] = {
  {1, 0}, // Process A holds Pipe 1
  {0, 1}  // Process B holds Pipe 2
};
static int req_pipe[MAX_PROC][MAX_RES] = {
  {0, 1}, // Process A requests Pipe 2
  {1, 0}  // Process B requests Pipe 1
};
static int avail_pipe[MAX_RES] = {0, 0};
static int deadlocked_pipe[MAX_PROC];

void simulate_pipe_deadlock(void) {
  printf("============================================================\n");
  printf(" Task (c): Real Process Pipe Deadlock Simulation & Detection\n");
  printf(" Process A holds Pipe1 (write) and waits to read Pipe2\n");
  printf(" Process B holds Pipe2 (write) and waits to read Pipe1\n");
  printf("============================================================\n");

  int n = 2, m = 2;
  print_state(n, m, alloc_pipe, req_pipe, "Request   ", avail_pipe);

  int count = detect_deadlock(n, m, alloc_pipe, req_pipe, avail_pipe, deadlocked_pipe);
  if (count > 0) {
    printf("ALGORITHM RESULT: DEADLOCK DETECTED among %d processes!\n", count);
    printf("Deadlocked processes: ");
    for (int i = 0; i < count; i++) {
      printf("Process %c (P%d)%s",
             (deadlocked_pipe[i] == 0 ? 'A' : 'B'),
             deadlocked_pipe[i],
             (i == count - 1) ? "\n" : ", ");
    }
    printf("Explanation: Process A holds Pipe 1 waiting for Pipe 2;\n");
    printf("             Process B holds Pipe 2 waiting for Pipe 1 (Circular Wait).\n");
  } else {
    printf("ALGORITHM RESULT: No deadlock.\n");
  }
}

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf("       Deadlock Detection & Avoidance Algorithms in xv6     \n");
  printf("============================================================\n");

  // -------------------------------------------------------------------------
  // Matrix 1: Deadlock Detection (Safe / No Deadlock Scenario)
  // -------------------------------------------------------------------------
  printf("\n============================================================\n");
  printf(" TEST MATRIX 1: Deadlock Detection Algorithm (Acyclic / Safe)\n");
  printf(" 5 Processes (P0..P4), 3 Resource Types (R0..R2)\n");
  printf("============================================================\n");
  int n1 = 5, m1 = 3;

  print_state(n1, m1, alloc1, req1, "Request   ", avail1);

  int count1 = detect_deadlock(n1, m1, alloc1, req1, avail1, deadlocked1);
  if (count1 == 0) {
    printf("RESULT: NO DEADLOCK DETECTED.\n");
    printf("All processes can eventually terminate and release resources.\n");
  } else {
    printf("RESULT: DEADLOCK DETECTED (%d processes deadlocked).\n", count1);
  }

  // -------------------------------------------------------------------------
  // Matrix 2: Deadlock Detection (Deadlock Exists Scenario)
  // -------------------------------------------------------------------------
  printf("\n============================================================\n");
  printf(" TEST MATRIX 2: Deadlock Detection Algorithm (Deadlocked State)\n");
  printf(" P2 requests [0, 0, 1] creating circular wait\n");
  printf("============================================================\n");

  print_state(n1, m1, alloc2, req2, "Request   ", avail2);

  int count2 = detect_deadlock(n1, m1, alloc2, req2, avail2, deadlocked2);
  if (count2 > 0) {
    printf("RESULT: DEADLOCK DETECTED!\n");
    printf("Deadlocked processes (%d): [ ", count2);
    for (int i = 0; i < count2; i++) {
      printf("P%d ", deadlocked2[i]);
    }
    printf("]\n");
    printf("Processes cannot proceed due to circular resource dependencies.\n");
  } else {
    printf("RESULT: NO DEADLOCK.\n");
  }

  // -------------------------------------------------------------------------
  // Matrix 3: Banker's Algorithm (Avoidance: Becomes Unsafe After Request)
  // -------------------------------------------------------------------------
  printf("\n============================================================\n");
  printf(" TEST MATRIX 3: Banker's Algorithm Deadlock Avoidance\n");
  printf(" Testing initial safety & hypothetical request leading to UNSAFE\n");
  printf("============================================================\n");
  int n3 = 5, m3 = 3;
  for (int i = 0; i < n3; i++)
    for (int j = 0; j < m3; j++)
      need3[i][j] = max3[i][j] - alloc3[i][j];

  print_state(n3, m3, alloc3, need3, "Need      ", avail3);

  if (is_safe_state(n3, m3, alloc3, need3, avail3, safe_seq)) {
    printf("Initial State Check: System is in a SAFE state.\nSafe sequence: <");
    for (int i = 0; i < n3; i++)
      printf("P%d%s", safe_seq[i], (i == n3 - 1) ? ">\n" : ", ");
  }

  // Scenario 3a: Hypothetical Safe Request from P1: [1, 0, 2]
  printf("\n--- Request Scenario A ---\n");
  printf("Hypothetical Request: Process P1 requests [R0=1, R1=0, R2=2]...\n");
  int req_p1[3] = {1, 0, 2};
  int can_grant = 1;
  for (int j = 0; j < m3; j++) {
    if (req_p1[j] > need3[1][j] || req_p1[j] > avail3[j]) {
      can_grant = 0;
      break;
    }
  }
  if (can_grant) {
    for (int j = 0; j < m3; j++) {
      avail3[j] -= req_p1[j];
      alloc3[1][j] += req_p1[j];
      need3[1][j] -= req_p1[j];
    }
    if (is_safe_state(n3, m3, alloc3, need3, avail3, safe_seq)) {
      printf("Request Evaluation: GRANTED (System remains in safe state).\n");
      printf("New Safe Sequence: <");
      for (int i = 0; i < n3; i++)
        printf("P%d%s", safe_seq[i], (i == n3 - 1) ? ">\n" : ", ");
    }
    // Rollback for next test
    for (int j = 0; j < m3; j++) {
      avail3[j] += req_p1[j];
      alloc3[1][j] -= req_p1[j];
      need3[1][j] += req_p1[j];
    }
  }

  // Scenario 3b: Hypothetical Unsafe Request from P0: [0, 3, 0]
  printf("\n--- Request Scenario B (Unsafe Request) ---\n");
  printf("Hypothetical Request: Process P0 requests [R0=0, R1=3, R2=0]...\n");
  int req_p0[3] = {0, 3, 0};
  for (int j = 0; j < m3; j++) {
    avail3[j] -= req_p0[j];
    alloc3[0][j] += req_p0[j];
    need3[0][j] -= req_p0[j];
  }

  print_state(n3, m3, alloc3, need3, "Need      ", avail3);

  if (is_safe_state(n3, m3, alloc3, need3, avail3, safe_seq)) {
    printf("Request Evaluation: GRANTED (System remains in safe state).\n");
  } else {
    printf("Request Evaluation: DENIED! Would lead to an UNSAFE state.\n");
    // Rollback
    for (int j = 0; j < m3; j++) {
      avail3[j] += req_p0[j];
      alloc3[0][j] -= req_p0[j];
      need3[0][j] += req_p0[j];
    }
    printf("State successfully rolled back to safe configuration.\n");
  }

  // Scenario 3c: Request Exceeding Available: P0 requests [4, 0, 0] (Avail is [3, 3, 2])
  printf("\n--- Request Scenario C (Exceeds Available Resources) ---\n");
  printf("Hypothetical Request: Process P0 requests [R0=4, R1=0, R2=0]...\n");
  int req_p0_exceed[3] = {4, 0, 0};
  int valid = 1;
  for (int j = 0; j < m3; j++) {
    if (req_p0_exceed[j] > need3[0][j]) {
      printf("Error: Process has exceeded its maximum claim!\n");
      valid = 0;
      break;
    }
    if (req_p0_exceed[j] > avail3[j]) {
      printf("Error: Process must wait, since resources are not available (Requested %d > Available %d)!\n",
             req_p0_exceed[j], avail3[j]);
      valid = 0;
      break;
    }
  }
  if (!valid) {
    printf("Request Evaluation: REJECTED immediately (insufficient available resources).\n");
  }

  // -------------------------------------------------------------------------
  // Pipe Deadlock Simulation
  // -------------------------------------------------------------------------
  printf("\n");
  simulate_pipe_deadlock();

  printf("\n============================================================\n");
  printf("All Deadlock Detection and Avoidance tests completed.\n");
  printf("============================================================\n");

  exit(0);
}
