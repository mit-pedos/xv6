#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Peterson's shared variables in shared memory
struct peterson_data {
  volatile int flag[2];       // Intent to enter critical section
  volatile int turn;          // Turn variable
  volatile int shared_counter;// Critical section shared resource
};

#define ITERATIONS 10

void remainder_delay(void) {
  for (volatile int d = 0; d < 200000; d++) {
    // busy delay in remainder section
  }
}

int main(int argc, char *argv[]) {
  printf("--- Peterson's Algorithm Mutual Exclusion Test ---\n");

  // Allocate shared memory page
  struct peterson_data *shm = (struct peterson_data *)shm_get();
  if (shm == 0) {
    printf("Failed to allocate shared memory!\n");
    exit(1);
  }

  // Initialize Peterson's algorithm state
  shm->flag[0] = 0;
  shm->flag[1] = 0;
  shm->turn = 0;
  shm->shared_counter = 0;

  int pid = fork();
  if (pid < 0) {
    printf("Fork failed!\n");
    exit(1);
  }

  int id = (pid == 0) ? 1 : 0; // 0 = Parent, 1 = Child
  int other = 1 - id;

  for (int i = 0; i < ITERATIONS; i++) {
    // --- Entry Section ---
    shm->flag[id] = 1;
    shm->turn = other;
    __sync_synchronize(); // memory barrier

    while (shm->flag[other] && shm->turn == other) {
      // Busy wait until it is safe to enter
    }

    // --- Critical Section ---
    shm->shared_counter++;
    printf("Process %d in CS, counter = %d\n", id, shm->shared_counter);

    // Short delay inside CS to rigorously test mutual exclusion
    for (volatile int k = 0; k < 10000; k++);

    // --- Exit Section ---
    __sync_synchronize();
    shm->flag[id] = 0;

    // --- Remainder Section ---
    remainder_delay();
  }

  if (pid != 0) {
    // Parent waits for child to finish
    wait(0);
    printf("\nExpected final counter = %d\n", 2 * ITERATIONS);
    printf("Actual final counter   = %d\n", shm->shared_counter);
    if (shm->shared_counter == 2 * ITERATIONS) {
      printf("--- Peterson's Algorithm Test Passed: Mutual exclusion preserved, zero lost updates! ---\n");
    } else {
      printf("--- Test Failed: Counter mismatch! ---\n");
    }
  }

  exit(0);
}
