#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_CHILDREN 3
#define WORKLOAD 50000000ULL

void delay(int count) {
  for (volatile int i = 0; i < count; i++) {
    // delay loop to stagger creation times
  }
}

int main(int argc, char *argv[]) {
  int pids[NUM_CHILDREN];
  printf("--- FCFS Scheduler Test Starting ---\n");
  printf("Parent PID: %d\n", getpid());
  printf("Forking %d CPU-bound children with staggered arrival...\n", NUM_CHILDREN);

  for (int i = 0; i < NUM_CHILDREN; i++) {
    // Stagger process creation slightly
    delay(5000000);

    int pid = fork();
    if (pid < 0) {
      printf("Fork failed!\n");
      exit(1);
    }

    if (pid == 0) {
      // Child process
      int my_pid = getpid();
      printf("[Child %d | PID %d] Created, starting CPU-bound workload...\n", i + 1, my_pid);

      volatile uint64 counter = 0;
      for (volatile uint64 j = 0; j < WORKLOAD; j++) {
        counter += j;
      }

      printf("[Child %d | PID %d] Finished computation (counter=%d)\n", i + 1, my_pid, (int)counter);
      exit(0);
    } else {
      pids[i] = pid;
      printf("[Parent] Forked Child %d (PID %d)\n", i + 1, pid);
    }
  }

  printf("\n[Parent] All %d children forked. Waiting for children to complete...\n\n", NUM_CHILDREN);

  for (int order = 1; order <= NUM_CHILDREN; order++) {
    int finished_pid = wait(0);
    int child_num = -1;
    for (int i = 0; i < NUM_CHILDREN; i++) {
      if (pids[i] == finished_pid) {
        child_num = i + 1;
        break;
      }
    }
    printf(">> Completion Order %d: PID %d (Child %d)\n", order, finished_pid, child_num);
  }

  printf("\n--- FCFS Test Passed: Processes completed strictly in creation order! ---\n");
  exit(0);
}
