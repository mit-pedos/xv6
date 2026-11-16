#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int global_var = 42;

static void
print_mapping(const char *name, uint64 va)
{
  uint64 pa = vtop(va);
  if (pa == (uint64)-1) {
    printf("[vtop] %s:\n       VA: 0x%lx  ->  PA: -1 (UNMAPPED / INVALID)\n", name, va);
  } else {
    printf("[vtop] %s:\n       VA: 0x%lx  ->  PA: 0x%lx\n", name, va, pa);
  }
}

int
main(int argc, char *argv[])
{
  int stack_var = 100;

  printf("============================================================\n");
  printf("    Virtual-to-Physical Address Translation Test (vtop)     \n");
  printf("============================================================\n\n");

  printf("--- 1. Testing Code and Global Variables ---\n");
  print_mapping("main() function code", (uint64)main);
  print_mapping("global_var (global data)", (uint64)&global_var);
  printf("\n");

  printf("--- 2. Testing Stack Variable ---\n");
  print_mapping("stack_var (local stack variable)", (uint64)&stack_var);
  printf("\n");

  printf("--- 3. Testing Heap Allocation via sbrk() ---\n");
  char *heap_ptr = sbrk(4096);
  if (heap_ptr == (char *)-1) {
    printf("sbrk failed!\n");
    exit(1);
  }
  // Write data to touch the page
  *heap_ptr = 'X';
  *(heap_ptr + 256) = 'Y';
  print_mapping("heap_ptr (allocated heap page base)", (uint64)heap_ptr);
  print_mapping("heap_ptr + 256 (offset within page)", (uint64)(heap_ptr + 256));
  printf("\n");

  printf("--- 4. Testing Unmapped Virtual Addresses ---\n");
  uint64 unmapped_va1 = 0x7FFFFFF0;
  uint64 unmapped_va2 = (uint64)heap_ptr + (1024ULL * 1024ULL * 64ULL); // 64MB above heap
  print_mapping("Unmapped Address 1 (arbitrary high address)", unmapped_va1);
  print_mapping("Unmapped Address 2 (far above heap break)", unmapped_va2);
  printf("\n");

  printf("============================================================\n");
  printf("vttest: All translation tests completed successfully!\n");
  printf("============================================================\n");
  exit(0);
}
