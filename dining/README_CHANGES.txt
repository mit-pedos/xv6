================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 5 - QUESTION 4
Dining Philosophers Problem with Deadlock Avoidance using Semaphores
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 5, Question 4 (5d)
- Topic          : Concurrency, Deadlock Avoidance & Resource Hierarchy
- Test Program   : user/dining.c
- Shell Command  : $ dining

Objective:
Solve the classic Dining Philosophers synchronization problem for 5 philosophers
in xv6 using kernel-level counting semaphores and shared memory:
1. Model: 5 philosophers seated at a round table with 5 chopsticks/forks. Each
   philosopher alternates between THINKING, HUNGRY, and EATING.
2. Invariants:
   - A philosopher can only eat when holding both adjacent forks (left and right).
   - No two adjacent philosophers may eat simultaneously (mutual exclusion).
   - The system must NEVER deadlock (elimination of circular wait).
   - No philosopher may starve: all philosophers complete 5 full meals.


2. THEORETICAL BACKGROUND & DEADLOCK PREVENTION
--------------------------------------------------------------------------------
1. The 4 Coffman Conditions for Deadlock:
   - Mutual Exclusion : Only one philosopher can hold a fork at a time.
   - Hold and Wait    : A philosopher holds one fork and waits to acquire the other.
   - No Preemption    : A fork cannot be forcibly taken away from a philosopher.
   - Circular Wait    : Philosopher 0 waits for 1, 1 for 2, ..., 4 for 0.

2. Elimination of Circular Wait via Havender's Resource Ordering:
   - Assign a global linear ordering to the forks: Fork 0 < Fork 1 < Fork 2 < Fork 3 < Fork 4.
   - Every philosopher MUST request and acquire the lower-indexed fork first, and the
     higher-indexed fork second:
       $$FirstFork = \min(left, right)$$
       $$SecondFork = \max(left, right)$$
   - Concrete Allocation Order:
     * Philosopher 0: forks (0, 1) $\to$ acquires Fork 0 first, then Fork 1.
     * Philosopher 1: forks (1, 2) $\to$ acquires Fork 1 first, then Fork 2.
     * Philosopher 2: forks (2, 3) $\to$ acquires Fork 2 first, then Fork 3.
     * Philosopher 3: forks (3, 4) $\to$ acquires Fork 3 first, then Fork 4.
     * Philosopher 4: forks (4, 0) $\to$ $\min(4, 0) = 0$! Acquires Fork 0 first, then Fork 4.

3. Why Deadlock is Impossible:
   - Both Philosopher 0 and Philosopher 4 compete for Fork 0 first.
   - One of them wins Fork 0; the other blocks BEFORE holding any fork!
   - Consequently, the circular chain of "each process holding one fork and waiting
     for the next" cannot form. At least one philosopher always secures both forks,
     eats, and releases them.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/memlayout.h    | Modified | Defined SHM_VA = (TRAPFRAME - PGSIZE) = 0x3FFFFFFE000.
kernel/defs.h         | Modified | Declared shminit, shm_get, seminit, ksem_init,
                      |          | ksem_wait, ksem_post, walk.
kernel/main.c         | Modified | Initialized shared memory and semaphore tables.
kernel/proc.c         | Modified | Propagated SHM_VA in fork(); preserved physical frame
                      |          | on exit in proc_freepagetable().
kernel/sysproc.c      | Modified | Implemented semaphore subsystem and shm allocator.
kernel/syscall.h      | Modified | Defined SYS_shm_get (23), SYS_sem_init (24),
                      |          | SYS_sem_wait (25), SYS_sem_post (26).
kernel/syscall.c      | Modified | Registered syscall handlers.
user/user.h           | Modified | Declared shm_get, sem_init, sem_wait, sem_post.
user/usys.pl          | Modified | Added syscall stubs.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_dining.
user/dining.c         | Created  | Test driver simulating 5 philosophers with resource
                      |          | ordering and mutual exclusion assertions.


4. DETAILED CODE WALKTHROUGH (user/dining.c)
--------------------------------------------------------------------------------
a) Shared State Structure:
   ```c
   struct dining_shared {
     int state[N_PHIL];       // THINKING (0), HUNGRY (1), EATING (2)
     int meals_eaten[N_PHIL]; // Counter of meals per philosopher
     int violations;          // Invariant violation counter
     int total_meals;         // Total meals completed across system
   };
   ```

