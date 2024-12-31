/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

#ifdef PHYSMEM_DEBUG
#define PHYSMEM_PRINT printf
#else
#define PHYSMEM_PRINT(...)
#endif

/*
  Machina Physical Memory.

  A page in machina can be unused, allocated by the kernel, or used by
  the physical memory cache.

  Physical memory cache can grow or shrink in size over time. A
  physical memory control process tries to make sure to leave always
  'reserved_pages' free for immediate use by the kernel.
*/

/*
  The pages available in the system. Calculated at boot.
*/
static unsigned long total_pages;

/*
  When we're below the watermark, clock starts, one tick per allocation.
*/
static unsigned long watermark_pages;

/*
  The system will try to keep reserved pages free all the time.
*/
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
  assert (pfn_avail() > reserved_pages);
  if (pfn_avail() <= watermark_pages)
    {
      memctrl_tick_one ();
    }
}

static inline pfn_t
physmem_pfnalloc (int low)
{
  pfn_t pfn;

  physmem_check ();
  pfn = stree_pfnalloc (low);
  PHYSMEM_PRINT("PHYSMEM: PFN %lx ALLOCATED.\n", pfn);
  return pfn;
}

static inline void
physmem_pfnfree (pfn_t pfn)
{
  PHYSMEM_PRINT("PHYSMEM: PFN %lx FREED.\n", pfn);
  stree_pfnfree (pfn);
}

pfn_t
userpfn_alloc (void)
{
  if (pfn_avail () <= reserved_pages)
    {
      memctrl_tick_one ();
      return PFN_INVALID;
    }

  return physmem_pfnalloc (0);
}

void
physmem_init (void)
{
  total_pages = pfn_avail ();
  reserved_pages = MIN (RESERVED_MEMORY / PAGE_SIZE, total_pages >> 4);
  watermark_pages = total_pages / 2;

  info ("Total Memory:    \t%ld Kb.", total_pages * PAGE_SIZE / 1024);
  info ("Reserved Memory: \t%ld Kb.", reserved_pages * PAGE_SIZE / 1024);
  info ("Available Memory:\t%ld Kb.",
	(total_pages - reserved_pages) * PAGE_SIZE / 1024);

  nux_set_allocator (physmem_pfnalloc, physmem_pfnfree);
}
