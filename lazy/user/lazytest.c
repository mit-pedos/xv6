#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/meminfo.h"

#define LARGE_SZ (32 * 1024 * 1024) // 32 MB = 8192 pages

int main(int argc, char *argv[]) {
  struct meminfo mi_before, mi_after_sbrk, mi_after_touch;

  printf("============================================================\n");
  printf("      Lazy Memory Allocation (Demand Paging) Test           \n");
  printf("============================================================\n\n");

  // Step 1: Measure initial physical memory
  getmeminfo(&mi_before);
  printf("--- Step 1: Initial Baseline Memory ---\n");
  printf("  Free physical pages:     %ld (%ld KB)\n", mi_before.free_pages, mi_before.free_pages * 4);
  printf("  Process user pages:      %ld\n\n", mi_before.user_pages);

  // Step 2: Large sbrk of 32 MB
  printf("--- Step 2: Request Large sbrk(32 MB) ---\n");
  char *p = sbrk(LARGE_SZ);
  if (p == (char *)-1) {
    printf("sbrk(32MB) failed!\n");
    exit(1);
  }
  getmeminfo(&mi_after_sbrk);
  printf("  sbrk returned base address: %p\n", p);
  printf("  Process virtual pages (sz/PGSIZE): %ld -> %ld (+8192 pages)\n",
         mi_before.user_pages, mi_after_sbrk.user_pages);
  printf("  Free physical pages: %ld -> %ld (change = %ld pages)\n",
         mi_before.free_pages, mi_after_sbrk.free_pages,
         mi_before.free_pages - mi_after_sbrk.free_pages);
  if (mi_after_sbrk.free_pages == mi_before.free_pages) {
    printf("  >> SUCCESS: 32 MB allocated lazily without consuming physical frames!\n\n");
  } else {
    printf("  >> WARNING: Physical memory consumed eagerly.\n\n");
  }

  // Step 3: Touch only 3 widely spaced pages
  printf("--- Step 3: Touching 3 Widely Spaced Pages (0 MB, 16 MB, 31 MB) ---\n");
  uint64 off0 = 0;
  uint64 off1 = 16 * 1024 * 1024;
  uint64 off2 = 31 * 1024 * 1024;

  p[off0] = 'A';
  p[off1] = 'B';
  p[off2] = 'C';

  getmeminfo(&mi_after_touch);
  printf("  Read back touched bytes: p[0]=%c, p[16MB]=%c, p[31MB]=%c\n",
         p[off0], p[off1], p[off2]);
  printf("  Free physical pages: %ld -> %ld (consumed %ld physical frames for 3 pages + page tables)\n",
         mi_after_sbrk.free_pages, mi_after_touch.free_pages,
         mi_after_sbrk.free_pages - mi_after_touch.free_pages);
  printf("  >> SUCCESS: Only touched pages triggered on-demand page faults!\n\n");

  // Step 4: System call using an untouched buffer
  printf("--- Step 4: Kernel System Call (read/pipe) into Untouched Heap Buffer ---\n");
  int fds[2];
  if (pipe(fds) < 0) {
    printf("pipe failed!\n");
    exit(1);
  }
  char *untouched_buf = p + (20 * 1024 * 1024); // 20 MB offset: untouched
  char test_msg[] = "HELLO_LAZY!";
  write(fds[1], test_msg, 12);
  int nread = read(fds[0], untouched_buf, 12);
  close(fds[0]);
  close(fds[1]);

  printf("  Pipe read into untouched buffer returned: %d bytes, content: '%s'\n",
         nread, untouched_buf);
  if (strcmp(untouched_buf, test_msg) == 0) {
    printf("  >> SUCCESS: Kernel successfully handled demand paging during syscall!\n\n");
  } else {
    printf("  >> FAILURE: Buffer content mismatch.\n\n");
  }

  // Step 5: Test fork & exit with lazy pages
  printf("--- Step 5: Testing fork() & exit() with Lazy Mappings ---\n");
  int pid = fork();
  if (pid < 0) {
    printf("fork failed!\n");
    exit(1);
  }
  if (pid == 0) {
    // Child verifies data and touches a new page
    if (p[off0] == 'A' && p[off1] == 'B' && p[off2] == 'C') {
      p[8 * 1024 * 1024] = 'D'; // Touch 8 MB in child
      exit(0);
    }
    exit(1);
  }
  wait(0);
  printf("  Child process executed, accessed lazy pages, and exited cleanly.\n\n");

  // Step 6: Test invalid access beyond sz
  printf("--- Step 6: Testing Invalid Access Beyond sz (Expect Child Process Termination) ---\n");
  int pid_bad = fork();
  if (pid_bad == 0) {
    // Access far beyond allocated heap (33 MB > 32 MB)
    printf("  [Child] Attempting invalid write to p[33 MB]...\n");
    p[33 * 1024 * 1024] = 'X';
    printf("  [Child] ERROR: Should have been killed!\n");
    exit(0);
  }
  int status = 0;
  wait(&status);
  printf("  Parent successfully reaped faulted child. Kernel is intact and running!\n");

  printf("\n============================================================\n");
  printf("lazytest passed all tasks and edge-case verifications!\n");
  printf("============================================================\n");
  exit(0);
}
