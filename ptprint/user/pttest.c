#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("============================================================\n");
  printf("         Page Table Printing Test Program (pttest)          \n");
  printf("============================================================\n\n");

  printf("--- 1. Calling vmprint() at initial start-up ---\n");
  int before_pages = vmprint();
  printf("Initial valid user pages reported by vmprint: %d\n\n", before_pages);

  printf("--- 2. Allocating 4 pages via sbrk(4 * 4096 = 16384 bytes) ---\n");
  char *p = sbrk(4 * 4096);
  if (p == (char *)-1) {
    printf("sbrk failed!\n");
    exit(1);
  }
  printf("sbrk allocated memory starting at: 0x%lx\n", (uint64)p);

  // Write to each of the 4 pages
  for (int i = 0; i < 4; i++) {
    p[i * 4096] = 'A' + i;
  }
  printf("Touched all 4 newly allocated pages.\n\n");

  printf("--- 3. Calling vmprint() after sbrk(4 * 4096) ---\n");
  int after_pages = vmprint();
  printf("New valid user pages reported by vmprint: %d\n\n", after_pages);

  printf("--- 4. Verification Analysis ---\n");
  printf("Initial user pages : %d\n", before_pages);
  printf("After sbrk(+4)     : %d\n", after_pages);
  printf("Difference         : %d pages\n", after_pages - before_pages);

  if (after_pages - before_pages == 4) {
    printf("SUCCESS: Exactly four new pages appeared in the page table!\n");
  } else {
    printf("FAILURE: Expected 4 new pages, got %d\n", after_pages - before_pages);
  }

  printf("============================================================\n");
  exit(0);
}
