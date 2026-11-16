#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Shared memory implementation
static char *global_shm_page = 0;
static struct spinlock global_shm_lock;

void
shminit(void)
{
  initlock(&global_shm_lock, "shm");
}

uint64
shm_get(void)
{
  struct proc *p = myproc();

  acquire(&global_shm_lock);
  if (global_shm_page == 0) {
    global_shm_page = kalloc();
    if (global_shm_page == 0) {
      release(&global_shm_lock);
      return 0;
    }
    memset(global_shm_page, 0, PGSIZE);
  }

  pte_t *pte = walk(p->pagetable, SHM_VA, 0);
  if (pte == 0 || (*pte & PTE_V) == 0) {
    if (mappages(p->pagetable, SHM_VA, PGSIZE, (uint64)global_shm_page, PTE_R | PTE_W | PTE_U) != 0) {
      release(&global_shm_lock);
      return 0;
    }
  }
  release(&global_shm_lock);
  return SHM_VA;
}

uint64
sys_shm_get(void)
{
  return shm_get();
}

// Semaphore implementation
#define MAX_SEM 32

struct semaphore {
  struct spinlock lock;
  int value;
  int active;
};

static struct semaphore sems[MAX_SEM];

void
seminit(void)
{
  for (int i = 0; i < MAX_SEM; i++) {
    initlock(&sems[i].lock, "sem");
    sems[i].value = 0;
    sems[i].active = 0;
  }
}

int
ksem_init(int id, int val)
{
  if (id < 0 || id >= MAX_SEM)
    return -1;
  acquire(&sems[id].lock);
  sems[id].value = val;
  sems[id].active = 1;
  release(&sems[id].lock);
  return 0;
}

int
ksem_wait(int id)
{
  if (id < 0 || id >= MAX_SEM)
    return -1;
  acquire(&sems[id].lock);
  while (sems[id].value <= 0) {
    sleep_prepare(&sems[id]);
    release(&sems[id].lock);
    sleep();
    acquire(&sems[id].lock);
  }
  sems[id].value--;
  release(&sems[id].lock);
  return 0;
}

int
ksem_post(int id)
{
  if (id < 0 || id >= MAX_SEM)
    return -1;
  acquire(&sems[id].lock);
  sems[id].value++;
  wakeup(&sems[id]);
  release(&sems[id].lock);
  return 0;
}

uint64
sys_sem_init(void)
{
  int id, val;
  argint(0, &id);
  argint(1, &val);
  return ksem_init(id, val);
}

uint64
sys_sem_wait(void)
{
  int id;
  argint(0, &id);
  return ksem_wait(id);
}

uint64
sys_sem_post(void)
{
  int id;
  argint(0, &id);
  return ksem_post(id);
}
