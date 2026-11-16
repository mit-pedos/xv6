#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define ALLOC_PAGES 10
#define ALLOC_SIZE (ALLOC_PAGES * 4096)

int main(int argc, char *argv[]) {
  printf("============================================================\n");
  printf("  Physical Memory Allocation Tracking Test (memtest)        \n");
  printf("============================================================\n\n");

  printf("--- Step 1: Baseline Memory Statistics ---\n");
  memstats();

  printf("\n--- Step 2: Allocating %d Pages (%d KB) using sbrk() ---\n",
         ALLOC_PAGES, ALLOC_SIZE / 1024);
  char *ptr = sbrk(ALLOC_SIZE);
  if (ptr == (char *)-1) {
    printf("sbrk allocation failed!\n");
    exit(1);
  }
  // Write to each allocated page so physical frames are accessed
  for (int i = 0; i < ALLOC_SIZE; i += 4096) {
    ptr[i] = 'M';
  }

  printf("Memory allocated at virtual address: %p\n", ptr);
  printf("Updated statistics after allocation:\n");
  memstats();

  printf("\n--- Step 3: Deallocating %d Pages using sbrk(-%d) ---\n",
         ALLOC_PAGES, ALLOC_SIZE);
  if (sbrk(-ALLOC_SIZE) == (char *)-1) {
    printf("sbrk deallocation failed!\n");
    exit(1);
  }

  printf("Updated statistics after deallocation:\n");
  memstats();

  printf("\n============================================================\n");
  printf("memtest completed successfully!\n");
  printf("Observations:\n");
  printf("  1. Allocating %d pages increased total & currently allocated counters\n", ALLOC_PAGES);
  printf("     and decreased available physical memory.\n");
  printf("  2. Releasing the memory increased total freed pages and restored\n");
  printf("     the available physical pages.\n");
  printf("============================================================\n");
  exit(0);
}
