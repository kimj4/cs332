#include <stdlib.h>
#include <stdio.h>
#include "tlb.h"
#include "pagetable.h"
#include "global.h" /* for tlb_size */
#include "statistics.h"
#include "process.h"


/*******************************************************************************
 * Looks up an address in the TLB. If no entry is found, calls pagetable_lookup()
 * to get the entry from the page table instead
 *
 * @param vpn The virtual page number to lookup.
 * @param write If the access is a write, this is 1. Otherwise, it is 0.
 * @return The physical frame number of the page we are accessing.
 */
pfn_t tlb_lookup(vpn_t vpn, int write) {
	/* currently just skips tlb_lookup and goes to pagetable */
	pfn_t pfn;
	/* pfn = pagetable_lookup(vpn, write);*/
	/*
	* FIX ME : Step 5
	* Note that tlb is an array with memory already allocated and initialized to 0/null
	* meaning that you don't need special cases for a not-full tlb, the valid field
	* will be 0 for both invalid and empty tlb entries, so you can just check that!
	*/

	/*
	* Search the TLB - hit if find valid entry with given VPN
	* Increment count_tlbhits on hit.
	*/

	tlbe_t *tlb_entry; /* the entry to be returned */
	int tlb_idx;
	int hit = 0;
	for (tlb_idx = 0; tlb_idx < tlb_size; tlb_idx++) {
		pcb_t *test_pcb = rlt[tlb[tlb_idx].vpn].pcb;
		vpn_t test_vpn = tlb[tlb_idx].vpn;
		if ( (tlb[tlb_idx].valid) && (tlb[tlb_idx].vpn == vpn)) {
			count_tlbhits++;
			tlb_entry = &tlb[tlb_idx];

			pfn = tlb_entry->pfn;
			hit = 1;

			tlb_entry->dirty = write;
			tlb_entry->used = 1;
			current_pagetable[vpn].dirty = write;
			current_pagetable[vpn].used = 1;
			break;
		}
	}

	/*
	* If it was a miss, call the page table lookup to get the pfn
	* Add current page as TLB entry. Replace any invalid entry first,
	* then do a clock-sweep to find a victim (entry to be replaced).
	*/
	if (!hit) {
		/* get the entry from ram */
		pfn = pagetable_lookup(vpn, write);
		int replaced = 0;
		/* look for invalid entry */
		for (tlb_idx = 0; tlb_idx < 4; tlb_idx++) {
			if (!tlb[tlb_idx].valid) {
				/* if found, replace the entry */
				tlb_entry = &tlb[tlb_idx];
				tlb_entry->vpn = vpn;
				tlb_entry->pfn = pfn;
				tlb_entry->valid = 1;
				tlb_entry->dirty = write;
				tlb_entry->used = 1;


				replaced = 1;
				current_pagetable[vpn].dirty = write;
				current_pagetable[vpn].used = 1;

				break;
			}
		}

		/* if there were no invalid entries, do a clock sweep.
		   The gatech assignment page says you don't have to keep track of
		   where you left off, so I'll just start from the beginning every
		   time.*/
		if (!replaced) {
			/*printf("DOING A CLOCK SWEEP!\n");*/

			int evicted = 0;
			int cur_idx = 0;
			while (!evicted) {
				if (tlb[cur_idx].used) {
					/* if used recently, unmark it */
					tlb[cur_idx].used = 0;
				} else {
					tlb_entry = &tlb[tlb_idx];

					tlb_entry->vpn = vpn;
					tlb_entry->pfn = pfn;
					tlb_entry->valid = 1;
					tlb_entry->dirty = write;
					tlb_entry->used = 1;
					current_pagetable[vpn].dirty = write;
					current_pagetable[vpn].used = 1;
					evicted = 1;
				}
				cur_idx = (cur_idx + 1) % 4;
			}
		}
	}


	/*
	* In all cases perform TLB house keeping. This means marking the found TLB entry as
	* used and if we had a write, dirty. We also need to update the page
	* table entry in memory with the same data.
	*/
	tlb_entry->used = 1;
	tlb_entry->dirty = write;



   return pfn;
}
