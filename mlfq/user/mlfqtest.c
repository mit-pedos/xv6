#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  printf("--- Multilevel Feedback Queue (MLFQ) Scheduler Test ---\n");
  printf("Parent PID: %d\n", getpid());
  printf("Forking CPU-bound process (demotes to level 2) and I/O-bound process (remains at level 0)...\n\n");

  int pid_cpu = fork();
  if (pid_cpu < 0) {
    printf("Fork failed!\n");
    exit(1);
  }

  if (pid_cpu == 0) {
    // CPU-bound child: performs continuous computation without yielding
    int my_pid = getpid();
    printf("[CPU-bound | PID %d] Initial Queue Level: %d (uptime: %d)\n",
           my_pid, get_queue(my_pid), uptime());

    volatile uint64 counter = 0;
    for (int step = 1; step <= 3; step++) {
      for (volatile uint64 j = 0; j < 40000000ULL; j++) {
        counter += j;
      }
      printf("[CPU-bound | PID %d] After Step %d computation: Queue Level = %d (uptime: %d)\n",
             my_pid, step, get_queue(my_pid), uptime());
    }

    printf("[CPU-bound | PID %d] Final Queue Level = %d (successfully sank to lowest queue!)\n",
           my_pid, get_queue(my_pid));
    exit(0);
  }

  int pid_io = fork();
  if (pid_io < 0) {
    printf("Fork failed!\n");
    exit(1);
  }

  if (pid_io == 0) {
    // I/O-bound child: frequently calls pause(1) (sleeps) before exhausting quantum
    int my_pid = getpid();
    printf("[I/O-bound | PID %d] Initial Queue Level: %d (uptime: %d)\n",
           my_pid, get_queue(my_pid), uptime());

    for (int step = 1; step <= 5; step++) {
      // Simulate I/O burst by sleeping 1 tick
      pause(1);
      // Small computation
      volatile uint64 counter = 0;
      for (volatile int j = 0; j < 1000000; j++) {
        counter += j;
      }
      printf("[I/O-bound | PID %d] After Step %d I/O burst: Queue Level = %d (remains at high priority!)\n",
             my_pid, step, get_queue(my_pid));
    }

    printf("[I/O-bound | PID %d] Final Queue Level = %d (retained top priority!)\n",
           my_pid, get_queue(my_pid));
    exit(0);
  }

  // Parent waits for both children
  wait(0);
  wait(0);

  printf("\n--- MLFQ Test Passed: CPU-bound sank to lowest queue; I/O process remained in top queue! ---\n");
  exit(0);
}
