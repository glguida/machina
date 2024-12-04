/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

/*
  Machina Physical Memory.

  A page in machina can be unused, allocated by the kernel, or used by
  the physical memory cache.

  Physical memory cache can grow or shrink in size over time. A
  physical memory control process tries to make sure to leave always
  'reserved_pages' free for immediate use by the kernel.
*/

static unsigned long total_pages;
static unsigned long reserved_pages;


/*
  Page Allocator.

  The normal NUX page allocator is modified to check that the physical
  memory allocation keeps in the margin of the target allocation for
  the physical memory cache.

  The physical memory controller intervenes reclaiming pages as
  needed, to maintain the balance.
*/

static inline void
physmem_check (void)
{
  printf ("pfn alloc!\n");
}

static inline pfn_t
physmem_pfnalloc (int low)
{
  pfn_t pfn;

  physmem_check ();
  pfn = stree_pfnalloc (low);
  return pfn;
}

static inline void
physmem_pfnfree (pfn_t pfn)
{
  stree_pfnfree (pfn);
}

void
physmem_init (void)
{
  total_pages = pfn_avail ();
  reserved_pages = MIN (RESERVED_MEMORY / PAGE_SIZE, total_pages >> 4);

  info ("Total Memory:    \t%ld Kb.", total_pages * PAGE_SIZE / 1024);
  info ("Reserved Memory: \t%ld Kb.", reserved_pages * PAGE_SIZE / 1024);
  info ("Available Memory:\t%ld Kb.",
	(total_pages - reserved_pages) * PAGE_SIZE / 1024);

  nux_set_allocator (physmem_pfnalloc, physmem_pfnfree);
}
