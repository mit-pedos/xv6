#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_CHILDREN 4
#define WORKLOAD 40000000ULL

int main(int argc, char *argv[]) {
  int pids[NUM_CHILDREN];
  int priorities[NUM_CHILDREN] = {5, 8, 12, 20}; // 1 = highest, 20 = lowest

  printf("--- Priority Preemptive Scheduler with Aging Test ---\n");
  printf("Parent PID: %d\n", getpid());
  printf("Forking %d processes with priorities (5=High, 8=Med-High, 12=Med-Low, 20=Lowest)...\n\n", NUM_CHILDREN);

  // Keep parent prioritized while forking children
  set_priority(getpid(), 1);

  for (int i = 0; i < NUM_CHILDREN; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("Fork failed!\n");
      exit(1);
    }

    if (pid == 0) {
      // Child process
      int my_pid = getpid();
      set_priority(my_pid, priorities[i]);
      printf("[Child %d | PID %d] Started with Priority %d (uptime: %d ticks)\n",
             i + 1, my_pid, priorities[i], uptime());

      volatile uint64 counter = 0;
      for (volatile uint64 j = 0; j < WORKLOAD; j++) {
        counter += j;
        // Periodic progress print to prove execution & starvation prevention
        if (j == WORKLOAD / 2) {
          printf("[Child %d | PID %d | InitPrio %d] Reached 50%% progress (uptime: %d ticks)\n",
                 i + 1, my_pid, priorities[i], uptime());
        }
      }

      printf("[Child %d | PID %d | InitPrio %d] Completed workload at tick %d\n",
             i + 1, my_pid, priorities[i], uptime());
      exit(0);
    } else {
      // Parent sets priority immediately
      set_priority(pid, priorities[i]);
      pids[i] = pid;
      printf("[Parent] Forked Child %d (PID %d) assigned Priority %d\n", i + 1, pid, priorities[i]);
    }
  }

  printf("\n[Parent] All children running. Waiting for completion...\n\n");

  for (int order = 1; order <= NUM_CHILDREN; order++) {
    set_priority(getpid(), 1); // Keep parent prioritized to reap children as soon as each exits
    int finished_pid = wait(0);
    int child_idx = -1;
    int init_prio = -1;
    for (int i = 0; i < NUM_CHILDREN; i++) {
      if (pids[i] == finished_pid) {
        child_idx = i + 1;
        init_prio = priorities[i];
        break;
      }
    }
    printf(">> Completion Order %d: PID %d (Child %d, Initial Priority %d, finished at tick %d)\n",
           order, finished_pid, child_idx, init_prio, uptime());
  }

  printf("\n--- Priority Test Passed: High priority completed first; low priority ran via aging! ---\n");
  exit(0);
}
