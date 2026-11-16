#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("============================================================\n");
  printf("  Page Table Management & Page Protection Test (pagetest)   \n");
  printf("============================================================\n\n");

  printf("--- 1. Allocating a physical page using sbrk(4096) ---\n");
  char *p = sbrk(4096);
  if (p == (char *)-1) {
    printf("sbrk(4096) failed!\n");
    exit(1);
  }
  printf("Allocated page at Virtual Address: 0x%lx\n\n", (uint64)p);

  printf("--- 2. Writing initial data to the allocated page ---\n");
  *p = 'H';
  *(p + 1) = 'I';
  *(p + 2) = '\0';
  printf("Successfully wrote \"%s\" to page at 0x%lx\n\n", p, (uint64)p);

  printf("--- 3. Checking pageinfo BEFORE setreadonly ---\n");
  if (pageinfo((uint64)p) < 0) {
    printf("pageinfo failed!\n");
    exit(1);
  }
  printf("Expected: Present=1, Writable=1, User=1\n\n");

  printf("--- 4. Invoking setreadonly() to mark page READ-ONLY ---\n");
  if (setreadonly((uint64)p) < 0) {
    printf("setreadonly failed!\n");
    exit(1);
  }
  printf("setreadonly() returned success.\n\n");

  printf("--- 5. Checking pageinfo AFTER setreadonly ---\n");
  if (pageinfo((uint64)p) < 0) {
    printf("pageinfo failed!\n");
    exit(1);
  }
  printf("Expected: Present=1, Writable=0, User=1 (Write permission removed!)\n\n");

  printf("--- 6. Verifying READ operations still succeed ---\n");
  printf("Read value from read-only page: '%c%c'\n", *p, *(p + 1));
  printf("Read verification passed!\n\n");

  printf("--- 7. Attempting WRITE to read-only page in a child process ---\n");
  int pid = fork();
  if (pid < 0) {
    printf("fork failed!\n");
    exit(1);
  }

  if (pid == 0) {
    printf("[Child pid %d] Attempting to write 'X' to read-only address 0x%lx...\n", getpid(), (uint64)p);
    // This write MUST trigger a store page fault exception in hardware MMU!
    *p = 'X';
    // If execution reaches here, protection failed!
    printf("[Child pid %d] ERROR: Write succeeded on read-only page!\n", getpid());
    exit(0);
  } else {
    int status = 0;
    wait(&status);
    printf("[Parent] Child process terminated with status %d.\n", status);
    if (status != 0) {
      printf("[Parent] SUCCESS: Child process was killed by kernel due to page fault exception!\n");
    } else {
      printf("[Parent] FAILED: Child process exited normally without page fault.\n");
    }
  }
  printf("\n");

  printf("--- 8. Testing unmapped address error handling ---\n");
  uint64 unmapped = 0x7FFFFFF0;
  printf("Calling pageinfo on unmapped address 0x%lx:\n", unmapped);
  int r1 = pageinfo(unmapped);
  printf("pageinfo returned: %d (expected -1)\n", r1);

  printf("Calling setreadonly on unmapped address 0x%lx:\n", unmapped);
  int r2 = setreadonly(unmapped);
  printf("setreadonly returned: %d (expected -1)\n\n", r2);

  printf("============================================================\n");
  printf("pagetest: All page protection tests completed successfully!\n");
  printf("============================================================\n");
  exit(0);
}
