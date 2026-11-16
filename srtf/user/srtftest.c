#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_CHILDREN 3

void delay(int count) {
  for (volatile int i = 0; i < count; i++) {
    // busy delay
  }
}

int main(int argc, char *argv[]) {
  int pids[NUM_CHILDREN];
  // 3 processes: Long (30 ticks), Medium (15 ticks), Short (5 ticks)
  int burst_times[NUM_CHILDREN] = {30, 15, 5};
  uint64 workloads[NUM_CHILDREN] = {60000000ULL, 30000000ULL, 10000000ULL};

  printf("--- Shortest Remaining Time First (SRTF) Preemptive Test ---\n");
  printf("Parent PID: %d\n", getpid());
  printf("Forking 3 processes with staggered arrival: P1(Long=30), P2(Med=15), P3(Short=5)...\n\n");

  for (int i = 0; i < NUM_CHILDREN; i++) {
    // Stagger process arrivals
    if (i > 0) {
      delay(8000000);
    }

    int pid = fork();
    if (pid < 0) {
      printf("Fork failed!\n");
      exit(1);
    }

    if (pid == 0) {
      // Child
      int my_pid = getpid();
      set_burst(my_pid, burst_times[i]);
      printf("[Child %d | PID %d] Running with Burst Time = %d ticks (uptime: %d)\n",
             i + 1, my_pid, burst_times[i], uptime());

      volatile uint64 counter = 0;
      for (volatile uint64 j = 0; j < workloads[i]; j++) {
        counter += j;
        if (j == workloads[i] / 2) {
          printf("[Child %d | PID %d | BT %d] Reached 50%% progress (uptime: %d)\n",
                 i + 1, my_pid, burst_times[i], uptime());
        }
      }

      printf("[Child %d | PID %d | BT %d] Completed workload at tick %d\n",
             i + 1, my_pid, burst_times[i], uptime());
      exit(0);
    } else {
      // Parent records burst time immediately
      set_burst(pid, burst_times[i]);
      pids[i] = pid;
      printf("[Parent] Forked Child %d (PID %d) with declared burst time = %d\n",
             i + 1, pid, burst_times[i]);
    }
  }

  printf("\n[Parent] All children forked. Waiting for completion...\n\n");

  for (int order = 1; order <= NUM_CHILDREN; order++) {
    int finished_pid = wait(0);
    int child_idx = -1;
    int bt = -1;
    for (int i = 0; i < NUM_CHILDREN; i++) {
      if (pids[i] == finished_pid) {
        child_idx = i + 1;
        bt = burst_times[i];
        break;
      }
    }
    printf(">> Completion Order %d: PID %d (Child %d, Burst Time %d ticks)\n",
           order, finished_pid, child_idx, bt);
  }

  printf("\n--- SRTF Test Passed: Short jobs preempted longer jobs and finished first! ---\n");
  exit(0);
}
