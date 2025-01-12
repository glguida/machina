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
  High Watermark: System is in what we might call a normal state: page
  are being allocated and there's pressure on the physical memory.
*/
static unsigned long hiwm_pages;

/*
  Low Watermark: The pressure on the physical memory is starting to
  require the kernel attention, as if it continues we might soon hit
  the reserved pages limit.
*/
static unsigned long lowm_pages;

/*
  The system will try to keep at least reserved pages free all the time.
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
  /*
    Main control for pages in the system:

    - When pages are below watermark_high, clock starts. This effectively
      refreshes the accessed bits. No swapout happens.

    - When pages are below watermark low, a user page cannot be
      allocated without issuing a swapout request.

    - When pages are below reserved memory, user pages allocation
      fail. Process requesting allocation will wait in a queue.
  */

  if (pfn_avail() <= hiwm_pages)
    memctrl_tick_one ();

  if (pfn_avail() <= lowm_pages)
    memctrl_swapout_enable ();
  else
    memctrl_swapout_disable ();
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
      /* XXX: WAIT */
      return PFN_INVALID;
    }

  return physmem_pfnalloc (0);
}

void
physmem_init (void)
{
  total_pages = pfn_avail ();
  reserved_pages = MIN (RESERVED_MEMORY / PAGE_SIZE, total_pages >> 4);
  hiwm_pages = total_pages / 2;
  lowm_pages = reserved_pages + (hiwm_pages - reserved_pages)/2;

  info ("Total Memory:    \t%ld Kb.", total_pages * PAGE_SIZE / 1024);
  info ("Reserved Memory: \t%ld Kb.", reserved_pages * PAGE_SIZE / 1024);
  info ("Available Memory:\t%ld Kb.",
	(total_pages - reserved_pages) * PAGE_SIZE / 1024);
  info ("Low Watermark:   \t%ld Kb.", lowm_pages * PAGE_SIZE / 1024);
  info ("High Watermark:  \t%ld Kb.", hiwm_pages * PAGE_SIZE / 1024);

  nux_set_allocator (physmem_pfnalloc, physmem_pfnfree);
}
