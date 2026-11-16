#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define N 5 // Number of processes (P0 to P4)
#define M 3 // Number of resource types (A, B, C)

int Allocation[N][M] = {
  {0, 1, 0}, // P0
  {2, 0, 0}, // P1
  {3, 0, 2}, // P2
  {2, 1, 1}, // P3
  {0, 0, 2}  // P4
};

int Max[N][M] = {
  {7, 5, 3}, // P0
  {3, 2, 2}, // P1
  {9, 0, 2}, // P2
  {2, 2, 2}, // P3
  {4, 3, 3}  // P4
};

int Available[M] = {3, 3, 2}; // Available instances of A, B, C
int Need[N][M];

void compute_need(void) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < M; j++) {
      Need[i][j] = Max[i][j] - Allocation[i][j];
    }
  }
}

void print_system_state(void) {
  printf("\n--- Current System State ---\n");
  printf("Process | Allocation |   Max      |   Need     \n");
  printf("        | A   B   C  | A   B   C  | A   B   C  \n");
  printf("-----------------------------------------------\n");
  for (int i = 0; i < N; i++) {
    printf("  P%d    | %d   %d   %d  | %d   %d   %d  | %d   %d   %d\n",
           i,
           Allocation[i][0], Allocation[i][1], Allocation[i][2],
           Max[i][0], Max[i][1], Max[i][2],
           Need[i][0], Need[i][1], Need[i][2]);
  }
  printf("Available: A = %d, B = %d, C = %d\n\n",
         Available[0], Available[1], Available[2]);
}

// Safety Algorithm
int is_safe(int safe_seq[N]) {
  int Work[M];
  int Finish[N];

  for (int j = 0; j < M; j++) {
    Work[j] = Available[j];
  }
  for (int i = 0; i < N; i++) {
    Finish[i] = 0;
  }

  int count = 0;
  while (count < N) {
    int found = 0;
    for (int i = 0; i < N; i++) {
      if (Finish[i] == 0) {
        int can_allocate = 1;
        for (int j = 0; j < M; j++) {
          if (Need[i][j] > Work[j]) {
            can_allocate = 0;
            break;
          }
        }
        if (can_allocate) {
          for (int j = 0; j < M; j++) {
            Work[j] += Allocation[i][j];
          }
          safe_seq[count++] = i;
          Finish[i] = 1;
          found = 1;
        }
      }
    }
    if (!found) {
      // No process could be safely satisfied
      return 0; // Unsafe state
    }
  }
  return 1; // Safe state
}

// Resource Request Algorithm
int request_resources(int pid, int req[M]) {
  printf(">>> Process P%d requests resources: [A=%d, B=%d, C=%d]\n",
         pid, req[0], req[1], req[2]);

  // Step 1: Check if Request <= Need
  for (int j = 0; j < M; j++) {
    if (req[j] > Need[pid][j]) {
      printf("ERROR: Process P%d has exceeded its maximum claim (Request > Need)!\n", pid);
      return -1;
    }
  }

  // Step 2: Check if Request <= Available
  for (int j = 0; j < M; j++) {
    if (req[j] > Available[j]) {
      printf("Notice: Process P%d must wait. Not enough resources available (Request > Available).\n", pid);
      return -2;
    }
  }

  // Step 3: Simulate granting request
  for (int j = 0; j < M; j++) {
    Available[j] -= req[j];
    Allocation[pid][j] += req[j];
    Need[pid][j] -= req[j];
  }

  // Step 4: Check if system remains safe
  int safe_seq[N];
  if (is_safe(safe_seq)) {
    printf("SUCCESS: Request granted safely!\n");
    printf("Safe sequence: <");
    for (int i = 0; i < N; i++) {
      printf("P%d%s", safe_seq[i], (i == N - 1) ? ">\n" : ", ");
    }
    return 0; // Committed
  } else {
    // Rollback changes
    for (int j = 0; j < M; j++) {
      Available[j] += req[j];
      Allocation[pid][j] -= req[j];
      Need[pid][j] += req[j];
    }
    printf("DENIED: Request denied — would lead to unsafe state! Rolled back.\n");
    return -3;
  }
}

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf("      Banker's Algorithm Simulation for Deadlock Avoidance  \n");
  printf("============================================================\n");

  compute_need();
  print_system_state();

  // Test Case 1: Initial Safety Check
  printf("[Test Case 1] Checking initial system safety...\n");
  int safe_seq[N];
  if (is_safe(safe_seq)) {
    printf("System is in a SAFE state.\nSafe execution sequence: <");
    for (int i = 0; i < N; i++) {
      printf("P%d%s", safe_seq[i], (i == N - 1) ? ">\n" : ", ");
    }
  } else {
    printf("System is in an UNSAFE state!\n");
  }

  // Test Case 2: Process 1 requests [1, 0, 2] (Should be GRANTED)
  printf("\n[Test Case 2] Process P1 requesting [1, 0, 2]...\n");
  int req1[M] = {1, 0, 2};
  request_resources(1, req1);
  print_system_state();

  // Test Case 3: Process 4 requests [3, 3, 0] (Exceeds Available)
  printf("\n[Test Case 3] Process P4 requesting [3, 3, 0]...\n");
  int req2[M] = {3, 3, 0};
  request_resources(4, req2);

  // Test Case 4: Process P0 requests [0, 2, 0] (Unsafe state -> DENIED)
  printf("\n[Test Case 4] Process P0 requesting [0, 2, 0]...\n");
  int req3[M] = {0, 2, 0};
  request_resources(0, req3);
  print_system_state();

  printf("============================================================\n");
  printf("Banker's Algorithm simulation completed successfully.\n");
  printf("============================================================\n");

  exit(0);
}
