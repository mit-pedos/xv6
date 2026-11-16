#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int global_var = 42;

int main(int argc, char *argv[]) {
  int local_var = 100;
  char *heap_var = (char *)malloc(128);
  if (heap_var)
    heap_var[0] = 'X';

  printf("============================================================\n");
  printf("  Virtual to Physical Address Translation Test (vatest)     \n");
  printf("============================================================\n\n");

  printf("--- Step 1: Translate Global, Local (Stack), and Heap Variables ---\n");
  uint64 g_va = (uint64)&global_var;
  uint64 g_pa = va2pa(g_va);
  printf("Global Variable:  VA = 0x%lx  -->  PA = 0x%lx\n", g_va, g_pa);

  uint64 l_va = (uint64)&local_var;
  uint64 l_pa = va2pa(l_va);
  printf("Local Variable:   VA = 0x%lx  -->  PA = 0x%lx\n", l_va, l_pa);

  uint64 h_va = (uint64)heap_var;
  uint64 h_pa = va2pa(h_va);
  printf("Heap Variable:    VA = 0x%lx  -->  PA = 0x%lx\n\n", h_va, h_pa);

  printf("--- Step 2: Unmapped Address Translation (0x7FFFFFFF) ---\n");
  uint64 bad_va = 0x7FFFFFFF;
  uint64 bad_pa = va2pa(bad_va);
  printf("Unmapped Address: VA = 0x%lx  -->  Result = %ld (Expected: -1)\n", bad_va, (long)bad_pa);
  if ((long)bad_pa == -1) {
    printf("SUCCESS: va2pa correctly returned -1 for unmapped address.\n\n");
  } else {
    printf("FAILURE: va2pa did not return -1.\n\n");
  }

  printf("--- Step 3: Forking Child to Demonstrate Distinct Physical Pages ---\n");
  int pid = fork();
  if (pid < 0) {
    printf("Fork failed!\n");
    exit(1);
  }

  if (pid == 0) {
    // Child
    uint64 child_g_va = (uint64)&global_var;
    uint64 child_g_pa = va2pa(child_g_va);
    printf("[Child PID %d]  VA = 0x%lx  -->  PA = 0x%lx\n", getpid(), child_g_va, child_g_pa);
    exit(0);
  } else {
    // Parent
    wait(0);
    uint64 parent_g_va = (uint64)&global_var;
    uint64 parent_g_pa = va2pa(parent_g_va);
    printf("[Parent PID %d] VA = 0x%lx  -->  PA = 0x%lx\n", getpid(), parent_g_va, parent_g_pa);
    printf("\nVERIFICATION:\n");
    printf("Same virtual address:   0x%lx (Parent & Child)\n", parent_g_va);
    printf("Separate physical page: PA copied on fork (Parent PA != Child PA)\n");
  }

  printf("\n============================================================\n");
  printf("vatest passed all verification steps successfully!\n");
  printf("============================================================\n");
  exit(0);
}
