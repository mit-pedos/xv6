#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define N_PHIL 5
#define NUM_CYCLES 5

#define STATE_THINKING 0
#define STATE_HUNGRY   1
#define STATE_EATING   2

// Semaphores 0..4 are forks 0..4.
// Semaphore 5 is a mutex for state and log synchronization.
#define SEM_MUTEX 5

struct dining_shared {
  int state[N_PHIL];
  int meals_eaten[N_PHIL];
  int violations;
  int total_meals;
};

void philosopher(struct dining_shared *ds, int id) {
  int left = id;
  int right = (id + 1) % N_PHIL;

  // Resource ordering deadlock avoidance strategy:
  // Always acquire the lower-numbered fork first, then higher-numbered fork.
  // For philosophers 0, 1, 2, 3: first is left (i), second is right (i+1).
  // For philosopher 4: left is 4, right is 0 -> first is right (0), second is left (4).
  // This breaks circular wait and guarantees freedom from deadlock.
  int first_fork = (left < right) ? left : right;
  int second_fork = (left < right) ? right : left;

  for (int cycle = 1; cycle <= NUM_CYCLES; cycle++) {
    // 1. THINKING
    pause(1);

    // 2. HUNGRY transition
    sem_wait(SEM_MUTEX);
    ds->state[id] = STATE_HUNGRY;
    printf("Philosopher %d [pid %d]: THINKING -> HUNGRY (Cycle %d/%d)\n",
           id, getpid(), cycle, NUM_CYCLES);
    sem_post(SEM_MUTEX);

    // Attempt to acquire first (lower-numbered) fork
    sem_wait(first_fork);
    sem_wait(SEM_MUTEX);
    printf("Philosopher %d [pid %d]: picked up fork %d, waiting for fork %d\n",
           id, getpid(), first_fork, second_fork);
    sem_post(SEM_MUTEX);

    // Attempt to acquire second (higher-numbered) fork
    sem_wait(second_fork);

    // 3. EATING transition
    sem_wait(SEM_MUTEX);
    ds->state[id] = STATE_EATING;

    // Verify mutual exclusion: neither neighbor may be EATING simultaneously
    int left_neighbor = (id + N_PHIL - 1) % N_PHIL;
    int right_neighbor = (id + 1) % N_PHIL;
    if (ds->state[left_neighbor] == STATE_EATING ||
        ds->state[right_neighbor] == STATE_EATING) {
      printf("VIOLATION: Philosopher %d is EATING while neighbor (%d or %d) is also EATING!\n",
             id, left_neighbor, right_neighbor);
      ds->violations++;
    }

    ds->meals_eaten[id]++;
    ds->total_meals++;
    printf("Philosopher %d [pid %d]: HUNGRY -> EATING (holding forks %d, %d) [Meal #%d]\n",
           id, getpid(), first_fork, second_fork, ds->meals_eaten[id]);
    sem_post(SEM_MUTEX);

    // Eating duration
    pause(2);

    // 4. Release both forks
    sem_post(second_fork);
    sem_post(first_fork);

    // EATING -> THINKING transition
    sem_wait(SEM_MUTEX);
    ds->state[id] = STATE_THINKING;
    printf("Philosopher %d [pid %d]: EATING -> THINKING (released forks %d, %d)\n",
           id, getpid(), first_fork, second_fork);
    sem_post(SEM_MUTEX);
  }

  printf("Philosopher %d [pid %d]: Finished all %d dining cycles.\n",
         id, getpid(), NUM_CYCLES);
}

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf("Dining Philosophers Simulation with Deadlock Avoidance\n");
  printf("Philosophers = %d, Forks = %d, Cycles = %d\n", N_PHIL, N_PHIL, NUM_CYCLES);
  printf("Strategy: Resource Ordering (acquire lower-numbered fork first)\n");
  printf("============================================================\n");

  // Obtain shared memory page
  struct dining_shared *ds = (struct dining_shared *)shm_get();
  if (ds == 0) {
    printf("Error: shm_get failed\n");
    exit(1);
  }

  // Initialize shared state
  for (int i = 0; i < N_PHIL; i++) {
    ds->state[i] = STATE_THINKING;
    ds->meals_eaten[i] = 0;
  }
  ds->violations = 0;
  ds->total_meals = 0;

  // Initialize fork semaphores 0..4 (each initialized to 1)
  for (int i = 0; i < N_PHIL; i++) {
    if (sem_init(i, 1) < 0) {
      printf("Error: sem_init fork %d failed\n", i);
      exit(1);
    }
  }

  // Initialize state mutex semaphore 5
  if (sem_init(SEM_MUTEX, 1) < 0) {
    printf("Error: sem_init SEM_MUTEX failed\n");
    exit(1);
  }

  // Fork 5 philosopher processes
  for (int i = 0; i < N_PHIL; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("fork philosopher %d failed\n", i);
      exit(1);
    }
    if (pid == 0) {
      philosopher(ds, i);
      exit(0);
    }
  }

  // Wait for all 5 philosopher processes to complete
  for (int i = 0; i < N_PHIL; i++) {
    wait(0);
  }

  int expected_total = N_PHIL * NUM_CYCLES;

  printf("============================================================\n");
  printf("Simulation Finished Summary:\n");
  for (int i = 0; i < N_PHIL; i++) {
    printf("Philosopher %d ate %d / %d meals.\n",
           i, ds->meals_eaten[i], NUM_CYCLES);
  }
  printf("Total meals eaten: %d (Expected: %d)\n", ds->total_meals, expected_total);
  printf("Mutual exclusion violations: %d\n", ds->violations);

  if (ds->total_meals == expected_total && ds->violations == 0) {
    printf("SUCCESS:\n");
    printf("  1. All 5 philosophers completed all %d cycles without deadlocking.\n", NUM_CYCLES);
    printf("  2. No two adjacent philosophers ate simultaneously.\n");
    printf("  3. Resource ordering effectively eliminated circular wait.\n");
  } else {
    printf("FAILURE: Dining simulation assertions failed!\n");
  }
  printf("============================================================\n");

  exit(0);
}
