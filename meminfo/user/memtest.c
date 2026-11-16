#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/meminfo.h"

void print_meminfo(const char *label, struct meminfo *mi) {
  printf("[%s]\n", label);
  printf("  Total System Pages:      %ld (%ld KB)\n", mi->total_pages, mi->total_pages * 4);
  printf("  Free Physical Pages:     %ld (%ld KB)\n", mi->free_pages, mi->free_pages * 4);
  printf("  Process Address Pages:   %ld (%ld KB, sz/PGSIZE)\n", mi->user_pages, mi->user_pages * 4);
  printf("  Process Page-Table Pages:%ld (%ld KB)\n", mi->pagetable_pages, mi->pagetable_pages * 4);
}

int main(int argc, char *argv[]) {
  struct meminfo mi_baseline, mi_allocated, mi_child, mi_after_fork, mi_after_free;

  printf("============================================================\n");
  printf("         Physical Memory Statistics Test (memtest)          \n");
  printf("============================================================\n\n");

  // Step 1: Baseline memory state
  if (getmeminfo(&mi_baseline) < 0) {
    printf("getmeminfo failed!\n");
    exit(1);
  }
  print_meminfo("1. Initial Baseline", &mi_baseline);

  // Step 2: Grow heap by 4 pages (16384 bytes)
  printf("\n--- Step 2: Allocate 4 Pages (16 KB) with sbrk(4 * 4096) ---\n");
  char *p = sbrk(4 * 4096);
  if (p == (char *)-1) {
    printf("sbrk failed!\n");
    exit(1);
  }
  // Touch pages so physical allocation is active
  for (int i = 0; i < 4 * 4096; i += 4096)
    p[i] = 'A';

  getmeminfo(&mi_allocated);
  print_meminfo("2. After Heap Expansion (sbrk +4 pages)", &mi_allocated);
  printf("  -> Free pages change: %ld -> %ld (decreased by %ld)\n",
         mi_baseline.free_pages, mi_allocated.free_pages,
         mi_baseline.free_pages - mi_allocated.free_pages);
  printf("  -> Process user pages: %ld -> %ld (increased by %ld)\n",
         mi_baseline.user_pages, mi_allocated.user_pages,
         mi_allocated.user_pages - mi_baseline.user_pages);

  // Step 3: Fork a child process
  printf("\n--- Step 3: Forking a Child Process ---\n");
  int pid = fork();
  if (pid < 0) {
    printf("Fork failed!\n");
    exit(1);
  }

  if (pid == 0) {
    // Child process prints its memory state
    getmeminfo(&mi_child);
    print_meminfo("3a. Inside Child Process", &mi_child);
    printf("  -> Free pages dropped further due to child address space and page tables.\n");
    exit(0);
  } else {
    // Parent waits for child to exit and reaps it
    wait(0);
    getmeminfo(&mi_after_fork);
    print_meminfo("3b. Parent After Reaping Child", &mi_after_fork);

    // Verify zero memory leak from fork/exit
    printf("\n--- Verification: Memory Leak Check After Child Termination ---\n");
    printf("  Free pages before fork: %ld\n", mi_allocated.free_pages);
    printf("  Free pages after reap:  %ld\n", mi_after_fork.free_pages);
    if (mi_after_fork.free_pages == mi_allocated.free_pages) {
      printf("  >> SUCCESS: Exact match! All child frames and page tables reaped (NO LEAK).\n");
    } else {
      printf("  >> WARNING: Discrepancy of %ld pages.\n",
             mi_allocated.free_pages - mi_after_fork.free_pages);
    }
  }

  // Step 4: Deallocate heap memory with sbrk(-4 * 4096)
  printf("\n--- Step 4: Shrink Heap with sbrk(-4 * 4096) ---\n");
  sbrk(-4 * 4096);
  getmeminfo(&mi_after_free);
  print_meminfo("4. After Heap Shrinking", &mi_after_free);
  printf("  -> Free pages returned from %ld to %ld (restored %ld pages)\n",
         mi_after_fork.free_pages, mi_after_free.free_pages,
         mi_after_free.free_pages - mi_after_fork.free_pages);

  printf("\n============================================================\n");
  printf("memtest completed all tasks successfully!\n");
  printf("============================================================\n");
  exit(0);
}
