#include "swapfile.h"
#include "statistics.h"
#include "pagetable.h"
#include "page-splitting.h"

/*******************************************************************************
 * Looks up an address in the current page table. If the entry for the given
 * page is not valid, traps to the OS.
 *
 * @param vpn The virtual page number to lookup.
 * @param write If the access is a write, this is 1. Otherwise, it is 0.
 * @return The physical frame number of the page we are accessing.
 */
pfn_t pagetable_lookup(vpn_t vpn, int write) {
    pfn_t pfn = 0;

    pte_t entry = current_pagetable[vpn];
    if (entry.valid) {
        pfn = entry.pfn;
    } else {
        count_pagefaults++;
        entry.pfn = pagefault_handler(vpn, write);
        entry.valid = 1;

        pfn = entry.pfn;
    }
    return pfn;
}
