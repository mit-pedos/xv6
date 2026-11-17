================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 5 - QUESTION 3
Readers-Writers Problem using Semaphores (Reader-Preference)
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 5, Question 3 (5c)
- Topic          : Process Concurrency & Readers-Writers Synchronization
- Test Program   : user/readwrite.c
- Shell Command  : $ readwrite

Objective:
Implement a solution to the classic Readers-Writers synchronization problem using
kernel-level counting semaphores and a shared memory region in xv6:
1. Enforce Reader-Preference Policy:
   - Multiple reader processes may read shared data concurrently without blocking
     one another.
   - Writer processes require strictly exclusive access: when a writer writes,
     no other writer and no readers may be inside the critical section.
2. Invariants & Correctness Guarantees:
   - Simultaneous Readers: $ActiveReaders \ge 0$ (observed peak = 3).
   - Exclusive Writer: When $ActiveWriters == 1$, $ActiveReaders == 0$.
   - Zero Violations: No concurrent read-write or write-write collisions occur.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Courtois First Readers-Writers Formulation (Reader-Preference):
   - Two semaphores:
     * `SEM_MUTEX` (initialized to 1): Protects the shared counter `read_count`.
     * `SEM_WRT`   (initialized to 1): Grants exclusive access to the writer, and
       serves as a mutual-exclusion barrier between the reader group and writers.
   - Shared Variable:
     * `read_count`: Tracks the number of readers currently in the critical section.

2. Reader Protocol:
   ```c
   sem_wait(SEM_MUTEX);
   read_count++;
   if (read_count == 1) {
     sem_wait(SEM_WRT); // First reader locks out writers
   }
   sem_post(SEM_MUTEX);

   // Reading critical section (concurrent readers allowed)
   read(shared_data);

   sem_wait(SEM_MUTEX);
   read_count--;
   if (read_count == 0) {
     sem_post(SEM_WRT); // Last reader unlocks writers
   }
   sem_post(SEM_MUTEX);
   ```

3. Writer Protocol:
   ```c
   sem_wait(SEM_WRT);   // Acquire exclusive writer lock

   // Writing critical section (exclusive access)
   write(shared_data);

   sem_post(SEM_WRT);   // Release writer lock
   ```

4. Writer Starvation Trade-Off:
   Because new readers can join the critical section as long as at least one reader
   is active (`read_count > 0`), a continuous stream of arriving readers can delay
   waiting writers indefinitely (Writer Starvation). Solutions like Writer-Preference
   or Fair (FIFO) semaphores resolve this in production operating systems.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/memlayout.h    | Modified | Defined SHM_VA = (TRAPFRAME - PGSIZE) = 0x3FFFFFFE000.
kernel/defs.h         | Modified | Added declarations for shminit, shm_get, seminit,
                      |          | ksem_init, ksem_wait, ksem_post, walk.
kernel/main.c         | Modified | Initialized shared memory and semaphore tables.
kernel/proc.c         | Modified | Propagated SHM_VA in fork(); preserved physical
                      |          | frame on exit in proc_freepagetable().
kernel/sysproc.c      | Modified | Implemented kernel semaphore subsystem and shm.
kernel/syscall.h      | Modified | Defined SYS_shm_get (23), SYS_sem_init (24),
                      |          | SYS_sem_wait (25), SYS_sem_post (26).
kernel/syscall.c      | Modified | Registered syscall handlers.
user/user.h           | Modified | Declared shm_get, sem_init, sem_wait, sem_post.
user/usys.pl          | Modified | Added syscall stubs.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_readwrite.
user/readwrite.c      | Created  | Test driver running 3 concurrent readers and
                      |          | 2 concurrent writers with invariant assertions.


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) Shared State Structure (user/readwrite.c):
   ```c
   struct rw_shared {
     int shared_data;       // Data variable incremented by writers
     int read_count;        // Active reader counter
     int active_readers;    // Verification counter of readers inside CS
     int active_writers;    // Verification counter of writers inside CS (0 or 1)
     int max_readers_seen;  // Peak concurrent readers observed
     int total_writes;      // Total write operations completed
     int violations;        // Safety assertion failure counter
   };
   ```

b) Invariant Checking:
   - When any writer enters, it verifies `active_readers == 0` and `active_writers == 0`.
     If violated, `violations++`.
   - When any reader enters, it verifies `active_writers == 0`. If violated, `violations++`.


5. SYSTEM CALL SPECIFICATIONS
--------------------------------------------------------------------------------
1. void *shm_get(void)
   - Returns pointer to shared physical page (`0x3FFFFFFE000`).
2. int sem_init(int id, int value)
   - Initializes semaphore `id` with `value`.
3. int sem_wait(int id)
   - Decrements semaphore `id`, blocking if `value <= 0`.
4. int sem_post(int id)
   - Increments semaphore `id` and wakes waiting processes.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/readwrite

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ readwrite

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
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

Analysis:
- Up to 3 readers accessed `shared_data` simultaneously (`max_readers_seen = 3`).
- All 10 write operations executed with strictly `active_readers == 0` and
  `active_writers == 1`.
- Violations remained at 0, confirming mutual exclusion between reader group and writers.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why does only the first reader call `sem_wait(SEM_WRT)`?
A1: The first reader acts as the representative for all readers. It locks out
    writers. Subsequent readers simply increment `read_count` and enter without
    contending on `SEM_WRT`.

Q2: Why does only the last reader call `sem_post(SEM_WRT)`?
A2: As long as at least one reader remains active, writers must remain locked out.
    Only when the final reader exits (`read_count == 0`) is it safe to signal
    `SEM_WRT` to allow a waiting writer to proceed.

Q3: What is the purpose of `SEM_MUTEX` in the reader protocol?
A3: `SEM_MUTEX` protects the critical section that reads and modifies `read_count`.
    Without `SEM_MUTEX`, concurrent readers could interleave updates to `read_count`,
    potentially causing two readers to both think they are the first reader, or
    failing to signal `SEM_WRT` on exit.

Q4: What is the primary drawback of Reader-Preference?
A4: Writer starvation. If new readers continuously arrive and increment `read_count`
    before existing readers leave, `read_count` never drops to 0, and waiting writers
    are starved indefinitely.
================================================================================
