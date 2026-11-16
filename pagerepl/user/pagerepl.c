#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_REFS 64
#define MAX_FRAMES 16

static const int default_ref_string[] = {
  7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
};
#define DEFAULT_REF_LEN (sizeof(default_ref_string) / sizeof(default_ref_string[0]))

// Format percentage with two decimal places
static void
print_percentage(int num, int total)
{
  int whole = (num * 100) / total;
  int frac = ((num * 10000) / total) % 100;
  if (frac < 10)
    printf("%d.0%d%%", whole, frac);
  else
    printf("%d.%d%%", whole, frac);
}

// Print row of frames and status
static void
print_row(int step, int page, int frames[], int num_frames, const char *status)
{
  printf("  %d\t|   %d\t| [", step + 1, page);
  for (int i = 0; i < num_frames; i++) {
    if (frames[i] == -1)
      printf(" - ");
    else
      printf(" %d ", frames[i]);
  }
  printf("]\t| %s\n", status);
}

// Simulate FIFO
static void
run_fifo(const int refs[], int nrefs, int nframes, int verbose, int *out_faults, int *out_hits)
{
  int frames[MAX_FRAMES];
  int fifo_queue[MAX_FRAMES];
  int head = 0, tail = 0;
  int faults = 0, hits = 0;

  for (int i = 0; i < nframes; i++) {
    frames[i] = -1;
    fifo_queue[i] = -1;
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf(" FIFO Simulation (Frames: %d)\n", nframes);
    printf("------------------------------------------------------------\n");
    printf("  Step\t|  Page\t|  Frame Contents\t| Status\n");
    printf("------------------------------------------------------------\n");
  }

  for (int i = 0; i < nrefs; i++) {
    int p = refs[i];
    int hit = 0;

    for (int j = 0; j < nframes; j++) {
      if (frames[j] == p) {
        hit = 1;
        break;
      }
    }

    if (hit) {
      hits++;
      if (verbose)
        print_row(i, p, frames, nframes, "HIT ");
    } else {
      faults++;
      int empty_idx = -1;
      for (int j = 0; j < nframes; j++) {
        if (frames[j] == -1) {
          empty_idx = j;
          break;
        }
      }

      if (empty_idx != -1) {
        frames[empty_idx] = p;
        fifo_queue[tail] = empty_idx;
        tail = (tail + 1) % nframes;
      } else {
        int victim_slot = fifo_queue[head];
        head = (head + 1) % nframes;
        frames[victim_slot] = p;
        fifo_queue[tail] = victim_slot;
        tail = (tail + 1) % nframes;
      }

      if (verbose)
        print_row(i, p, frames, nframes, "FAULT");
    }
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf("FIFO Result (%d frames): Faults = %d, Hits = %d, Hit Ratio = ",
           nframes, faults, hits);
    print_percentage(hits, nrefs);
    printf("\n\n");
  }

  *out_faults = faults;
  *out_hits = hits;
}

// Simulate LRU
static void
run_lru(const int refs[], int nrefs, int nframes, int verbose, int *out_faults, int *out_hits)
{
  int frames[MAX_FRAMES];
  int last_access[MAX_FRAMES];
  int faults = 0, hits = 0;

  for (int i = 0; i < nframes; i++) {
    frames[i] = -1;
    last_access[i] = -1;
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf(" LRU Simulation (Frames: %d)\n", nframes);
    printf("------------------------------------------------------------\n");
    printf("  Step\t|  Page\t|  Frame Contents\t| Status\n");
    printf("------------------------------------------------------------\n");
  }

  for (int i = 0; i < nrefs; i++) {
    int p = refs[i];
    int hit_idx = -1;

    for (int j = 0; j < nframes; j++) {
      if (frames[j] == p) {
        hit_idx = j;
        break;
      }
    }

    if (hit_idx != -1) {
      hits++;
      last_access[hit_idx] = i;
      if (verbose)
        print_row(i, p, frames, nframes, "HIT ");
    } else {
      faults++;
      int empty_idx = -1;
      for (int j = 0; j < nframes; j++) {
        if (frames[j] == -1) {
          empty_idx = j;
          break;
        }
      }

      if (empty_idx != -1) {
        frames[empty_idx] = p;
        last_access[empty_idx] = i;
      } else {
        int lru_slot = 0;
        int min_time = last_access[0];
        for (int j = 1; j < nframes; j++) {
          if (last_access[j] < min_time) {
            min_time = last_access[j];
            lru_slot = j;
          }
        }
        frames[lru_slot] = p;
        last_access[lru_slot] = i;
      }

      if (verbose)
        print_row(i, p, frames, nframes, "FAULT");
    }
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf("LRU Result (%d frames): Faults = %d, Hits = %d, Hit Ratio = ",
           nframes, faults, hits);
    print_percentage(hits, nrefs);
    printf("\n\n");
  }

  *out_faults = faults;
  *out_hits = hits;
}

