// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint64 total_allocated;  // Total pages ever allocated by kalloc()
  uint64 total_freed;      // Total pages freed by kfree() after initialization
  uint64 available_pages;  // Currently available physical pages in freelist
  int initialized;         // Set after kinit finishes initial freerange
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.total_allocated = 0;
  kmem.total_freed = 0;
  kmem.available_pages = 0;
  kmem.initialized = 0;
  freerange(end, (void *)PHYSTOP);
  kmem.initialized = 1;
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.available_pages++;
  if (kmem.initialized) {
    kmem.total_freed++;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r) {
    kmem.freelist = r->next;
    kmem.available_pages--;
    if (kmem.initialized) {
      kmem.total_allocated++;
    }
  }
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}

// System call helper to display memory allocation statistics
int
memstats(void)
{
  uint64 tot_alloc, tot_freed, curr_alloc, avail;

  acquire(&kmem.lock);
  tot_alloc = kmem.total_allocated;
  tot_freed = kmem.total_freed;
  curr_alloc = kmem.total_allocated - kmem.total_freed;
  avail = kmem.available_pages;
  release(&kmem.lock);

  printf("\n--- Physical Memory Allocation Statistics ---\n");
  printf("  Total Allocated Pages:     %ld (%ld KB)\n", tot_alloc, tot_alloc * 4);
  printf("  Total Freed Pages:         %ld (%ld KB)\n", tot_freed, tot_freed * 4);
  printf("  Currently Allocated Pages: %ld (%ld KB)\n", curr_alloc, curr_alloc * 4);
  printf("  Available Physical Pages:  %ld (%ld KB)\n", avail, avail * 4);
  printf("---------------------------------------------\n");
  return 0;
}
