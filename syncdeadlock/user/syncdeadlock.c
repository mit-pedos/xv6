#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_PROCS 5
#define NUM_RES_TYPES 3
#define NUM_CYCLES 3

#define RES_PRINTER 0
#define RES_SCANNER 1
#define RES_DISK    2

#define SEM_MUTEX   3

static char *res_names[NUM_RES_TYPES] = {"Printer", "Scanner", "Disk"};
static int res_capacities[NUM_RES_TYPES] = {2, 1, 2}; // 2 printers, 1 scanner, 2 disks

struct shared_state {
  int in_use[NUM_RES_TYPES];
  int completed[NUM_PROCS];
  int violations;
  int total_tasks;
};

// Resource requirements for each of the 5 processes (2 resources per process)
int proc_needs[NUM_PROCS][2] = {
  {RES_PRINTER, RES_SCANNER}, // P0 needs Printer & Scanner
  {RES_SCANNER, RES_DISK},    // P1 needs Scanner & Disk
  {RES_PRINTER, RES_DISK},    // P2 needs Printer & Disk
  {RES_PRINTER, RES_SCANNER}, // P3 needs Printer & Scanner
  {RES_SCANNER, RES_DISK}     // P4 needs Scanner & Disk
};

void run_process(struct shared_state *sh, int id) {
  int r1 = proc_needs[id][0];
  int r2 = proc_needs[id][1];

  // Resource ordering deadlock avoidance strategy:
  // Strictly request lower-numbered resource type first, then higher-numbered.
  int first = (r1 < r2) ? r1 : r2;
  int second = (r1 < r2) ? r2 : r1;

  for (int cycle = 1; cycle <= NUM_CYCLES; cycle++) {
    // 1. Request first resource
    sem_wait(SEM_MUTEX);
    printf("[Proc %d (pid %d)]: Requesting %s (first resource)...\n",
           id, getpid(), res_names[first]);
    sem_post(SEM_MUTEX);

    sem_wait(first);

    sem_wait(SEM_MUTEX);
    sh->in_use[first]++;
    printf("[Proc %d (pid %d)]: GRANTED %s (In use: %d/%d)\n",
           id, getpid(), res_names[first], sh->in_use[first], res_capacities[first]);
    sem_post(SEM_MUTEX);

    // 2. Request second resource
    sem_wait(SEM_MUTEX);
    printf("[Proc %d (pid %d)]: Requesting %s (second resource)...\n",
           id, getpid(), res_names[second]);
    sem_post(SEM_MUTEX);

    sem_wait(second);

    sem_wait(SEM_MUTEX);
    sh->in_use[second]++;
    printf("[Proc %d (pid %d)]: GRANTED %s (In use: %d/%d)\n",
           id, getpid(), res_names[second], sh->in_use[second], res_capacities[second]);

    // Check invariants
    for (int k = 0; k < NUM_RES_TYPES; k++) {
      if (sh->in_use[k] > res_capacities[k]) {
        printf("VIOLATION: Resource %s exceeded capacity! (Used: %d, Max: %d)\n",
               res_names[k], sh->in_use[k], res_capacities[k]);
        sh->violations++;
      }
    }

    printf("[Proc %d (pid %d)]: Holding [%s, %s]. STARTING WORK (Cycle %d/%d)...\n",
           id, getpid(), res_names[first], res_names[second], cycle, NUM_CYCLES);
    sem_post(SEM_MUTEX);

    // Simulate task execution
    pause(2);

    // 3. Release resources
    sem_wait(SEM_MUTEX);
    sh->in_use[second]--;
    sh->in_use[first]--;
    sh->completed[id]++;
    sh->total_tasks++;
    printf("[Proc %d (pid %d)]: FINISHED WORK. Released [%s, %s] (Cycle %d/%d done).\n",
           id, getpid(), res_names[first], res_names[second], cycle, NUM_CYCLES);
    sem_post(SEM_MUTEX);

    sem_post(second);
    sem_post(first);

    // Brief delay before next cycle
    pause(1);
  }

  printf("[Proc %d (pid %d)]: Completed all %d cycles cleanly.\n",
         id, getpid(), NUM_CYCLES);
}

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf(" Multi-Resource Synchronization with Deadlock Avoidance   \n");
  printf(" Processes: 5, Resources: Printer(2), Scanner(1), Disk(2)   \n");
  printf(" Strategy : Resource Ordering (Hierarchy of Resources)      \n");
  printf(" Cycles   : %d per process (Total tasks = %d)               \n",
         NUM_CYCLES, NUM_PROCS * NUM_CYCLES);
  printf("============================================================\n");

  // Obtain shared memory page
  struct shared_state *sh = (struct shared_state *)shm_get();
  if (sh == 0) {
    printf("Error: shm_get failed\n");
    exit(1);
  }

  // Initialize shared state
  for (int i = 0; i < NUM_RES_TYPES; i++) {
    sh->in_use[i] = 0;
  }
  for (int i = 0; i < NUM_PROCS; i++) {
    sh->completed[i] = 0;
  }
  sh->violations = 0;
  sh->total_tasks = 0;

  // Initialize semaphores
  // SEM_PRINTER (0) = 2
  // SEM_SCANNER (1) = 1
  // SEM_DISK    (2) = 2
  // SEM_MUTEX   (3) = 1
  if (sem_init(RES_PRINTER, res_capacities[RES_PRINTER]) < 0 ||
      sem_init(RES_SCANNER, res_capacities[RES_SCANNER]) < 0 ||
      sem_init(RES_DISK,    res_capacities[RES_DISK]) < 0 ||
      sem_init(SEM_MUTEX,   1) < 0) {
    printf("Error: sem_init failed\n");
    exit(1);
  }

  // Fork 5 processes
  for (int i = 0; i < NUM_PROCS; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("fork failed for proc %d\n", i);
      exit(1);
    }
    if (pid == 0) {
      run_process(sh, i);
      exit(0);
    }
  }

  // Wait for all 5 processes to finish
  for (int i = 0; i < NUM_PROCS; i++) {
    wait(0);
  }

  int expected_total = NUM_PROCS * NUM_CYCLES;

  printf("============================================================\n");
  printf("Simulation Finished Summary:\n");
  for (int i = 0; i < NUM_PROCS; i++) {
    printf("Process %d completed: %d / %d tasks.\n",
           i, sh->completed[i], NUM_CYCLES);
  }
  printf("Total tasks completed: %d (Expected: %d)\n", sh->total_tasks, expected_total);
  printf("Capacity violations: %d\n", sh->violations);

  if (sh->total_tasks == expected_total && sh->violations == 0) {
    printf("SUCCESS:\n");
    printf("  1. All 5 processes completed without any deadlock or starvation.\n");
    printf("  2. Resource capacities (Printer: 2, Scanner: 1, Disk: 2) were strictly preserved.\n");
    printf("  3. Resource ordering guaranteed acyclic wait-for relationships.\n");
  } else {
    printf("FAILURE: Simulation assertions failed!\n");
  }
  printf("============================================================\n");

  exit(0);
}
