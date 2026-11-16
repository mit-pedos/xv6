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

// Semaphore Subsystem
#define MAX_SEM 16

struct semaphore {
  struct spinlock lk;
  int value;
  int active;
};

static struct semaphore sems[MAX_SEM];

void
seminit(void)
{
  for (int i = 0; i < MAX_SEM; i++) {
    initlock(&sems[i].lk, "sem");
    sems[i].value = 0;
    sems[i].active = 0;
  }
}

int
ksem_init(int id, int val)
{
  if (id < 0 || id >= MAX_SEM)
    return -1;
  acquire(&sems[id].lk);
  sems[id].value = val;
  sems[id].active = 1;
  release(&sems[id].lk);
  return 0;
}

int
ksem_wait(int id)
{
  if (id < 0 || id >= MAX_SEM)
    return -1;
  acquire(&sems[id].lk);
  while (sems[id].value <= 0) {
    sleep_prepare(&sems[id]);
    release(&sems[id].lk);
    sleep();
    acquire(&sems[id].lk);
  }
  sems[id].value--;
  release(&sems[id].lk);
  return 0;
}

int
ksem_signal(int id)
{
  if (id < 0 || id >= MAX_SEM)
    return -1;
  acquire(&sems[id].lk);
  sems[id].value++;
  wakeup(&sems[id]);
  release(&sems[id].lk);
  return 0;
}

uint64
sys_clone(void)
{
  uint64 fcn, arg, stack;
  argaddr(0, &fcn);
  argaddr(1, &arg);
  argaddr(2, &stack);
  return kclone(fcn, arg, stack);
}

uint64
sys_join(void)
{
  return kjoin();
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
sys_sem_signal(void)
{
  int id;
  argint(0, &id);
  return ksem_signal(id);
}
