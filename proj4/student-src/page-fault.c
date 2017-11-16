#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "process.h"
#include "global.h"
#include "swapfile.h"

/*******************************************************************************
 * Page fault handler. When the CPU encounters an invalid address mapping in a
 * process' page table, it invokes the CPU via this handler. The OS then
 * allocates a physical frame for the requested page (either by using a free
 * frame or evicting one), changes the process' page table to reflect the
 * mapping and then restarts the interrupted process.
 *
 * @param vpn The virtual page number requested from the current process.
 * @param write If the CPU is writing to the page, this is 1. Otherwise, it's 0.
 * @return The physical frame the OS has mapped to the virtual page.
 */
pfn_t pagefault_handler(vpn_t request_vpn, int write) {
  pfn_t victim_pfn;
  vpn_t victim_vpn;
  pcb_t *victim_pcb;

  /* Sanity Check */
  assert(current_pagetable != NULL);

  /* Find a free frame */
  victim_pfn = get_free_frame();
  assert(victim_pfn < CPU_NUM_FRAMES); /* make sure the victim_pfn is valid */

  /* Use the reverse lookup table to find the victim. */
  victim_vpn = rlt[victim_pfn].vpn;
  victim_pcb = rlt[victim_pfn].pcb;

  /*
   * FIX ME : Problem 4
   * If victim page is occupied - if it is not the pcb will be NULL:
   *
   * 1) If it's dirty, save it to disk with page_to_disk()
   * 2) Invalidate the page's entry in the victim's page table.
   * 3) Clear the victim page's TLB entry using the function tlb_clearone().
   */



   if (victim_pcb) {
       /*printf("\nthere's something to evict!\n\n");*/
       pte_t victim_pte = victim_pcb->pagetable[victim_vpn];

       if (victim_pte.dirty) {
           printf("=============page dirty, writing to disk\n");
           page_to_disk(victim_pfn, victim_vpn, victim_pcb->pid);
           victim_pte.dirty = 0;
       }
       victim_pte.valid = 0;
       tlb_clearone(victim_vpn);
   }

  printf("PAGE FAULT (VPN %u), evicting (PFN %u VPN %u)\n", request_vpn,
      victim_pfn, victim_vpn);


  /* FIX ME */
  /* Update the reverse lookup table to replace the victim entry info with this
   * process' info instead (pcb and vpn)
   * Update the current process' page table (pfn and valid)
   */
   rlt[victim_pfn].vpn = request_vpn;
   current->pagetable[request_vpn].pfn = victim_pfn;
   current->pagetable[request_vpn].valid = 1;
   current->pagetable[request_vpn].valid = write;
   rlt[victim_pfn].pcb = current;


  /*
   * Retreive the page from disk. Note that is really a lie: we save pages in
   * memory (since doing file I/O for this simulation would be annoying and
   * wouldn't add that much to the learning). Also, if the page technically
   * does't exist yet (i.e., the page has never been accessed yet, we return a
   * blank page. Real systems would check for invalid pages and possibly read
   * stuff like code pages from disk. For purposes of this simulation, we won't
   * worry about that. =)
   */
  page_from_disk(victim_pfn, request_vpn, current->pid);

  return victim_pfn;
}
