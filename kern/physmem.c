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



#if 0
void
memcache_movepage (struct cobj *tobj, vmoff_t toff, struct cobj *fobj,
		   vmoff_t foff)
{
  /*
     Assumes: both objects are locked.
   */
  pfn_t pfn = cobj_locked_getpfn (fobj, foff);
  assert (pfn != PFN_INVALID);
  struct physmem_page *page = _get_entry (pfn);

  spinlock (&physcache_lock);
  spinlock (&page->lock);
  switch (page->type)
    {
    case PGTY_PRIVATE:
      cobj_locked_unmap (fobj, foff, pfn, VMOFF_PRIVATE);
      cobj_locked_map (tobj, toff, pfn, VMOFF_PRIVATE);
      assert (page->private.obj == fobj);
      assert (page->private.off == foff);
      page->private.obj = tobj;
      page->private.off = toff;
      break;
    case PGTY_ROSHARED:
      {
	struct cobj_link *v, *t;
	bool done = false;
	cobj_locked_unmap (fobj, foff, pfn, VMOFF_PRIVATE);
	cobj_locked_map (tobj, toff, pfn, VMOFF_PRIVATE);
	LIST_FOREACH_SAFE (v, &page->roshared.links, list, t)
	{
	  if ((v->obj == fobj) && (v->off == foff))
	    {
	      v->obj = tobj;
	      v->off = toff;
	      done = true;
	      break;
	    }
	  continue;
	}
	assert (done);
	break;
      }
    case PGTY_ZEROSHARED:
      {
	cobj_locked_unmap (fobj, foff, pfn, VMOFF_PRIVATE);
	cobj_locked_map (tobj, toff, pfn, VMOFF_PRIVATE);
	break;
      }
    }
  spinunlock (&page->lock);

  TAILQ_REMOVE (&physcache, page, pageq);
  TAILQ_INSERT_HEAD (&physcache, page, pageq);
  spinunlock (&physcache_lock);
}
#endif


#if 0
void
add_link (struct physmem_page *page, struct cacheobj *obj, vmoff_t off)
{
  assert (page->type == PGTY_ROSHARED);
  struct cobj_link *l = slab_alloc (&cobjlinks);
  assert (l != NULL);
  l->obj = obj;
  l->off = off;
  LIST_INSERT_HEAD (&page->roshared.links, l, list);
}

void
memcache_copypage (struct cacheobj *tobj, vmoff_t toff, struct cacheobj *fobj,
		   vmoff_t foff)
{
  /*
     Assumes: both objects are locked.
   */
  pfn_t pfn = cobj_locked_getpfn (fobj, foff);
  assert (pfn != PFN_INVALID);
  struct physmem_page *page = _get_entry (pfn);

  spinlock (&page->lock);
  switch (page->type)
    {
    case PGTY_ZEROSHARED:
      /*
         Zero shared. Just map.
       */
      assert (pfn == zeropfn);
      cobj_locked_map (tobj, toff, pfn, VMOFF_ROSHARED);
      break;
    case PGTY_ROSHARED:
      /*
         Already shared. Map and add link.
       */
      cobj_locked_map (tobj, toff, pfn, VMOFF_ROSHARED);
      add_link (page, tobj, toff);
      break;
    case PGTY_PRIVATE:
      /*
         Make page shared.
       */
      assert (page->private.obj == fobj);
      assert (page->private.off == foff);
      cobj_locked_map (fobj, foff, pfn, VMOFF_ROSHARED);
      page->type = PGTY_ROSHARED;
      LIST_INIT (&page->roshared.links);
      cobj_locked_map (fobj, foff, pfn, VMOFF_ROSHARED);
      add_link (page, fobj, foff);
      cobj_locked_map (tobj, toff, pfn, VMOFF_ROSHARED);
      add_link (page, tobj, toff);
      break;
    default:
      fatal ("Invalid page type %d\n", page->type);
      break;
    }


  /*
     Add the page back at the top of the clock.
   */
  spinlock (&physcache_lock);
  TAILQ_REMOVE (&physcache, page, pageq);
  TAILQ_INSERT_TAIL (&physcache, page, pageq);	/* XXX: THIS SHOULD BE PUT AFTER THE CLOCK. */
  spinunlock (&physcache_lock);
}

void
memcache_unshare (struct cacheobj *obj, vmoff_t off)
{
  /*
     Assumes: both objects are locked.
   */
  pfn_t pfn = cobj_locked_getpfn (obj, off);
  assert (pfn != PFN_INVALID);
  struct physmem_page *page = _get_entry (pfn);


}
#endif

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
