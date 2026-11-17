================================================================================
OPERATING SYSTEMS LAB ASSIGNMENT 7 - QUESTION 2
Process/Thread Synchronization using Counting Semaphores
================================================================================

1. ASSIGNMENT OVERVIEW & OBJECTIVE
--------------------------------------------------------------------------------
- Lab / Question : Lab 7, Question 2 (7b)
- Topic          : Thread Synchronization & Kernel Counting Semaphores
- Test Program   : user/synctest.c
- Shell Command  : $ synctest

Objective:
Implement a full kernel-level counting semaphore subsystem in xv6 to eliminate race
conditions between concurrent threads sharing address space:
1. Kernel Semaphore Subsystem:
   - Provide 16 kernel counting semaphores (`MAX_SEM = 16`).
   - Implement blocking synchronization via xv6's `sleep()` and `wakeup()` primitives.
2. System Calls:
   - `int sem_init(int sem_id, int value)`: Initializes semaphore count.
   - `int sem_wait(int sem_id)`: Decrements count; blocks caller if count <= 0.
   - `int sem_signal(int sem_id)`: Increments count and wakes sleeping threads.
3. Verification:
   - Part 1: Fix Question 1's race condition: 3 threads execute 100,000 increments
     inside a semaphore critical section, achieving exactly 300,000 (0 lost updates).
   - Part 2: Bounded-Buffer Producer-Consumer synchronization using `empty`, `full`,
     and `mutex` semaphores over a 5-slot buffer.


2. THEORETICAL BACKGROUND & OS PRINCIPLES
--------------------------------------------------------------------------------
1. Semaphore vs. Spinlock vs. Mutex:
   - Spinlock (Busy-Waiting): Continuously polls a memory word using atomic instructions
     (e.g., `amoswap`). Wastes CPU cycles; cannot be held across `sleep()`. Ideal for
     short kernel critical sections.
   - Counting Semaphore (Sleeping Lock): Non-negative integer counter. If a thread
     requests a resource when none is available, the thread relinquishes the CPU and
     transitions to `SLEEPING` state until awakened by `sem_signal()`.
2. Eliminating Lost Updates:
   - A non-atomic operation like `counter++` compiles into three CPU instructions:
       `lw a5, 0(s0)`     (Load)
       `addi a5, a5, 1`   (Modify)
       `sw a5, 0(s0)`     (Store)
   - When multiple threads execute this concurrently without synchronization, context
     switches between load and store cause lost updates. Enclosing the sequence in
     `sem_wait` and `sem_signal` ensures atomicity.


3. SUMMARY OF MODIFIED & ADDED FILES
--------------------------------------------------------------------------------
File                  | Nature   | Purpose / Changes
----------------------+----------+----------------------------------------------
kernel/proc.h         | Modified | Retained int is_thread for thread identification.
kernel/defs.h         | Modified | Declared kclone, kjoin, seminit, ksem_init,
                      |          | ksem_wait, ksem_signal.
kernel/main.c         | Modified | Added seminit() invocation during boot.
kernel/proc.c         | Modified | Retained uvmshare(), kclone(), kjoin() from 7a.
kernel/sysproc.c      | Modified | Implemented semaphore data structures and syscall
                      |          | handlers: sys_sem_init, sys_sem_wait, sys_sem_signal.
kernel/syscall.h      | Modified | Registered SYS_clone, SYS_join, SYS_sem_init,
                      |          | SYS_sem_wait, SYS_sem_signal.
kernel/syscall.c      | Modified | Wired syscall table.
user/user.h           | Modified | Exported semaphore prototypes to user API.
user/usys.pl          | Modified | Added syscall stubs.
Makefile              | Modified | Set CPUS := 1; cleared DETFLAGS; added $U/_synctest.
user/synctest.c       | Created  | Comprehensive test harness for synchronized counter
                      |          | (Part 1) and bounded buffer producer-consumer (Part 2).


4. DETAILED CODE WALKTHROUGH
--------------------------------------------------------------------------------
a) Kernel Semaphore Structure (kernel/sysproc.c):
   ```c
   struct semaphore {
     struct spinlock lk;  // Protects semaphore value
     int value;           // Current resource count
     int active;          // 1 if allocated, 0 if unused
   } sems[16];
   ```

b) Implementation of Operations:
   - `ksem_wait(int sem_id)`:
     ```c
     acquire(&sems[sem_id].lk);
     while (sems[sem_id].value <= 0) {
       sleep(&sems[sem_id], &sems[sem_id].lk);
     }
     sems[sem_id].value--;
     release(&sems[sem_id].lk);
     ```
   - `ksem_signal(int sem_id)`:
     ```c
     acquire(&sems[sem_id].lk);
     sems[sem_id].value++;
     wakeup(&sems[sem_id]);
     release(&sems[sem_id].lk);
     ```


