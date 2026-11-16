#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define BUFFER_SIZE 5
#define TOTAL_ITEMS 20

#define SEM_MUTEX 0
#define SEM_EMPTY 1
#define SEM_FULL  2

struct shared_buffer {
  int buffer[BUFFER_SIZE];
  int in;
  int out;
  int count;
  int max_observed;
  int min_observed;
};

void producer(struct shared_buffer *sb) {
  printf("Producer started (pid %d)\n", getpid());
  for (int item = 1; item <= TOTAL_ITEMS; item++) {
    // Check if buffer is currently full before waiting
    if (sb->count == BUFFER_SIZE) {
      printf("Producer: Buffer is FULL (%d/%d slots), waiting on empty semaphore...\n",
             sb->count, BUFFER_SIZE);
    }

    // Wait for free slot
    sem_wait(SEM_EMPTY);
    // Enter critical section
    sem_wait(SEM_MUTEX);

    // Insert item
    sb->buffer[sb->in] = item;
    sb->in = (sb->in + 1) % BUFFER_SIZE;
    sb->count++;

    if (sb->count > sb->max_observed)
      sb->max_observed = sb->count;

    printf("Producer [pid %d]: Produced item %d | Buffer count: %d/%d\n",
           getpid(), item, sb->count, BUFFER_SIZE);

    // Exit critical section
    sem_post(SEM_MUTEX);
    // Signal that a slot is full
    sem_post(SEM_FULL);

    // Sleep briefly (1 tick)
    pause(1);
  }
  printf("Producer finished producing %d items.\n", TOTAL_ITEMS);
}

void consumer(struct shared_buffer *sb) {
  int last_item = 0;
  int errors = 0;

  printf("Consumer started (pid %d)\n", getpid());
  for (int i = 0; i < TOTAL_ITEMS; i++) {
    // Check if buffer is currently empty before waiting
    if (sb->count == 0) {
      printf("Consumer: Buffer is EMPTY (0/%d slots), waiting on full semaphore...\n",
             BUFFER_SIZE);
    }

    // Wait for available item
    sem_wait(SEM_FULL);
    // Enter critical section
    sem_wait(SEM_MUTEX);

    // Remove item
    int item = sb->buffer[sb->out];
    sb->out = (sb->out + 1) % BUFFER_SIZE;
    sb->count--;

    if (sb->count < sb->min_observed)
      sb->min_observed = sb->count;

    printf("Consumer [pid %d]: Consumed item %d | Buffer count: %d/%d\n",
           getpid(), item, sb->count, BUFFER_SIZE);

    // Verify FIFO order and no data corruption
    if (item != last_item + 1) {
      printf("ERROR: Expected item %d but got %d!\n", last_item + 1, item);
      errors++;
    }
    last_item = item;

    // Exit critical section
    sem_post(SEM_MUTEX);
    // Signal that a slot is empty
    sem_post(SEM_EMPTY);

    // Sleep briefly (2 ticks) to let producer outpace consumer and test buffer full condition
    pause(2);
  }

  if (errors == 0 && last_item == TOTAL_ITEMS) {
    printf("Consumer verification: ALL %d items received correctly in FIFO order!\n", TOTAL_ITEMS);
  } else {
    printf("Consumer verification: FAILED with %d order errors.\n", errors);
  }
}

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf("Producer-Consumer Bounded Buffer Simulation with Semaphores\n");
  printf("Buffer capacity = %d, Total items = %d\n", BUFFER_SIZE, TOTAL_ITEMS);
  printf("============================================================\n");

  // Obtain shared memory page
  struct shared_buffer *sb = (struct shared_buffer *)shm_get();
  if (sb == 0) {
    printf("Error: shm_get failed\n");
    exit(1);
  }

  // Initialize shared state
  sb->in = 0;
  sb->out = 0;
  sb->count = 0;
  sb->max_observed = 0;
  sb->min_observed = 0;

  // Initialize semaphores
  // SEM_MUTEX = 1 (binary semaphore for mutual exclusion)
  // SEM_EMPTY = BUFFER_SIZE (counting semaphore for available slots)
  // SEM_FULL  = 0 (counting semaphore for filled slots)
  if (sem_init(SEM_MUTEX, 1) < 0 ||
      sem_init(SEM_EMPTY, BUFFER_SIZE) < 0 ||
      sem_init(SEM_FULL, 0) < 0) {
    printf("Error: sem_init failed\n");
    exit(1);
  }

  int pid_prod = fork();
  if (pid_prod < 0) {
    printf("fork producer failed\n");
    exit(1);
  }
  if (pid_prod == 0) {
    producer(sb);
    exit(0);
  }

  int pid_cons = fork();
  if (pid_cons < 0) {
    printf("fork consumer failed\n");
    exit(1);
  }
  if (pid_cons == 0) {
    consumer(sb);
    exit(0);
  }

  // Wait for both children to terminate
  wait(0);
  wait(0);

  printf("============================================================\n");
  printf("Simulation Finished Summary:\n");
  printf("Peak buffer usage: %d / %d slots\n", sb->max_observed, BUFFER_SIZE);
  printf("Min buffer usage: %d / %d slots\n", sb->min_observed, BUFFER_SIZE);
  if (sb->count == 0 && sb->max_observed <= BUFFER_SIZE && sb->min_observed >= 0) {
    printf("SUCCESS: No buffer overflow or underflow detected.\n");
    printf("Buffer is cleanly drained (final count = 0).\n");
  } else {
    printf("FAILURE: Buffer state violated!\n");
  }
  printf("============================================================\n");

  exit(0);
}
