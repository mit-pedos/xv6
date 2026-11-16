#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define REF_LEN 20
#define MAX_FRAMES 10

static const int ref_string[REF_LEN] = {
  7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
};

// Helper to print a percentage with two decimal places in xv6
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

// Print contents of frames in aligned format
static void
print_frame_state(int step, int page, int frames[], int num_frames, const char *status)
{
  printf(" %d\t| %d\t| ", step + 1, page);
  for (int i = 0; i < num_frames; i++) {
    if (frames[i] == -1)
      printf(" - ");
    else
      printf(" %d ", frames[i]);
  }
  printf("\t| %s\n", status);
}

// ============================================================================
// 1. First-In, First-Out (FIFO) Simulation
// ============================================================================
static void
sim_fifo(int num_frames, int verbose, int *out_faults, int *out_hits)
{
  int frames[MAX_FRAMES];
  int fifo_queue[MAX_FRAMES];
  int queue_head = 0, queue_tail = 0;
  int faults = 0, hits = 0;

  for (int i = 0; i < num_frames; i++) {
    frames[i] = -1;
    fifo_queue[i] = -1;
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf(" FIFO Simulation (Frames: %d)\n", num_frames);
    printf("------------------------------------------------------------\n");
    printf(" Step\t| Ref\t| Frame State\t| Status\n");
    printf("------------------------------------------------------------\n");
  }

  for (int i = 0; i < REF_LEN; i++) {
    int p = ref_string[i];
    int found = 0;

    // Check for page hit
    for (int j = 0; j < num_frames; j++) {
      if (frames[j] == p) {
        found = 1;
        break;
      }
    }

    if (found) {
      hits++;
      if (verbose)
        print_frame_state(i, p, frames, num_frames, "HIT ");
    } else {
      faults++;
      // Check for an empty slot
      int empty_idx = -1;
      for (int j = 0; j < num_frames; j++) {
        if (frames[j] == -1) {
          empty_idx = j;
          break;
        }
      }

      if (empty_idx != -1) {
        frames[empty_idx] = p;
        fifo_queue[queue_tail] = empty_idx;
        queue_tail = (queue_tail + 1) % num_frames;
      } else {
        // Evict oldest page according to FIFO queue
        int victim_frame = fifo_queue[queue_head];
        queue_head = (queue_head + 1) % num_frames;
        frames[victim_frame] = p;
        fifo_queue[queue_tail] = victim_frame;
        queue_tail = (queue_tail + 1) % num_frames;
      }

      if (verbose)
        print_frame_state(i, p, frames, num_frames, "FAULT");
    }
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf("FIFO Summary (%d frames): Page Faults = %d, Page Hits = %d\n", num_frames, faults, hits);
    printf("Hit Ratio: ");
    print_percentage(hits, REF_LEN);
    printf(" | Fault Ratio: ");
    print_percentage(faults, REF_LEN);
    printf("\n\n");
  }

  *out_faults = faults;
  *out_hits = hits;
}

