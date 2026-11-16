#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("============================================================\n");
  printf("        Lazy Memory Allocation Test (faulttest)             \n");
  printf("============================================================\n\n");

  printf("--- 1. Baseline Page Fault Count ---\n");
  int f0 = getfaults();
  printf("Initial handled page faults: %d\n\n", f0);

  printf("--- 2. Requesting 32 pages via sbrk(32 * 4096 = 131072 bytes) ---\n");
  char *p = sbrk(32 * 4096);
  if (p == (char *)-1) {
    printf("sbrk failed!\n");
    exit(1);
  }
  printf("sbrk returned base address: 0x%lx\n", (uint64)p);

  int f_after_sbrk = getfaults();
  printf("Handled page faults immediately after sbrk: %d\n", f_after_sbrk);
  if (f_after_sbrk == f0) {
    printf("PASSED: No physical pages allocated yet (lazy allocation active!)\n\n");
  } else {
    printf("FAILED: Expected 0 new page faults, got %d\n\n", f_after_sbrk - f0);
  }

  printf("--- 3. Writing to exactly 5 distinct pages ---\n");
  int page_indices[5] = {0, 4, 11, 19, 31};
  for (int i = 0; i < 5; i++) {
    int idx = page_indices[i];
    p[idx * 4096] = 'A' + i;
    printf("  Touched page index %d (VA 0x%lx) with '%c'\n",
           idx, (uint64)&p[idx * 4096], 'A' + i);
  }
  printf("\n");

  printf("--- 4. Checking Page Fault Count via getfaults() ---\n");
  int f_after_writes = getfaults();
  int new_faults = f_after_writes - f0;
  printf("Total handled page faults: %d (New faults: %d)\n", f_after_writes, new_faults);

  if (new_faults == 5) {
    printf("SUCCESS: Exactly 5 page faults were handled by the kernel!\n\n");
  } else {
    printf("FAILED: Expected 5 page faults, got %d\n\n", new_faults);
  }

  printf("--- 5. Testing Access Above Allocated Memory Size ---\n");
  int pid = fork();
  if (pid < 0) {
    printf("fork failed!\n");
    exit(1);
  }

  if (pid == 0) {
    // Child process accesses address above process size
    char *bad_addr = p + (35 * 4096);
    printf("[Child pid %d] Attempting to access address above p->sz (VA 0x%lx)...\n",
           getpid(), (uint64)bad_addr);
    *bad_addr = 'Z'; // Must trigger unhandled page fault and kill process
    printf("[Child pid %d] ERROR: Out-of-bounds access succeeded unexpectedly!\n", getpid());
    exit(0);
  } else {
    int status = 0;
    wait(&status);
    printf("[Parent] Child process terminated with status %d.\n", status);
    if (status != 0) {
      printf("[Parent] SUCCESS: Only the offending child was killed by the kernel!\n");
    } else {
      printf("[Parent] FAILED: Child process exited normally without being killed.\n");
    }
  }

  printf("\n============================================================\n");
  printf("faulttest: All lazy allocation tests completed successfully!\n");
  printf("============================================================\n");
  exit(0);
}
