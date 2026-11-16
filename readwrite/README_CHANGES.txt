================================================================================
Operating Systems Lab Assignment - 5
Question 3: Readers-Writers Problem using Semaphores
================================================================================

1. OVERVIEW & POLICY
--------------------
This question implements a solution to the classic Readers-Writers problem using
kernel-level counting semaphores and a shared memory region in xv6.

Policy Implemented: Readers-Preference
- Any reader process can enter the critical section to read as long as no writer is
  currently writing.
- Multiple readers are permitted to read concurrently.
- The first reader to enter acquires the exclusive writer lock (wrt semaphore), blocking
  any writers.
- Subsequent readers increment read_count and enter immediately without blocking on wrt.
- The last reader to leave releases the exclusive writer lock (wrt semaphore), allowing
  a waiting writer to proceed.
- Writers require strict mutual exclusion: a writer can only enter when read_count == 0
  and no other writer is active.

2. SYNCHRONIZATION PRIMITIVES & SHARED STATE
--------------------------------------------
- Shared Memory:
  Mapped at virtual address SHM_VA = (TRAPFRAME - PGSIZE) = 0x3FFFFFFE000.
  Holds struct rw_shared:
    - shared_data: The integer variable modified by writers and inspected by readers.
    - read_count: Integer tracking current number of active readers.
    - active_writers: Tracks number of active writers in the critical section (must be 0 or 1).
    - max_readers_seen: Peak number of concurrent readers observed.
    - total_writes: Total successful write operations completed.
    - violations: Counter incremented if a reader detects an active writer or if a writer
      detects active readers / multiple writers.

- Kernel Semaphores:
  1. SEM_MUTEX (ID 0): Binary semaphore (initialized to 1) protecting read_count updates.
  2. SEM_WRT   (ID 1): Binary semaphore (initialized to 1) providing exclusive access to writers
                       and mutual exclusion between the reader group and writers.

3. FILES MODIFIED / ADDED
-------------------------
- kernel/memlayout.h:
  * Defined SHM_VA as (TRAPFRAME - PGSIZE).
- kernel/defs.h:
  * Added declarations for walk(), shminit(), shm_get(), seminit(), ksem_init(),
    ksem_wait(), and ksem_post().
- kernel/main.c:
  * Initialized shared memory subsystem (shminit()) and semaphore table (seminit()).
- kernel/proc.c:
  * Unmapped SHM_VA in proc_freepagetable() without freeing the shared physical frame.
  * Propagated SHM_VA mapping to child processes during kfork().
- kernel/sysproc.c:
  * Implemented shminit(), shm_get(), sys_shm_get().
  * Implemented semaphore table and syscalls: seminit(), ksem_init(), ksem_wait(),
    ksem_post(), sys_sem_init(), sys_sem_wait(), sys_sem_post().
- kernel/syscall.h & kernel/syscall.c:
  * Registered SYS_shm_get (23), SYS_sem_init (24), SYS_sem_wait (25), SYS_sem_post (26).
- user/user.h & user/usys.pl:
  * Exported shm_get(), sem_init(), sem_wait(), sem_post() to user space.
- user/readwrite.c:
  * Test driver spawning 3 reader processes and 2 writer processes.
  * Includes assertions verifying no simultaneous reader-writer or writer-writer execution.
- Makefile:
  * Added $U/_readwrite to UPROGS.
  * Cleared DETFLAGS to prevent compiler errors on paths with spaces.
  * Set CPUS := 1.

4. HOW TO BUILD AND RUN
-----------------------
Inside this directory:
    $ make clean
    $ make qemu

Once xv6 boots into the shell, run:
    $ readwrite

To exit QEMU:
    Press Ctrl+A, then X.

5. SAMPLE OUTPUT VERIFICATION
-----------------------------
$ readwrite
============================================================
Readers-Writers Problem Simulation (Readers-Preference)
Readers = 3, Writers = 2
Reads per reader = 5, Writes per writer = 5
============================================================
Reader 1 started (pid 4)
Reader 1 [pid 4]: read shared_data = 0 | active readers = 1
Reader 2 started (pid 5)
Reader 3 started (pid 6)
Reader 3 [pid 6]: read shared_data = 0 | active readers = 3
Writer 1 started (pid 7)
Writer 2 started (pid 8)
...
Writer 1 [pid 7]: wrote shared_data = 1 | active readers = 0, active writers = 1
Writer 2 [pid 8]: wrote shared_data = 2 | active readers = 0, active writers = 1
...
Writer 2 [pid 8]: wrote shared_data = 10 | active readers = 0, active writers = 1
Writer 1 [pid 7] finished all 5 writes.
Writer 2 [pid 8] finished all 5 writes.
============================================================
Simulation Finished Summary:
Final shared_data value: 10 (Expected: 10)
Total writes recorded: 10
Max concurrent readers observed: 3
Synchronization violations: 0
SUCCESS:
  1. Multiple readers read simultaneously (max = 3).
  2. Writers had exclusive access (read_count = 0 and active_writers = 1).
  3. No lost updates or race conditions occurred.
============================================================
