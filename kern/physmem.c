/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <assert.h>

#include "internal.h"

/*
  Machina Physical Memory.

  A page in machina can be unused, allocated by the kernel, or used by
  the physical memory cache.

  A physical memory cache is a dynamic cache of memory objects created
  by pagers, which are the entirety of user tasks memory.

  Physical memory cache can grow or shrink in size over time. A
  physical memory control process tries to make sure to leave always
  'reserved_pages' free for immediate use by the kernel.
*/

static unsigned long total_pages;
static unsigned long reserved_pages;

/*
  Physical Memory DB.

  Every page in the Physical Memory Cache has a valid 'physmem_page'
  structure associated with it.

  This is allocated as needed, and at the moment once allocated is not
  freed. The DB indexes by pfn.
*/

struct physmem_page {
  lock_t lock;
  struct memobj *obj;
  vmoff_t off;
  TAILQ_ENTRY(physmem_page) pageq;
};

static struct physmem_page **physmem_db = NULL;

#define NUM_ENTRIES (PAGE_SIZE/sizeof(struct physmem_page))
#define L1OFF(_pfn) ((_pfn)/NUM_ENTRIES)
#define L0OFF(_pfn) ((_pfn)%NUM_ENTRIES)

static void
_populate_entry (pfn_t pfn)
{
  struct physmem_page **ptr;

  ptr = physmem_db + L1OFF (pfn);

  if (*ptr == NULL)
    {
      pfn_t pfn;
      void *new;

      pfn = pfn_alloc(0);
      assert (pfn != PFN_INVALID);
      new = kva_map (pfn, HAL_PTE_P | HAL_PTE_W);
      if (!__sync_bool_compare_and_swap(&ptr, NULL, new))
	{
	  kva_unmap (new, PAGE_SIZE);
	  pfn_free(pfn);
	}
    }

  assert (*ptr != NULL);
}

static inline struct physmem_page *
_get_entry (pfn_t pfn)
{
  struct physmem_page **ptr;
  assert (pfn <= hal_physmem_maxpfn ());

  _populate_entry (pfn);

  ptr = physmem_db + L1OFF (pfn);

  if (*ptr == NULL)
    return NULL;
 else
    return *ptr + L0OFF (pfn);
}

/*
  Physical Memory Cache.

  A page in the physical memory cache is uniquely assigned an offset
  in a VM object.

  Each object is put in a circular queue managed by the page
  reclamation algorithm.
*/
lock_t physcache_lock;
TAILQ_HEAD(, physmem_page) physcache;

void
memcache_movepage(struct memobj *tobj, vmoff_t toff, pfn_t pfn, struct memobj *fobj, vmoff_t foff)
{
  struct physmem_page *page = _get_entry(pfn);

  /*
    Assumes: both objects are locked.
   */
  spinlock(&physcache_lock);
  spinlock(&page->lock);
  assert(page->obj == fobj);
  assert(page->off == foff);
  page->obj = tobj;
  page->off = toff;
  spinunlock(&page->lock);
  TAILQ_REMOVE(&physcache, page, pageq);
  TAILQ_INSERT_HEAD(&physcache, page, pageq);
  spinunlock(&physcache_lock);
}

pfn_t
memcache_addzeropage(struct memobj *obj, vmoff_t off)
{
  pfn_t pfn = pfn_alloc(0);
  assert (pfn != PFN_INVALID);
  struct physmem_page *page = _get_entry(pfn);

  /*
    TODO: Zero-page sharing?
  */

  /*
    Assumes: obj is locked.
  */
  spinlock(&page->lock);
  assert(page->obj == NULL);
  page->obj = obj;
  page->off = off;
  spinunlock(&page->lock);

  spinlock(&physcache_lock);
  TAILQ_INSERT_HEAD(&physcache, page, pageq);
  spinunlock(&physcache_lock);

  return pfn;
}

pfn_t
memcache_copypage(struct memobj *tobj, vmoff_t toff, pfn_t fpfn, struct memobj *fobj, vmoff_t foff)
{
  pfn_t dpfn = pfn_alloc(0);
  assert (dpfn != PFN_INVALID);
  struct physmem_page *dest = _get_entry(dpfn);
  struct physmem_page *src = _get_entry(fpfn);
  void *dptr, *sptr;

  /*
    Assumes: both objects are locked.
  */
  dptr = pfn_get(dpfn);
  sptr = pfn_get(fpfn);
  memcpy(dptr, sptr, PAGE_SIZE);
  pfn_put(dpfn, dptr);
  pfn_put(fpfn, sptr);
  

  spinlock_dual(&dest->lock, &src->lock);
  assert(src->obj == fobj);
  assert(src->off == foff);
  assert(dest->obj == NULL);
  dest->obj = tobj;
  dest->off = toff;
  spinunlock_dual(&dest->lock, &src->lock);

  spinlock(&physcache_lock);
  TAILQ_INSERT_HEAD(&physcache, dest, pageq);
  spinunlock(&physcache_lock);

  return dpfn;
}

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
  printf("pfn alloc!\n");
}

static inline pfn_t
physmem_pfnalloc (int low)
{
  pfn_t pfn;

  physmem_check();
  pfn = stree_pfnalloc(low);
  return pfn;
}

static inline void
physmem_pfnfree (pfn_t pfn)
{
  stree_pfnfree(pfn);
}

void
physmem_init (void)
{
  total_pages = pfn_avail();
  reserved_pages = MIN(RESERVED_MEMORY/PAGE_SIZE, total_pages >> 4);

  info ("Total Memory:    \t%ld Kb.", total_pages * PAGE_SIZE / 1024 );
  info ("Reserved Memory: \t%ld Kb.", reserved_pages * PAGE_SIZE / 1024 );
  info ("Available Memory:\t%ld Kb.", (total_pages - reserved_pages) * PAGE_SIZE / 1024);

  nux_set_allocator(physmem_pfnalloc, physmem_pfnfree);
}

