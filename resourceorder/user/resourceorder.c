#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define LOCK1 0
#define LOCK2 1

void run_deadlock_scenario(void) {
  printf("============================================================\n");
  printf("  SCENARIO 1: INCONSISTENT ORDERING (DEADLOCK DEMONSTRATION)\n");
  printf("  Process A: Requests Lock1 -> Lock2\n");
  printf("  Process B: Requests Lock2 -> Lock1 (Reverse Order)\n");
  printf("============================================================\n");

  sem_init(LOCK1, 1);
  sem_init(LOCK2, 1);

  int pid_a = fork();
  if (pid_a < 0) {
    printf("fork A failed\n");
    exit(1);
  }
  if (pid_a == 0) {
    // Process A
    printf("[Tick %d] [Process A (pid %d)]: Attempting to acquire Lock 1...\n",
           uptime(), getpid());
    sem_wait(LOCK1);
    printf("[Tick %d] [Process A (pid %d)]: ACQUIRED Lock 1.\n",
           uptime(), getpid());

    // Delay to let Process B acquire Lock 2
    printf("[Tick %d] [Process A (pid %d)]: Holding Lock 1, sleeping 2 ticks...\n",
           uptime(), getpid());
    pause(2);

    printf("[Tick %d] [Process A (pid %d)]: Attempting to acquire Lock 2...\n",
           uptime(), getpid());
    sem_wait(LOCK2);

    // Will never reach here due to deadlock
    printf("[Tick %d] [Process A (pid %d)]: ACQUIRED Lock 2. Entering CS.\n",
           uptime(), getpid());
    sem_post(LOCK2);
    sem_post(LOCK1);
    exit(0);
  }

  int pid_b = fork();
  if (pid_b < 0) {
    printf("fork B failed\n");
    exit(1);
  }
  if (pid_b == 0) {
    // Process B: small delay to let Process A grab Lock 1 first
    pause(1);

    printf("[Tick %d] [Process B (pid %d)]: Attempting to acquire Lock 2...\n",
           uptime(), getpid());
    sem_wait(LOCK2);
    printf("[Tick %d] [Process B (pid %d)]: ACQUIRED Lock 2.\n",
           uptime(), getpid());

    // Delay to guarantee Process A has reached wait on Lock 2
    printf("[Tick %d] [Process B (pid %d)]: Holding Lock 2, sleeping 2 ticks...\n",
           uptime(), getpid());
    pause(2);

    printf("[Tick %d] [Process B (pid %d)]: Attempting to acquire Lock 1...\n",
           uptime(), getpid());
    sem_wait(LOCK1);

    // Will never reach here due to deadlock
    printf("[Tick %d] [Process B (pid %d)]: ACQUIRED Lock 1. Entering CS.\n",
           uptime(), getpid());
    sem_post(LOCK1);
    sem_post(LOCK2);
    exit(0);
  }

  // Parent monitors processes
  // In this mode, both children hang forever in circular wait
  wait(0);
  wait(0);
}

void run_fixed_scenario(void) {
  printf("============================================================\n");
  printf("  SCENARIO 2: RESOURCE ORDERING (DEADLOCK PREVENTED)\n");
  printf("  Global Order: Lock1 (Resource 0) < Lock2 (Resource 1)\n");
  printf("  Both Process A & Process B acquire Lock1 THEN Lock2.\n");
  printf("============================================================\n");

  sem_init(LOCK1, 1);
  sem_init(LOCK2, 1);

  int pid_a = fork();
  if (pid_a < 0) {
    printf("fork A failed\n");
    exit(1);
  }
  if (pid_a == 0) {
    // Process A
    printf("[Tick %d] [Process A (pid %d)]: Attempting to acquire Lock 1...\n",
           uptime(), getpid());
    sem_wait(LOCK1);
    printf("[Tick %d] [Process A (pid %d)]: ACQUIRED Lock 1.\n",
           uptime(), getpid());

    pause(1);

    printf("[Tick %d] [Process A (pid %d)]: Attempting to acquire Lock 2...\n",
           uptime(), getpid());
    sem_wait(LOCK2);
    printf("[Tick %d] [Process A (pid %d)]: ACQUIRED Lock 2. Entering Critical Section.\n",
           uptime(), getpid());

    pause(2); // In CS

    printf("[Tick %d] [Process A (pid %d)]: Exiting Critical Section. Releasing Lock 2 and Lock 1.\n",
           uptime(), getpid());
    sem_post(LOCK2);
    sem_post(LOCK1);
    printf("[Tick %d] [Process A (pid %d)]: Completed successfully.\n",
           uptime(), getpid());
    exit(0);
  }

  int pid_b = fork();
  if (pid_b < 0) {
    printf("fork B failed\n");
    exit(1);
  }
  if (pid_b == 0) {
    // Process B also respects global order: Lock 1 THEN Lock 2
    pause(1);

    printf("[Tick %d] [Process B (pid %d)]: Attempting to acquire Lock 1...\n",
           uptime(), getpid());
    sem_wait(LOCK1);
    printf("[Tick %d] [Process B (pid %d)]: ACQUIRED Lock 1.\n",
           uptime(), getpid());

    printf("[Tick %d] [Process B (pid %d)]: Attempting to acquire Lock 2...\n",
           uptime(), getpid());
    sem_wait(LOCK2);
    printf("[Tick %d] [Process B (pid %d)]: ACQUIRED Lock 2. Entering Critical Section.\n",
           uptime(), getpid());

    pause(2); // In CS

    printf("[Tick %d] [Process B (pid %d)]: Exiting Critical Section. Releasing Lock 2 and Lock 1.\n",
           uptime(), getpid());
    sem_post(LOCK2);
    sem_post(LOCK1);
    printf("[Tick %d] [Process B (pid %d)]: Completed successfully.\n",
           uptime(), getpid());
    exit(0);
  }

  // Parent waits for both children to complete
  wait(0);
  wait(0);

  printf("============================================================\n");
  printf("SUCCESS: Both Process A and Process B completed cleanly!\n");
  printf("Deadlock was completely eliminated by enforcing global resource ordering.\n");
  printf("============================================================\n");
}

int main(int argc, char *argv[]) {
  if (argc > 1 && strcmp(argv[1], "bad") == 0) {
    run_deadlock_scenario();
  } else {
    run_fixed_scenario();
  }
  exit(0);
}
