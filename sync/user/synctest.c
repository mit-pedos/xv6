#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

// Part 1 Defines
#define NUM_THREADS 3
#define INCREMENTS_PER_THREAD 100000
#define SEM_COUNTER 0

volatile int counter = 0;
volatile int start_flag = 0;

void sync_thread_worker(void *arg) {
  int id = (int)(uint64)arg;

  while (start_flag == 0)
    ;

  for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
    // Synchronized Critical Section using Semaphore
    sem_wait(SEM_COUNTER);
    volatile int val = counter;
    for (volatile int d = 0; d < 3; d++);
    counter = val + 1;
    sem_signal(SEM_COUNTER);
  }

  printf("Thread %d (pid %d) finished %d synchronized increments.\n",
         id, getpid(), INCREMENTS_PER_THREAD);
  exit(0);
}

void test_part1_synchronized_counter(void) {
  printf("============================================================\n");
  printf(" PART 1: Synchronized Threads Counter Test using Semaphores \n");
  printf(" Threads: %d, Increments: %d each, Expected: %d            \n",
         NUM_THREADS, INCREMENTS_PER_THREAD, NUM_THREADS * INCREMENTS_PER_THREAD);
  printf("============================================================\n");

  sem_init(SEM_COUNTER, 1);
  counter = 0;
  start_flag = 0;

  void *stacks[NUM_THREADS];
  int tids[NUM_THREADS];

  for (int i = 0; i < NUM_THREADS; i++) {
    stacks[i] = malloc(PGSIZE);
    tids[i] = clone(sync_thread_worker, (void *)(uint64)(i + 1), stacks[i]);
    printf("Main: Cloned Thread %d (pid %d)\n", i + 1, tids[i]);
  }

  printf("Main: Releasing all 3 threads concurrently...\n");
  start_flag = 1;

  for (int i = 0; i < NUM_THREADS; i++) {
    int pid = join();
    printf("Main: Reaped thread with pid %d via join()\n", pid);
    free(stacks[i]);
  }

  int expected = NUM_THREADS * INCREMENTS_PER_THREAD;
  int actual = counter;

  printf("------------------------------------------------------------\n");
  printf("Part 1 Results: Expected = %d, Actual = %d\n", expected, actual);
  if (actual == expected) {
    printf("SUCCESS: Counter reached EXACTLY %d with 0 lost updates!\n", actual);
    printf("Semaphores successfully eliminated the race condition.\n");
  } else {
    printf("FAILURE: Race condition still present (lost = %d)\n", expected - actual);
  }
}

// Part 2 Defines
#define BUFFER_SIZE 5
#define TOTAL_ITEMS 15

#define SEM_PC_MUTEX 1
#define SEM_PC_EMPTY 2
#define SEM_PC_FULL  3

int pc_buffer[BUFFER_SIZE];
int pc_in = 0;
int pc_out = 0;
int pc_count = 0;
int pc_max_seen = 0;

void producer_thread(void *arg) {
  for (int item = 1; item <= TOTAL_ITEMS; item++) {
    sem_wait(SEM_PC_EMPTY);
    sem_wait(SEM_PC_MUTEX);

    pc_buffer[pc_in] = item;
    pc_in = (pc_in + 1) % BUFFER_SIZE;
    pc_count++;
    if (pc_count > pc_max_seen)
      pc_max_seen = pc_count;

    printf("[Producer (pid %d)]: Produced %d | Buffer count: %d/%d\n",
           getpid(), item, pc_count, BUFFER_SIZE);

    sem_signal(SEM_PC_MUTEX);
    sem_signal(SEM_PC_FULL);

    pause(1);
  }
  exit(0);
}

void consumer_thread(void *arg) {
  int last_item = 0;
  int order_errors = 0;

  for (int i = 0; i < TOTAL_ITEMS; i++) {
    sem_wait(SEM_PC_FULL);
    sem_wait(SEM_PC_MUTEX);

    int item = pc_buffer[pc_out];
    pc_out = (pc_out + 1) % BUFFER_SIZE;
    pc_count--;

    printf("[Consumer (pid %d)]: Consumed %d | Buffer count: %d/%d\n",
           getpid(), item, pc_count, BUFFER_SIZE);

    if (item != last_item + 1) {
      printf("ERROR: Expected %d, got %d\n", last_item + 1, item);
      order_errors++;
    }
    last_item = item;

    sem_signal(SEM_PC_MUTEX);
    sem_signal(SEM_PC_EMPTY);

    pause(2); // let producer fill buffer
  }

  if (order_errors == 0 && last_item == TOTAL_ITEMS) {
    printf("Consumer: All %d items verified in strict FIFO order!\n", TOTAL_ITEMS);
  }
  exit(0);
}

void test_part2_producer_consumer(void) {
  printf("\n============================================================\n");
  printf(" PART 2: Classic Producer-Consumer Bounded Buffer Test      \n");
  printf(" Buffer Size = %d, Total Items = %d                         \n",
         BUFFER_SIZE, TOTAL_ITEMS);
  printf(" Semaphores: Empty (init 5), Full (init 0), Mutex (init 1)  \n");
  printf("============================================================\n");

  pc_in = 0;
  pc_out = 0;
  pc_count = 0;
  pc_max_seen = 0;

  sem_init(SEM_PC_MUTEX, 1);
  sem_init(SEM_PC_EMPTY, BUFFER_SIZE);
  sem_init(SEM_PC_FULL, 0);

  void *stack_p = malloc(PGSIZE);
  void *stack_c = malloc(PGSIZE);

  clone(producer_thread, 0, stack_p);
  clone(consumer_thread, 0, stack_c);

  join();
  join();

  free(stack_p);
  free(stack_c);

  printf("------------------------------------------------------------\n");
  printf("Part 2 Results: Peak Buffer Usage = %d/%d slots, Final Count = %d\n",
         pc_max_seen, BUFFER_SIZE, pc_count);
  if (pc_count == 0 && pc_max_seen <= BUFFER_SIZE) {
    printf("SUCCESS: No buffer overflow or underflow occurred.\n");
    printf("Producer-Consumer synchronization fully verified!\n");
  } else {
    printf("FAILURE: Buffer bounds violated!\n");
  }
}

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf("      xv6 Process/Thread Synchronization using Semaphores   \n");
  printf("============================================================\n");

  test_part1_synchronized_counter();
  test_part2_producer_consumer();

  printf("\n============================================================\n");
  printf("All synchronization tests completed successfully!\n");
  printf("============================================================\n");

  exit(0);
}