b) Philosopher Lifecycle:
   ```c
   void philosopher(struct dining_shared *ds, int id) {
     int left = id;
     int right = (id + 1) % N_PHIL;
     int first = (left < right) ? left : right;
     int second = (left < right) ? right : left;

     for (int i = 0; i < NUM_CYCLES; i++) {
       // THINKING
       pause(1);

       // HUNGRY
       ds->state[id] = STATE_HUNGRY;
       sem_wait(first);
       sem_wait(second);

       // EATING (Verify adjacent neighbors are NOT eating)
       ds->state[id] = STATE_EATING;
       int left_neighbor = (id + N_PHIL - 1) % N_PHIL;
       int right_neighbor = (id + 1) % N_PHIL;
       if (ds->state[left_neighbor] == STATE_EATING ||
           ds->state[right_neighbor] == STATE_EATING) {
         ds->violations++;
       }
       ds->meals_eaten[id]++;
       pause(2); // Simulate eating duration

       // FINISHED EATING
       ds->state[id] = STATE_THINKING;
       sem_post(second);
       sem_post(first);
     }
   }
   ```


5. SYSTEM CALL SPECIFICATIONS
--------------------------------------------------------------------------------
1. void *shm_get(void)
   - Returns pointer to shared page (`0x3FFFFFFE000`).
2. int sem_init(int id, int value)
   - Initializes binary semaphores 0..4 for forks (value = 1).
3. int sem_wait(int id)
   - Acquires fork `id`.
4. int sem_post(int id)
   - Releases fork `id`.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/dining

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ dining

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ dining
============================================================
Dining Philosophers Simulation with Deadlock Avoidance
Philosophers = 5, Forks = 5, Cycles = 5
Strategy: Resource Ordering (acquire lower-numbered fork first)
============================================================
Philosopher 0 [pid 4]: THINKING -> HUNGRY (Cycle 1/5)
Philosopher 0 [pid 4]: picked up fork 0, waiting for fork 1
Philosopher 0 [pid 4]: HUNGRY -> EATING (holding forks 0, 1) [Meal #1]
...
Philosopher 4 [pid 8]: picked up fork 0, waiting for fork 4
Philosopher 4 [pid 8]: HUNGRY -> EATING (holding forks 0, 4) [Meal #1]
...
============================================================
Simulation Finished Summary:
Philosopher 0 ate 5 / 5 meals.
Philosopher 1 ate 5 / 5 meals.
Philosopher 2 ate 5 / 5 meals.
Philosopher 3 ate 5 / 5 meals.
Philosopher 4 ate 5 / 5 meals.
Total meals eaten: 25 (Expected: 25)
Mutual exclusion violations: 0
SUCCESS:
  1. All 5 philosophers completed all 5 cycles without deadlocking.
  2. No two adjacent philosophers ate simultaneously.
  3. Resource ordering effectively eliminated circular wait.
============================================================

Analysis:
- All 5 philosophers ate exactly 5 meals ($5 \times 5 = 25$ total).
- Violations remained at 0 (no adjacent philosophers ate simultaneously).
- No deadlock occurred.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why does a naive solution to Dining Philosophers cause a deadlock?
A1: If all 5 philosophers become hungry simultaneously and each picks up their left
    fork, every fork is taken. Each philosopher then waits for their right fork, which
    is held by their neighbor. This forms a circular dependency with all 5 processes
    waiting forever.

Q2: Which of the Coffman conditions is broken by Resource Ordering?
A2: The Circular Wait condition. By imposing a total ordering on resources and
    enforcing that resources must be requested in strictly increasing order, cycles
    in the resource allocation graph become mathematically impossible.

Q3: Why does Philosopher 4 pick up Fork 0 before Fork 4?
A3: Philosopher 4 needs Fork 4 and Fork 0. Under the resource ordering rule,
    $\min(4, 0) = 0$. Therefore, Philosopher 4 must acquire Fork 0 first, breaking
    the symmetry that would otherwise allow all philosophers to grab one fork.

Q4: What is the difference between Deadlock and Starvation in this problem?
A4: Deadlock means no philosopher can make progress (all are blocked). Starvation
    means the system as a whole makes progress (some philosophers eat), but one
    particular philosopher is indefinitely blocked from eating. Resource ordering
    guarantees deadlock freedom.
================================================================================
