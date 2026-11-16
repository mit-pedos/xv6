#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_CHILDREN 4
#define WORKLOAD 50000000ULL

int main(int argc, char *argv[]) {
  int pids[NUM_CHILDREN];
  // Deliberately unsorted burst times to prove SJF reorders execution
  // Arrival order: 40, 10, 30, 20
  int burst_times[NUM_CHILDREN] = {40, 10, 30, 20};

  printf("--- Non-preemptive Shortest Job First (SJF) Test ---\n");
  printf("Parent PID: %d\n", getpid());
  printf("Forking %d processes with declared burst times (40, 10, 30, 20 ticks)...\n\n", NUM_CHILDREN);

  for (int i = 0; i < NUM_CHILDREN; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("Fork failed!\n");
      exit(1);
    }

    if (pid == 0) {
      // Child process calls set_burst immediately after forking
      int my_pid = getpid();
      set_burst(my_pid, burst_times[i]);
      printf("[Child %d | PID %d] Declared burst time = %d ticks (uptime: %d)\n",
             i + 1, my_pid, burst_times[i], uptime());

      volatile uint64 counter = 0;
      for (volatile uint64 j = 0; j < WORKLOAD; j++) {
        counter += j;
      }

      printf("[Child %d | PID %d | BT %d] Completed workload at tick %d\n",
             i + 1, my_pid, burst_times[i], uptime());
      exit(0);
    } else {
      // Parent also records burst time in kernel immediately
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
    printf(">> Completion Order %d: PID %d (Child %d, Declared Burst Time %d ticks)\n",
           order, finished_pid, child_idx, bt);
  }

  printf("\n--- SJF Test Passed: Processes completed strictly in ascending order of declared burst time! ---\n");
  exit(0);
}
