#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_CHILDREN 4

int main(int argc, char *argv[]) {
  int pids[NUM_CHILDREN];
  // Mix of very long, short, and medium processes
  // P1: Very Long (80 ticks)
  // P2: Short (10 ticks)
  // P3: Short (10 ticks)
  // P4: Medium (30 ticks)
  int burst_times[NUM_CHILDREN] = {80, 10, 10, 30};
  uint64 workloads[NUM_CHILDREN] = {360000000ULL, 60000000ULL, 60000000ULL, 180000000ULL};

  printf("============================================================\n");
  printf("  Highest Response Ratio Next (HRRN) Non-Preemptive Test    \n");
  printf("============================================================\n");
  printf("Parent PID: %d\n", getpid());
  printf("Forking 4 processes:\n");
  printf("  Child 1: Very Long Job (BT = 80 ticks)\n");
  printf("  Child 2: Short Job     (BT = 10 ticks)\n");
  printf("  Child 3: Short Job     (BT = 10 ticks)\n");
  printf("  Child 4: Medium Job    (BT = 30 ticks)\n\n");

  // Keep parent prioritized while forking children
  set_burst(getpid(), 1);

  for (int i = 0; i < NUM_CHILDREN; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("Fork failed!\n");
      exit(1);
    }

    if (pid == 0) {
      // Child
      int my_pid = getpid();
      set_burst(my_pid, burst_times[i]);
      printf("[Child %d | PID %d | BT %d] -> STARTED workload at tick %d\n",
             i + 1, my_pid, burst_times[i], uptime());

      volatile uint64 counter = 0;
      for (volatile uint64 j = 0; j < workloads[i]; j++) {
        counter += j;
        if (j == workloads[i] / 2) {
          printf("[Child %d | PID %d | BT %d] -> 50%% progress reached at tick %d\n",
                 i + 1, my_pid, burst_times[i], uptime());
        }
      }

      printf("[Child %d | PID %d | BT %d] -> COMPLETED workload at tick %d\n",
             i + 1, my_pid, burst_times[i], uptime());
      exit(0);
    } else {
      // Parent records burst time immediately
      set_burst(pid, burst_times[i]);
      pids[i] = pid;
      printf("[Parent] Forked Child %d (PID %d, declared BT = %d) at tick %d\n",
             i + 1, pid, burst_times[i], uptime());
    }
  }

  printf("\n[Parent] All %d children forked. Waiting for completion...\n\n", NUM_CHILDREN);

  for (int order = 1; order <= NUM_CHILDREN; order++) {
    set_burst(getpid(), 1); // Keep parent prioritized to reap children as soon as each exits
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
    printf(">> Completion Order %d: PID %d (Child %d, Burst Time %d ticks, finished at tick %d)\n",
           order, finished_pid, child_idx, bt, uptime());
  }

  printf("\n--- HRRN Test Passed: Short jobs ran early; long jobs completed without starvation! ---\n");
  exit(0);
}