5. SYSTEM CALL SPECIFICATIONS
--------------------------------------------------------------------------------
1. int sem_init(int sem_id, int value)
   - Parameters: `sem_id` (0..15), `value` (initial count >= 0).
   - Return Value: 0 on success, -1 on invalid argument.
2. int sem_wait(int sem_id)
   - Decrements count; blocks if count <= 0. Return 0 on success.
3. int sem_signal(int sem_id)
   - Increments count; wakes sleeping threads. Return 0 on success.


6. HOW TO COMPILE AND RUN
--------------------------------------------------------------------------------
Step 1: Open a terminal and navigate to this folder:
        cd /home/udit/Desktop/xv6/sync

Step 2: Clean and compile xv6, then launch QEMU:
        make clean && make qemu

Step 3: Inside the xv6 shell, run the test program:
        $ synctest

Step 4: Exit QEMU:
        Press Ctrl-A, release both keys, then press X.


7. VERIFIED TEST EXECUTION OUTPUT & ANALYSIS
--------------------------------------------------------------------------------
Output Log:
--------------------------------------------------------------------------------
$ synctest
============================================================
      xv6 Process/Thread Synchronization using Semaphores   
============================================================
============================================================
 PART 1: Synchronized Threads Counter Test using Semaphores 
 Threads: 3, Increments: 100000 each, Expected: 300000            
============================================================
Main: Cloned Thread 1 (pid 4)
Main: Cloned Thread 2 (pid 5)
Main: Cloned Thread 3 (pid 6)
Main: Releasing all 3 threads concurrently...
Thread 1 (pid 4) finished 100000 synchronized increments.
Main: Reaped thread with pid 4 via join()
Thread 2 (pid 5) finished 100000 synchronized increments.
Main: Reaped thread with pid 5 via join()
Thread 3 (pid 6) finished 100000 synchronized increments.
Main: Reaped thread with pid 6 via join()
------------------------------------------------------------
Part 1 Results: Expected = 300000, Actual = 300000
SUCCESS: Counter reached EXACTLY 300000 with 0 lost updates!
Semaphores successfully eliminated the race condition.

============================================================
 PART 2: Classic Producer-Consumer Bounded Buffer Test      
 Buffer Size = 5, Total Items = 15                         
 Semaphores: Empty (init 5), Full (init 0), Mutex (init 1)  
============================================================
[Producer (pid 7)]: Produced 1 | Buffer count: 1/5
[Consumer (pid 8)]: Consumed 1 | Buffer count: 0/5
...
[Producer (pid 7)]: Produced 15 | Buffer count: 5/5
[Consumer (pid 8)]: Consumed 15 | Buffer count: 0/5
Consumer: All 15 items verified in strict FIFO order!
------------------------------------------------------------
Part 2 Results: Peak Buffer Usage = 5/5 slots, Final Count = 0
SUCCESS: No buffer overflow or underflow occurred.
Producer-Consumer synchronization fully verified!

============================================================
All synchronization tests completed successfully!
============================================================

Analysis:
- In Part 1, all 300,000 increments were completely preserved with 0 lost updates,
  resolving the 66% loss observed in Question 1 without synchronization.
- In Part 2, peak buffer usage was strictly bounded at 5, and items were consumed
  in FIFO sequence.


8. FREQUENTLY ASKED VIVA / LAB EXAM QUESTIONS & ANSWERS
--------------------------------------------------------------------------------
Q1: Why does `ksem_wait()` take a spinlock internally?
A1: To protect the `value` variable and prevent a race condition between checking
    `value <= 0` and calling `sleep()`. The internal spinlock is passed to `sleep()`,
    which atomically releases it while placing the process on the sleep channel.

Q2: What is the difference between `sem_signal` and `wakeup`?
A2: `sem_signal` is the user-facing semaphore operation that increments the resource
    counter and calls `wakeup()`. `wakeup()` is the internal xv6 kernel scheduling
    primitive that transitions processes sleeping on a channel address (`chan`)
    to the `RUNNABLE` state.

Q3: Can a semaphore have a negative value in this implementation?
A3: No. In this classical Dijkstra implementation, the value never drops below 0
    because threads block in a `while (value <= 0)` loop before the decrement occurs.

Q4: Why does Part 1 succeed with exactly 300,000 while Question 1 resulted in 100,000?
A4: Because `sem_wait` and `sem_signal` enforce mutual exclusion: only one thread at
    a time can execute the read-modify-write instructions of `counter++`, preventing
    interleaved overwrite of registers.
================================================================================
