#include "statistics.h"

#define MEMORY_ACCESS_TIME      100 /* 100 ns */
#define DISK_ACCESS_TIME   10000000 /* 10 million ns = 10 ms */

/*	Available variables from statistics that you may find useful
 *    count_pagefaults   - the number of page faults that occurred (includes required)
 *    count_tlbhits      - the number of tlbhits that occurred
 *    count_writes       - the number of stores/writes that occurred
 *    count_reads        - the number of reads that occurred
 * 	  disk_accesses		 - the number of disk accesses NOT including required faults
 * Any other values you might need are composites of the above values.
 */

double compute_emat_all() {
   /* FIX ME - Compute the average memory access time, including required page faults
    * that occur when loading a new process.
    */

    /* memory accesses include one for tlb hits and at least 2 for a page fault
      also the number of writes and reads all take one access */
    int num_mem_accesses = count_tlbhits + 2 * count_pagefaults + count_writes + count_reads;
    /* disk access is just the number of page faults doubled (design of simulator) */
    int num_disk_accesses = count_pagefaults * 2;

    int total_time = num_mem_accesses * MEMORY_ACCESS_TIME + num_disk_accesses * DISK_ACCESS_TIME;

    int total_accesses = count_writes + count_reads;

    int emat = total_time / total_accesses;

    return emat;
}

double compute_emat_unforced() {
   /* FIX ME - Compute the average memory access time NOT including required faults
    */

    /* memory accesses include one for tlb hits and at least 2 for a page fault
      also the number of writes and reads all take one access */
    int num_mem_accesses = count_tlbhits + 2 * count_pagefaults + count_writes + count_reads;

    int num_disk_accesses = count_diskaccesses;
    printf("count_diskaccesses: %li\n", count_diskaccesses);

    int total_time = num_mem_accesses * MEMORY_ACCESS_TIME + num_disk_accesses * DISK_ACCESS_TIME;

    int total_accesses = count_writes + count_reads;

    int emat = total_time / total_accesses;

    return emat;

	return 0;
}
