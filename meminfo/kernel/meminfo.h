#ifndef _MEMINFO_H_
#define _MEMINFO_H_

struct meminfo {
  uint64 total_pages;      // Total physical pages available in system
  uint64 free_pages;       // Free physical pages in freelist
  uint64 user_pages;       // Pages in user address space (sz / PGSIZE)
  uint64 pagetable_pages;  // Pages used by process page tables
};

#endif
