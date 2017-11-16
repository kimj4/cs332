#ifndef _STUDENT_PAGE_SPLITTING_H_
#define _STUDENT_PAGE_SPLITTING_H_

#include "global.h"

/*******************************************************************************
 * Your assignment for problem 1 is to fix the following macros.
 * Note: you can put any expression (equation) in the parentheses including
 * the use of any constants, global variables, and/or the given argument "addr"
 * HINT: global variable page_size tells you how many addresses a single page contains.
 * HINT: use modulus division and integer division, not bitwise shifts/masks,
 * unless you really really want to...
 */

/*******************************************************************************
 * Get the page number from a virtual address.
 *
 * @param addr The virtual address.
 * @return     The virtual page number for this addres.
 */
#define VADDR_PAGENUM(addr) (addr / page_size)

/*******************************************************************************
 * Get the offset for a particular address.
 * @param addr The virtual address.
 * @return     The offset into a page for the given virtual address.
 */
#define VADDR_OFFSET(addr) (addr % page_size)

#endif/*_STUDENT_PAGE_SPLITTING_H_*/
