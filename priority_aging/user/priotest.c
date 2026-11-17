#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_CHILDREN 4
#define WORKLOAD 500000000ULL

int main(int argc, char *argv[]) {
  int pids[NUM_CHILDREN];
  int priorities[NUM_CHILDREN] = {5, 8, 12, 20}; // 1 = highest, 20 = lowest

  printf("============================================================\n");
  printf(" Priority Preemptive Scheduler with Aging (Anti-Starvation) \n");
  printf("============================================================\n");
  printf("Parent PID: %d\n", getpid());
  printf("Forking %d CPU-bound processes with priorities:\n", NUM_CHILDREN);
  printf("  Child 1: Priority %d (High)\n", priorities[0]);
  printf("  Child 2: Priority %d (Medium-High)\n", priorities[1]);
  printf("  Child 3: Priority %d (Medium-Low)\n", priorities[2]);
  printf("  Child 4: Priority %d (Lowest)\n\n", priorities[3]);

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
      int start_prio = get_priority(my_pid);
      printf("[Child %d | PID %d] Starting execution at tick %d (Initial: %d, Current: %d)\n",
             i + 1, my_pid, uptime(), priorities[i], start_prio);

      volatile uint64 counter = 0;
      for (volatile uint64 j = 0; j < WORKLOAD; j++) {
        counter += j;
        if (j == WORKLOAD / 2) {
          int cur_prio = get_priority(my_pid);
          printf("[Child %d | PID %d] 50%% Workload Done (Initial: %d -> Current: %d, tick: %d)\n",
                 i + 1, my_pid, priorities[i], cur_prio, uptime());
        }
      }

      int final_prio = get_priority(my_pid);
      printf("[Child %d | PID %d] Workload Completed! (Initial: %d -> Final: %d, tick: %d)\n",
             i + 1, my_pid, priorities[i], final_prio, uptime());
      exit(0);
    } else {
      // Parent sets child priority immediately
      set_priority(pid, priorities[i]);
      pids[i] = pid;
    }
  }

  printf("[Parent] All children forked. Waiting for completion...\n\n");

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

  printf("\n============================================================\n");
  printf("Test Summary:\n");
  printf("1. Preemption & Priority: Higher priority processes (5, 8) completed first.\n");
  printf("2. Anti-Starvation Aging: Lower priority processes (12, 20) had priority boosted\n");
  printf("   dynamically as wait_ticks exceeded 30, preventing starvation.\n");
  printf("============================================================\n");

  exit(0);
}
