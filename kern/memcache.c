/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

/*
  The physical memory cache is a dynamic cache of memory objects created
  by pagers, which are the entirety of user tasks memory.
*/

struct slab cobjlinks;

/*
  Physical Memory DB.

  Every page in the Physical Memory Cache has a valid 'physmem_page'
  structure associated with it.

  This is allocated as needed, and at the moment once allocated is not
  freed. The DB indexes by pfn.

  Each page has a list of Cache Objects (and their offsets) that maps
  them. This is used to remove mappings when asked to remove the page
  by the physical memory controller.

  There's a special page, the zero page. It is meant to always be
  zero, and must be mapped read-only. For this page, we do not store
  the cache objects linked to it, as it will be never removed.
*/

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
      if (!__sync_bool_compare_and_swap(ptr, NULL, new))
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

  A page in the physical memory cache is either uniquely assigned an
  offset in a VM object, or shared read-only while being assigned to
  multiple vm-objects.

  Each page (except for the zero page) is put in a circular queue
  managed by the memory controller.
*/

pfn_t zeropfn = PFN_INVALID;
struct physmem_page *zeropage;

static pfn_t
_zeropage_private(struct cobj_link *cl)
{
  pfn_t pfn;
  struct physmem_page *page;

  pfn = pfn_alloc(0);
  assert(pfn != PFN_INVALID);
  page = _get_entry(pfn);
  
  spinlock_init(&page->lock);

  LIST_INSERT_HEAD(&page->links, cl, list);
  page->links_no = 1;

  memctrl_newpage(page);

  return pfn;
}

static pfn_t
_duplicate_private(pfn_t pfn, struct cobj_link *cl)
{
  pfn_t newpfn;
  struct physmem_page *page;
  void *dst, *src;

  assert(pfn != PFN_INVALID);
  newpfn = pfn_alloc(0);
  assert(newpfn != PFN_INVALID);

  src = pfn_get(pfn);
  dst = pfn_get(newpfn);
  memcpy(dst, src, PAGE_SIZE);
  pfn_put(pfn, src);
  pfn_put(newpfn, dst);
  
  page = _get_entry(pfn);
  spinlock_init(&page->lock);

  LIST_INSERT_HEAD(&page->links, cl, list);
  page->links_no = 1;

  memctrl_newpage(page);
  return pfn;
}

pfn_t
memcache_zeropage_new (struct cacheobj *obj, mcn_vmoff_t off, bool roshared, mcn_vmprot_t protmask)
{
  pfn_t pfn;

  assert (zeropfn != PFN_INVALID);
  if (roshared)
      pfn = zeropfn;
  else
    {
      struct cobj_link *cl;
      cl = slab_alloc(&cobjlinks);
      assert (cl != NULL);
      cl->cobj = obj;
      cl->off = off;
      pfn = _zeropage_private(cl);
    }
  ipte_t old = cacheobj_map(obj, off, pfn, roshared, protmask);
  printf("ipte old: %lx (sizeof ipte: %lx)\n", old.raw, sizeof(old));
  return pfn;
}

/*
  Unshare.
*/
pfn_t
memcache_unshare (pfn_t pfn, struct cacheobj *obj, mcn_vmoff_t off, mcn_vmprot_t protmask)
{
  pfn_t outpfn;
  struct physmem_page *page = _get_entry(pfn);

  printf("MEMCACHE: unshared %lx %p %lx (mask: %x)\n", pfn, obj, off, protmask);
  spinlock(&page->lock);

  if (pfn == zeropfn)
    {
      /* Zero page: No need to remove the link. */
      struct cobj_link *cl;
      cl = slab_alloc(&cobjlinks);
      assert (cl != NULL);
      cl->cobj = obj;
      cl->off = off;
      outpfn = _zeropage_private(cl);
    }
  else if (page->links_no == 1)
    {
      /* Only one link. No need to alloc a new page. */
      outpfn = pfn;
    }
  else
    {
      struct cobj_link *v, *t, *found;

      /*
	First, find and remove the link.
      */
      found = NULL;
      LIST_FOREACH_SAFE(v, &page->links, list, t) {
	if ((v->cobj == obj) && (v->off == off))
	  {
	    LIST_REMOVE(v, list);
	    found = v;
	    break;
	  }
      }
      assert (found != NULL);

      /*
	Then, allocate a page and link it to the object.
      */
      outpfn = _duplicate_private(pfn, found);
    }

  /*
    Map in the cache object.
  */
  assert(outpfn != PFN_INVALID);
  ipte_t old = cacheobj_map(obj, off, outpfn, false, protmask);
  printf("ipte old: %lx (sizeof ipte: %lx)\n", old.raw, sizeof(old));

  spinunlock(&page->lock);

  return outpfn;  
}

static void
physmemdb_init(void)
{
  unsigned long maxpfn = hal_physmem_maxrampfn ();
  unsigned long l0_entries = NUM_ENTRIES;
  unsigned long l1_entries = (maxpfn + l0_entries - 1) / l0_entries;
  size_t l1_size = sizeof (struct physpage *) * l1_entries;

  physmem_db = (struct physmem_page **) kmem_alloc (0, l1_size);
  memset (physmem_db, 0, l1_size);
  info ("PFNDB L1: %p:%p\n", physmem_db, (void *)physmem_db + l1_size);
}

void
memcache_init(void)
{
  zeropfn = pfn_alloc(0);
  assert(zeropfn != PFN_INVALID);
  physmemdb_init();
  zeropage = _get_entry(zeropfn);

  slab_register(&cobjlinks, "COBJLINKS", sizeof(struct cobj_link), NULL, 0);
}