// ============================================================================
// 2. Least Recently Used (LRU) Simulation
// ============================================================================
static void
sim_lru(int num_frames, int verbose, int *out_faults, int *out_hits)
{
  int frames[MAX_FRAMES];
  int last_used[MAX_FRAMES];
  int faults = 0, hits = 0;

  for (int i = 0; i < num_frames; i++) {
    frames[i] = -1;
    last_used[i] = -1;
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf(" LRU Simulation (Frames: %d)\n", num_frames);
    printf("------------------------------------------------------------\n");
    printf(" Step\t| Ref\t| Frame State\t| Status\n");
    printf("------------------------------------------------------------\n");
  }

  for (int i = 0; i < REF_LEN; i++) {
    int p = ref_string[i];
    int found_idx = -1;

    for (int j = 0; j < num_frames; j++) {
      if (frames[j] == p) {
        found_idx = j;
        break;
      }
    }

    if (found_idx != -1) {
      hits++;
      last_used[found_idx] = i; // Update recency
      if (verbose)
        print_frame_state(i, p, frames, num_frames, "HIT ");
    } else {
      faults++;
      int empty_idx = -1;
      for (int j = 0; j < num_frames; j++) {
        if (frames[j] == -1) {
          empty_idx = j;
          break;
        }
      }

      if (empty_idx != -1) {
        frames[empty_idx] = p;
        last_used[empty_idx] = i;
      } else {
        // Find frame with minimum last_used timestamp
        int lru_idx = 0;
        int min_time = last_used[0];
        for (int j = 1; j < num_frames; j++) {
          if (last_used[j] < min_time) {
            min_time = last_used[j];
            lru_idx = j;
          }
        }
        frames[lru_idx] = p;
        last_used[lru_idx] = i;
      }

      if (verbose)
        print_frame_state(i, p, frames, num_frames, "FAULT");
    }
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf("LRU Summary (%d frames): Page Faults = %d, Page Hits = %d\n", num_frames, faults, hits);
    printf("Hit Ratio: ");
    print_percentage(hits, REF_LEN);
    printf(" | Fault Ratio: ");
    print_percentage(faults, REF_LEN);
    printf("\n\n");
  }

  *out_faults = faults;
  *out_hits = hits;
}

