#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_READERS 3
#define NUM_WRITERS 2
#define READS_PER_READER 5
#define WRITES_PER_WRITER 5

#define SEM_MUTEX 0
#define SEM_WRT   1

struct rw_shared {
  int shared_data;
  int read_count;
  int active_readers;
  int active_writers;
  int max_readers_seen;
  int total_writes;
  int violations;
};

void reader(struct rw_shared *sh, int id) {
  printf("Reader %d started (pid %d)\n", id, getpid());

  for (int i = 0; i < READS_PER_READER; i++) {
    // Entry section
    sem_wait(SEM_MUTEX);
    sh->read_count++;
    if (sh->read_count == 1) {
      sem_wait(SEM_WRT); // First reader locks out writers
    }
    sh->active_readers++;
    int cur_readers = sh->active_readers;
    if (cur_readers > sh->max_readers_seen) {
      sh->max_readers_seen = cur_readers;
    }
    sem_post(SEM_MUTEX);

    // Reading Critical Section
    if (sh->active_writers > 0) {
      printf("VIOLATION: Reader [pid %d] reading while writer active!\n", getpid());
      sh->violations++;
    }

    int val = sh->shared_data;
    printf("Reader %d [pid %d]: read shared_data = %d | active readers = %d\n",
           id, getpid(), val, cur_readers);

    pause(1); // simulate reading time & encourage concurrency

    // Exit section
    sem_wait(SEM_MUTEX);
    sh->active_readers--;
    sh->read_count--;
    if (sh->read_count == 0) {
      sem_post(SEM_WRT); // Last reader unlocks writers
    }
    sem_post(SEM_MUTEX);

    pause(1); // remainder delay
  }

  printf("Reader %d [pid %d] finished all %d reads.\n", id, getpid(), READS_PER_READER);
}

void writer(struct rw_shared *sh, int id) {
  printf("Writer %d started (pid %d)\n", id, getpid());

  for (int i = 0; i < WRITES_PER_WRITER; i++) {
    // Entry section: request exclusive lock
    sem_wait(SEM_WRT);

    // Writing Critical Section
    sh->active_writers++;

    // Assert mutual exclusion
    if (sh->active_readers != 0) {
      printf("VIOLATION: Writer [pid %d] writing while %d readers active!\n",
             getpid(), sh->active_readers);
      sh->violations++;
    }
    if (sh->active_writers > 1) {
      printf("VIOLATION: Multiple writers active simultaneously (%d)!\n",
             sh->active_writers);
      sh->violations++;
    }

    sh->shared_data++;
    sh->total_writes++;
    int new_val = sh->shared_data;

    printf("Writer %d [pid %d]: wrote shared_data = %d | active readers = %d, active writers = %d\n",
           id, getpid(), new_val, sh->active_readers, sh->active_writers);

    pause(1); // simulate write duration

    sh->active_writers--;

    // Exit section: release exclusive lock
    sem_post(SEM_WRT);

    pause(2); // remainder delay to give readers opportunities
  }

  printf("Writer %d [pid %d] finished all %d writes.\n", id, getpid(), WRITES_PER_WRITER);
}

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf("Readers-Writers Problem Simulation (Readers-Preference)\n");
  printf("Readers = %d, Writers = %d\n", NUM_READERS, NUM_WRITERS);
  printf("Reads per reader = %d, Writes per writer = %d\n",
         READS_PER_READER, WRITES_PER_WRITER);
  printf("============================================================\n");

  // Obtain shared memory page
  struct rw_shared *sh = (struct rw_shared *)shm_get();
  if (sh == 0) {
    printf("Error: shm_get failed\n");
    exit(1);
  }

  // Initialize shared state
  sh->shared_data = 0;
  sh->read_count = 0;
  sh->active_readers = 0;
  sh->active_writers = 0;
  sh->max_readers_seen = 0;
  sh->total_writes = 0;
  sh->violations = 0;

  // Initialize semaphores
  // SEM_MUTEX (0) = 1 protects read_count
  // SEM_WRT   (1) = 1 protects writing critical section
  if (sem_init(SEM_MUTEX, 1) < 0 || sem_init(SEM_WRT, 1) < 0) {
    printf("Error: sem_init failed\n");
    exit(1);
  }

  // Fork readers
  for (int i = 1; i <= NUM_READERS; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("fork reader failed\n");
      exit(1);
    }
    if (pid == 0) {
      reader(sh, i);
      exit(0);
    }
  }

  // Fork writers
  for (int i = 1; i <= NUM_WRITERS; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("fork writer failed\n");
      exit(1);
    }
    if (pid == 0) {
      writer(sh, i);
      exit(0);
    }
  }

  // Wait for all reader and writer processes
  int total_children = NUM_READERS + NUM_WRITERS;
  for (int i = 0; i < total_children; i++) {
    wait(0);
  }

  int expected_final_data = NUM_WRITERS * WRITES_PER_WRITER;

  printf("============================================================\n");
  printf("Simulation Finished Summary:\n");
  printf("Final shared_data value: %d (Expected: %d)\n",
         sh->shared_data, expected_final_data);
  printf("Total writes recorded: %d\n", sh->total_writes);
  printf("Max concurrent readers observed: %d\n", sh->max_readers_seen);
  printf("Synchronization violations: %d\n", sh->violations);

  if (sh->shared_data == expected_final_data &&
      sh->violations == 0 &&
      sh->max_readers_seen > 1) {
    printf("SUCCESS:\n");
    printf("  1. Multiple readers read simultaneously (max = %d).\n", sh->max_readers_seen);
    printf("  2. Writers had exclusive access (active_readers = 0 and active_writers = 1).\n");
    printf("  3. No lost updates or race conditions occurred.\n");
  } else {
    printf("FAILURE: Synchronization assertions failed!\n");
  }
  printf("============================================================\n");

  exit(0);
}
