/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

#ifndef MEMCACHE_DEBUG
#define MEMCACHE_PRINT(...)
#else
#define MEMCACHE_PRINT printf
#endif

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

      pfn = pfn_alloc (0);
      assert (pfn != PFN_INVALID);
      new = kva_map (pfn, HAL_PTE_P | HAL_PTE_W);
      if (!__sync_bool_compare_and_swap (ptr, NULL, new))
	{
	  kva_unmap (new, PAGE_SIZE);
	  pfn_free (pfn);
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

static void
_existingpage_private (pfn_t pfn, struct cobj_link *cl)
{
  struct physmem_page *page;

  page = _get_entry (pfn);
  assert (page->links_no == 0);
  assert ((page->pfn == 0) || (page->pfn == pfn));

  spinlock_init (&page->lock);
  LIST_INSERT_HEAD (&page->links, cl, list);
  page->links_no = 1;
  page->pfn = pfn;
  memctrl_newpage (page);
}

static pfn_t
_zeropage_private (struct cobj_link *cl)
{
  pfn_t pfn;
  struct physmem_page *page;

  pfn = userpfn_alloc ();
  if (pfn == PFN_INVALID)
    return pfn;
  page = _get_entry (pfn);

  spinlock_init (&page->lock);

  LIST_INSERT_HEAD (&page->links, cl, list);
  assert (page->links_no == 0);
  assert ((page->pfn == 0) || (page->pfn == pfn));
  page->links_no = 1;
  page->pfn = pfn;

  memctrl_newpage (page);

  return pfn;
}

static pfn_t
_duplicate_private (pfn_t pfn, struct cobj_link *cl)
{
  pfn_t newpfn;
  struct physmem_page *page;
  void *dst, *src;

  assert (pfn != PFN_INVALID);
  newpfn = userpfn_alloc ();
  if (newpfn == PFN_INVALID)
    return newpfn;

  src = pfn_get (pfn);
  dst = pfn_get (newpfn);
  memcpy (dst, src, PAGE_SIZE);
  pfn_put (pfn, src);
  pfn_put (newpfn, dst);

  page = _get_entry (newpfn);
  spinlock_init (&page->lock);
  /* Remove from old list. */
  LIST_REMOVE (cl, list);
  LIST_INSERT_HEAD (&page->links, cl, list);
  assert (page->links_no == 0);
  assert ((page->pfn == 0) || (page->pfn == newpfn));
  page->links_no = 1;
  page->pfn = newpfn;

  memctrl_newpage (page);
  return newpfn;
}

void
memcache_existingpage (struct cacheobj *obj, mcn_vmoff_t off, pfn_t pfn, mcn_vmprot_t protmask)
{
  struct cobj_link *cl;

  assert (pfn != PFN_INVALID);
  assert (pfn != zeropfn);

  cl = slab_alloc (&cobjlinks);
  assert (cl != NULL);
  cl->cobj = obj;
  cl->off = off;
  _existingpage_private (pfn, cl);

  MEMCACHE_PRINT ("MEMCACHE: %p offset %lx: Mapping existing %lx protmask %lx\n",
		  obj, off, pfn, (long)protmask);
  ipte_t old = cacheobj_map (obj, off, pfn, false, protmask);
  MEMCACHE_PRINT ("ipte old: %lx (sizeof ipte: %lx)\n", old.raw, sizeof (old));
  (void)old;
}

void
memcache_zeropage_new (struct cacheobj *obj, mcn_vmoff_t off, bool roshared,
		       mcn_vmprot_t protmask)
{
  pfn_t pfn;

  assert (zeropfn != PFN_INVALID);
  if (roshared)
    pfn = zeropfn;
  else
    {
      struct cobj_link *cl;
      cl = slab_alloc (&cobjlinks);
      assert (cl != NULL);
      cl->cobj = obj;
      cl->off = off;
      pfn = _zeropage_private (cl);
      if (pfn == PFN_INVALID)
	{
	  slab_free (cl);
	  return;
	}
    }

  ipte_t old = cacheobj_map (obj, off, pfn, roshared, protmask);
  MEMCACHE_PRINT ("ipte old: %lx (sizeof ipte: %lx)\n", old.raw, sizeof (old));
  (void)old;
}

/*
  Share Page.
*/
void
memcache_share (pfn_t pfn, struct cacheobj *obj, mcn_vmoff_t off,
		mcn_vmprot_t protmask)
{
  struct physmem_page *page = _get_entry (pfn);

  MEMCACHE_PRINT ("MEMCACHE: Share pfn %lx to obj %p off %lx (mask %lx)\n", pfn, obj,
	  off, protmask);
  spinlock (&page->lock);

  if (pfn != zeropfn)
    {
      struct cobj_link *cl;
      cl = slab_alloc (&cobjlinks);
      assert (cl != NULL);
      cl->cobj = obj;
      cl->off = off;
      LIST_INSERT_HEAD (&page->links, cl, list);
      page->links_no++;
    }


  ipte_t old = cacheobj_map (obj, off, pfn, true, protmask);
  MEMCACHE_PRINT ("ipte old: %lx (sizeof ipte: %lx)\n", old.raw, sizeof (old));
  (void)old;

  spinunlock (&page->lock);
}


/*
  Unshare Page.

  OBJ will receive a private copy of PFN at offset OFF.
*/
void
memcache_unshare (pfn_t pfn, struct cacheobj *obj, mcn_vmoff_t off,
		  mcn_vmprot_t protmask)
{
  pfn_t outpfn;
  struct physmem_page *page = _get_entry (pfn);

  MEMCACHE_PRINT ("MEMCACHE: Unshare %lx %p %lx (mask: %x)\n", pfn, obj, off,
	  protmask);
  spinlock (&page->lock);

  if (pfn == zeropfn)
    {
      /* Zero page: No need to remove the link. */
      struct cobj_link *cl;
      cl = slab_alloc (&cobjlinks);
      assert (cl != NULL);
      cl->cobj = obj;
      cl->off = off;
      outpfn = _zeropage_private (cl);
      if (outpfn == PFN_INVALID)
	{
	  slab_free (cl);
	  spinunlock (&page->lock);
	  return;
	}
    }
  else if (page->links_no == 1)
    {
      MEMCACHE_PRINT ("MEMCACHE: ONLY ONE LINK!\n");
      /* Only one link. No need to alloc a new page. */
      outpfn = pfn;
    }
  else
    {
      struct cobj_link *v, *t, *found;
      MEMCACHE_PRINT ("MEMCACHE: NUMBER OF LINKS: %d\n", page->links_no);

      /*
         First, find the link.
       */
      found = NULL;
      LIST_FOREACH_SAFE (v, &page->links, list, t)
      {
	if ((v->cobj == obj) && (v->off == off))
	  {
	    found = v;
	    break;
	  }
      }
      assert (found != NULL);

      /*
         Then, allocate a page and move the link to the new one.
       */
      outpfn = _duplicate_private (pfn, found);
      if (outpfn == PFN_INVALID)
	{
	  spinunlock (&page->lock);
	  return;
	}
      page->links_no -= 1;
    }

  /*
     Map in the cache object.
   */
  assert (outpfn != PFN_INVALID);
  ipte_t old = cacheobj_map (obj, off, outpfn, false, protmask);
  MEMCACHE_PRINT ("ipte old: %lx (sizeof ipte: %lx)\n", old.raw, sizeof (old));
  (void)old;

  spinunlock (&page->lock);
}

void
memcache_cobjremove (pfn_t pfn, struct cacheobj *obj, mcn_vmoff_t off)
{
  struct physmem_page *page = _get_entry (pfn);
  struct cobj_link *n;
  bool del = false;

  MEMCACHE_PRINT ("MEMCACHE: Remove Obj %p off %lx from PFN %lx (is zeropfn (%lx)? %d)\n",
	  obj, off, pfn, zeropfn, pfn == zeropfn);


  if (pfn == zeropfn)
    return;

  spinlock (&page->lock);
  assert (page->links_no != 0);

  LIST_FOREACH(n, &page->links, list)
    {
      MEMCACHE_PRINT("MEMCACHE: PFN %d cobj: %p off %ld\n",
	     pfn, n->cobj, n->off);
      if ((n->cobj == obj) && (n->off == off))
	break;
    }

  assert (n != NULL);
  LIST_REMOVE (n, list);
  if (--page->links_no == 0)
    del = true;

  cacheobj_unmap (obj, off);

  spinunlock (&page->lock);

  if (del)
    {
      memctrl_delpage (page);
      pfn_free(pfn);
    }
  slab_free (n);
}

static void
physmemdb_init (void)
{
  unsigned long maxpfn = hal_physmem_maxrampfn ();
  unsigned long l0_entries = NUM_ENTRIES;
  unsigned long l1_entries = (maxpfn + l0_entries - 1) / l0_entries;
  size_t l1_size = sizeof (struct physmem_page *) * l1_entries;

  physmem_db = (struct physmem_page **) kmem_alloc (0, l1_size);
  memset (physmem_db, 0, l1_size);
  info ("PFNDB L1: %p:%p\n", physmem_db, (void *) physmem_db + l1_size);
}

void
memcache_init (void)
{
  zeropfn = pfn_alloc (0);
  assert (zeropfn != PFN_INVALID);
  physmemdb_init ();
  zeropage = _get_entry (zeropfn);

  slab_register (&cobjlinks, "COBJLINKS", sizeof (struct cobj_link), NULL, 0);
}

bool
memcache_tick(struct physmem_page *page)
{
  bool accessed;
  struct cobj_link *v;

  accessed = false;
  spinlock (&page->lock);
  LIST_FOREACH (v, &page->links, list)
    accessed |= cacheobj_tick (v->cobj, v->off);
  spinunlock (&page->lock);

  return accessed;
}

bool
memcache_swapout (struct physmem_page *page, struct vmobjref *vmobjref)
{
  struct cobj_link *v, *t;

  struct vmobj *vmobj = vmobjref_unsafe_get (vmobjref);

  memcache_share (page->pfn, &vmobj->cobj, 0, MCN_VMPROT_READ);

  spinlock (&page->lock);
  /*
    Move the page from the original vmobject to a new vmobject
    that will be passed to a pager.
  */
  LIST_FOREACH_SAFE (v, &page->links, list, t)
    {
      if (v->cobj == &vmobj->cobj)
	{
	  /*
	    Skip swapping out the vmobject mapping we just created.
	  */
	  continue;
	}
      cacheobj_swapout (v->cobj, v->off, vmobjref_dup(vmobjref));
      LIST_REMOVE(v, list);
      page->links_no -= 1;
      assert (page->links_no != 0);
      slab_free(v);
    }
  spinunlock (&page->lock);
  vmobj = NULL;

  return false;
}