// ============================================================================
// 3. Optimal Page Replacement (OPT) Simulation
// ============================================================================
static void
sim_opt(int num_frames, int verbose, int *out_faults, int *out_hits)
{
  int frames[MAX_FRAMES];
  int faults = 0, hits = 0;

  for (int i = 0; i < num_frames; i++) {
    frames[i] = -1;
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf(" Optimal (OPT) Simulation (Frames: %d)\n", num_frames);
    printf("------------------------------------------------------------\n");
    printf(" Step\t| Ref\t| Frame State\t| Status\n");
    printf("------------------------------------------------------------\n");
  }

  for (int i = 0; i < REF_LEN; i++) {
    int p = ref_string[i];
    int found = 0;

    for (int j = 0; j < num_frames; j++) {
      if (frames[j] == p) {
        found = 1;
        break;
      }
    }

    if (found) {
      hits++;
      if (verbose)
        print_frame_state(i, p, frames, num_frames, "HIT ");
    } else {
      faults++;
      int empty_idx = -1;
      for (int j = 0; j < num_frames; j++) {
        if (frames[j] == -1) {
          empty_idx = j;
          break;
        }
      }

      if (empty_idx != -1) {
        frames[empty_idx] = p;
      } else {
        // Look ahead in future reference string
        int farthest_use = -1;
        int victim_frame = 0;

        for (int j = 0; j < num_frames; j++) {
          int page_in_frame = frames[j];
          int next_use = -1;

          for (int k = i + 1; k < REF_LEN; k++) {
            if (ref_string[k] == page_in_frame) {
              next_use = k;
              break;
            }
          }

          // If this page is never used again in future, it's the ideal victim
          if (next_use == -1) {
            victim_frame = j;
            break;
          }

          if (next_use > farthest_use) {
            farthest_use = next_use;
            victim_frame = j;
          }
        }

        frames[victim_frame] = p;
      }

      if (verbose)
        print_frame_state(i, p, frames, num_frames, "FAULT");
    }
  }

  if (verbose) {
    printf("------------------------------------------------------------\n");
    printf("OPT Summary (%d frames): Page Faults = %d, Page Hits = %d\n", num_frames, faults, hits);
    printf("Hit Ratio: ");
    print_percentage(hits, REF_LEN);
    printf(" | Fault Ratio: ");
    print_percentage(faults, REF_LEN);
    printf("\n\n");
  }

  *out_faults = faults;
  *out_hits = hits;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int
main(int argc, char *argv[])
{
  int faults, hits;

  printf("======================================================================\n");
  printf("     Page Replacement Algorithms Simulation (FIFO, LRU, Optimal)      \n");
  printf("======================================================================\n\n");

  printf("Reference String (%d page requests):\n ", REF_LEN);
  for (int i = 0; i < REF_LEN; i++) {
    printf("%d ", ref_string[i]);
  }
  printf("\n\n");

  printf("======================================================================\n");
  printf("PART 1: Step-by-Step Simulation with 3 Physical Frames\n");
  printf("======================================================================\n\n");

  sim_fifo(3, 1, &faults, &hits);
  sim_lru(3, 1, &faults, &hits);
  sim_opt(3, 1, &faults, &hits);

  printf("======================================================================\n");
  printf("PART 2: Multi-Frame Performance Comparison (3, 4, and 5 Frames)\n");
  printf("======================================================================\n\n");

  printf("+-----------+--------+-------------+-----------+-----------+------------+\n");
  printf("| Algorithm | Frames | Page Faults | Page Hits | Hit Ratio | Fault Ratio|\n");
  printf("+-----------+--------+-------------+-----------+-----------+------------+\n");

  int frame_counts[] = {3, 4, 5};
  for (int f = 0; f < 3; f++) {
    int fc = frame_counts[f];

    // FIFO
    sim_fifo(fc, 0, &faults, &hits);
    printf("| FIFO      |   %d    |     %d      |    %d     |  ", fc, faults, hits);
    print_percentage(hits, REF_LEN);
    printf("   |   ");
    print_percentage(faults, REF_LEN);
    printf("  |\n");

    // LRU
    sim_lru(fc, 0, &faults, &hits);
    printf("| LRU       |   %d    |     %d      |    %d     |  ", fc, faults, hits);
    print_percentage(hits, REF_LEN);
    printf("   |   ");
    print_percentage(faults, REF_LEN);
    printf("  |\n");

    // OPT
    sim_opt(fc, 0, &faults, &hits);
    printf("| OPT       |   %d    |     %d      |    %d     |  ", fc, faults, hits);
    print_percentage(hits, REF_LEN);
    printf("   |   ");
    print_percentage(faults, REF_LEN);
    printf("  |\n");

    printf("+-----------+--------+-------------+-----------+-----------+------------+\n");
  }

  printf("\n");
  printf("======================================================================\n");
  printf("PART 3: Algorithm Performance Analysis and Conclusion\n");
  printf("======================================================================\n");
  printf("1. Optimal (OPT):\n");
  printf("   - Achieved the lowest page faults across all frame sizes:\n");
  printf("     9 faults (3 frames), 8 faults (4 frames), and 7 faults (5 frames).\n");
  printf("   - Why: OPT looks ahead and replaces the page that will not be used\n");
  printf("     for the longest duration in the future. It is provably optimal and\n");
  printf("     serves as the theoretical upper bound for hit ratio.\n\n");

  printf("2. Least Recently Used (LRU):\n");
  printf("   - Achieved 12 faults (3 frames), 8 faults (4 frames), and 7 faults (5 frames).\n");
  printf("   - Why: LRU leverages the Principle of Locality (specifically Temporal\n");
  printf("     Locality) by approximating future behavior with past recency.\n");
  printf("     At 4 and 5 frames, LRU matched the performance of OPT exactly!\n\n");

  printf("3. First-In, First-Out (FIFO):\n");
  printf("   - Suffered the highest page faults:\n");
  printf("     15 faults (3 frames), 10 faults (4 frames), and 9 faults (5 frames).\n");
  printf("   - Why: FIFO only considers the arrival order without accounting for\n");
  printf("     frequency or recency of page accesses, evicting heavily used pages.\n\n");

  printf("Conclusion:\n");
  printf("  -> Optimal (OPT) performed best overall in minimizing page faults.\n");
  printf("  -> Among implementable online algorithms, LRU significantly outperformed\n");
  printf("     FIFO, achieving up to 65.00%% hit ratio.\n");
  printf("======================================================================\n");

  exit(0);
}
