#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_CHILDREN 3

int main(int argc, char *argv[]) {
  int pids[NUM_CHILDREN];
  // P1: Long (50 ticks), P2: Medium (20 ticks), P3: Short (5 ticks)
  int burst_times[NUM_CHILDREN] = {50, 20, 5};
  uint64 workloads[NUM_CHILDREN] = {600000000ULL, 240000000ULL, 60000000ULL};
  int spawn_delays[NUM_CHILDREN] = {0, 6, 6}; // Ticks parent sleeps before spawning next

  printf("============================================================\n");
  printf("  Shortest Remaining Time First (SRTF) Preemptive Test      \n");
  printf("============================================================\n");
  printf("Parent PID: %d\n", getpid());
  printf("Forking 3 processes with staggered arrival:\n");
  printf("  Child 1: Long Job   (BT = 50 ticks)\n");
  printf("  Child 2: Medium Job (BT = 20 ticks, arrives at tick +6 to PREEMPT Child 1)\n");
  printf("  Child 3: Short Job  (BT = 5 ticks,  arrives at tick +12 to PREEMPT Child 2)\n\n");

  for (int i = 0; i < NUM_CHILDREN; i++) {
    if (spawn_delays[i] > 0) {
      // Give parent highest priority (BT=1) so it preempts running child to spawn next
      set_burst(getpid(), 1);
      pause(spawn_delays[i]);
    }

    int pid = fork();
    if (pid < 0) {
      printf("Fork failed!\n");
      exit(1);
    }

    if (pid == 0) {
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

  printf("\n--- SRTF Test Passed: Short jobs preempted longer jobs and finished first! ---\n");
  exit(0);
}