int
main(int argc, char *argv[])
{
  int custom_refs[MAX_REFS];
  const int *refs = default_ref_string;
  int nrefs = DEFAULT_REF_LEN;
  int target_frames = 3;

  if (argc > 1) {
    target_frames = atoi(argv[1]);
    if (target_frames <= 0 || target_frames > MAX_FRAMES) {
      printf("Usage: pagerepl [num_frames] [page1 page2 ...]\n");
      exit(1);
    }
    if (argc == 3 && strchr(argv[2], ' ')) {
      // Parse space-separated numbers in a single quoted string
      char *s = argv[2];
      nrefs = 0;
      while (*s && nrefs < MAX_REFS) {
        while (*s == ' ') s++;
        if (!*s) break;
        custom_refs[nrefs++] = atoi(s);
        while (*s && *s != ' ') s++;
      }
      refs = custom_refs;
    } else if (argc > 2) {
      nrefs = argc - 2;
      if (nrefs > MAX_REFS)
        nrefs = MAX_REFS;
      for (int i = 0; i < nrefs; i++) {
        custom_refs[i] = atoi(argv[i + 2]);
      }
      refs = custom_refs;
    }
  }


  printf("============================================================\n");
  printf("     Page Replacement Algorithms Simulation (FIFO vs LRU)  \n");
  printf("============================================================\n\n");

  printf("Reference String (%d references):\n  ", nrefs);
  for (int i = 0; i < nrefs; i++) {
    printf("%d ", refs[i]);
  }
  printf("\n\n");

  int fifo_faults = 0, fifo_hits = 0;
  int lru_faults = 0, lru_hits = 0;

  printf("============================================================\n");
  printf("TASK B: Testing with %d Frames (Step-by-Step Trace)\n", target_frames);
  printf("============================================================\n\n");

  run_fifo(refs, nrefs, target_frames, 1, &fifo_faults, &fifo_hits);
  run_lru(refs, nrefs, target_frames, 1, &lru_faults, &lru_hits);

  if (target_frames == 3 && refs == default_ref_string) {
    printf("--- Verification Against Expected Results (3 Frames) ---\n");
    printf("FIFO Faults: %d (Expected: 15) -> %s\n",
           fifo_faults, (fifo_faults == 15) ? "VERIFIED" : "FAILED");
    printf("LRU  Faults: %d (Expected: 12) -> %s\n\n",
           lru_faults, (lru_faults == 12) ? "VERIFIED" : "FAILED");
  }

  printf("============================================================\n");
  printf("TASK C: Performance Comparison Across 3, 4, and 5 Frames\n");
  printf("============================================================\n\n");

  printf("+-----------+--------+-------------+-----------+-----------+\n");
  printf("| Algorithm | Frames | Page Faults | Page Hits | Hit Ratio |\n");
  printf("+-----------+--------+-------------+-----------+-----------+\n");

  int frame_tests[] = {3, 4, 5};
  for (int idx = 0; idx < 3; idx++) {
    int f = frame_tests[idx];
    int f_faults, f_hits;

    run_fifo(refs, nrefs, f, 0, &f_faults, &f_hits);
    printf("| FIFO      |   %d    |     %d      |    %d     |   ", f, f_faults, f_hits);
    print_percentage(f_hits, nrefs);
    printf("   |\n");

    run_lru(refs, nrefs, f, 0, &f_faults, &f_hits);
    printf("| LRU       |   %d    |     %d      |    %d     |   ", f, f_faults, f_hits);
    print_percentage(f_hits, nrefs);
    printf("   |\n");

    printf("+-----------+--------+-------------+-----------+-----------+\n");
  }

  printf("\n============================================================\n");
  printf("pagerepl: Simulation finished successfully!\n");
  printf("============================================================\n");

  exit(0);
}
