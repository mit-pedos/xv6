#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_THREADS 3
#define INCREMENTS_PER_THREAD 100000
#define PGSIZE 4096

volatile int counter = 0;
volatile int start_flag = 0;

void thread_worker(void *arg) {
  int id = (int)(uint64)arg;

  // Wait until all threads are created so they run truly concurrently
  while (start_flag == 0)
    ;

  for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
    // Non-atomic read-modify-write on shared memory variable
    volatile int val = counter;
    for (volatile int d = 0; d < 3; d++); // short window to expose race condition
    counter = val + 1;
  }

  printf("Thread %d (pid %d) finished %d increments.\n",
         id, getpid(), INCREMENTS_PER_THREAD);
  exit(0);
}

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf("   Kernel-Supported Threads Demonstration with clone()      \n");
  printf("   Threads: %d, Increments per thread: %d                   \n",
         NUM_THREADS, INCREMENTS_PER_THREAD);
  printf("   Expected counter without race condition: %d              \n",
         NUM_THREADS * INCREMENTS_PER_THREAD);
  printf("============================================================\n");

  void *stacks[NUM_THREADS];
  int tids[NUM_THREADS];

  counter = 0;

  // Create threads
  for (int i = 0; i < NUM_THREADS; i++) {
    stacks[i] = malloc(PGSIZE);
    if (stacks[i] == 0) {
      printf("Error: malloc failed for thread %d stack\n", i);
      exit(1);
    }

    tids[i] = clone(thread_worker, (void *)(uint64)(i + 1), stacks[i]);
    if (tids[i] < 0) {
      printf("Error: clone failed for thread %d\n", i);
      exit(1);
    }
    printf("Main (pid %d): Cloned Thread %d with pid %d\n",
           getpid(), i + 1, tids[i]);
  }

  // Release all threads simultaneously across CPUs
  printf("Main: Releasing all 3 threads concurrently...\n");
  start_flag = 1;

  // Join all threads
  for (int i = 0; i < NUM_THREADS; i++) {
    int joined_pid = join();
    printf("Main: Reaped thread with pid %d via join()\n", joined_pid);
  }

  int expected = NUM_THREADS * INCREMENTS_PER_THREAD;
  int actual = counter;

  printf("============================================================\n");
  printf("Simulation Finished Summary:\n");
  printf("All %d threads executed in the SAME shared address space.\n", NUM_THREADS);
  printf("Expected Counter : %d\n", expected);
  printf("Actual Counter   : %d\n", actual);

  if (actual < expected) {
    int lost = expected - actual;
    int pct = (lost * 100) / expected;
    printf("Race condition confirmed: Lost updates = %d (%d%% loss).\n",
           lost, pct);
    printf("Explanation: Unsynchronized concurrent execution caused\n");
    printf("interleaved read-modify-write cycles. (Fixed in Question 2 using Semaphores).\n");
  } else {
    printf("Counter reached %d.\n", actual);
  }
  printf("============================================================\n");

  // Free allocated stack pages
  for (int i = 0; i < NUM_THREADS; i++) {
    free(stacks[i]);
  }

  exit(0);
}
