/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <assert.h>

#include "internal.h"

struct slab vmobjlinks;

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

  There are two type of physpage: ROSHARED and PRIVATE. Private pages
  are associated with a single VM object. ROSHARED are associated with
  multiple VM ojbects, but must be strictly read-only.  The other
  special type is ZEROSHARED. The ZERO page is a special permanent
  page, always set to zero. It is read-only shared, but being
  permanent we do not store the object mappings.
*/

enum physmem_pgty {
  PGTY_ZEROSHARED,
  PGTY_ROSHARED,
  PGTY_PRIVATE,
};

struct cobj_link {
  struct cacheobj *cobj;
  vmoff_t off;
  LIST_ENTRY(cobj_link) list;
};

struct physmem_page {
  lock_t lock;
  enum physmem_pgty type;
  union {
    struct {
      struct cobj *obj;
      vmoff_t off;
    } private;
    struct {
      LIST_HEAD(,cobj_link) links;
    }roshared;
    struct {} zeroshared;
  };
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
  managed by the page reclamation algorithm.
*/

lock_t physcache_lock;
TAILQ_HEAD(, physmem_page) physcache;
pfn_t zeropfn = PFN_INVALID;
struct physmem_page *zeropage;

#if 0
void
memcache_movepage(struct cobj *tobj, vmoff_t toff, struct cobj *fobj, vmoff_t foff)
{
  /*
    Assumes: both objects are locked.
   */
  pfn_t pfn = cobj_locked_getpfn(fobj, foff);
  assert (pfn != PFN_INVALID);
  struct physmem_page *page = _get_entry(pfn);
  
  spinlock(&physcache_lock);
  spinlock(&page->lock);
  switch (page->type)
    {
    case PGTY_PRIVATE:
      cobj_locked_unmap (fobj, foff, pfn, VMOFF_PRIVATE);
      cobj_locked_map (tobj, toff, pfn, VMOFF_PRIVATE);
      assert(page->private.obj == fobj);
      assert(page->private.off == foff);
      page->private.obj = tobj;
      page->private.off = toff;
      break;
    case PGTY_ROSHARED: {
      struct cobj_link *v, *t;
      bool done = false;
      cobj_locked_unmap (fobj, foff, pfn, VMOFF_PRIVATE);
      cobj_locked_map (tobj, toff, pfn, VMOFF_PRIVATE);
      LIST_FOREACH_SAFE(v, &page->roshared.links, list, t) {
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
    case PGTY_ZEROSHARED: {
      cobj_locked_unmap (fobj, foff, pfn, VMOFF_PRIVATE);
      cobj_locked_map (tobj, toff, pfn, VMOFF_PRIVATE);
      break;
    }
    }
  spinunlock(&page->lock);

  TAILQ_REMOVE(&physcache, page, pageq);
  TAILQ_INSERT_HEAD(&physcache, page, pageq);
  spinunlock(&physcache_lock);
}
#endif

void
memcache_addzeropage(struct cacheobj *obj, vmoff_t off)
{
  assert (zeropfn != PFN_INVALID);

  ipte_t old = cacheobj_map(obj, off, zeropfn, false);
  assert (old.raw == IPTE_EMPTY.raw);
}

#if 0
void
add_link(struct physmem_page *page, struct cacheobj *obj, vmoff_t off)
{
  assert (page->type == PGTY_ROSHARED);
  struct cobj_link *l = slab_alloc(&cobjlinks);
  assert (l != NULL);
  l->obj = obj;
  l->off = off;
  LIST_INSERT_HEAD(&page->roshared.links, l, list);
}

void
memcache_copypage(struct cacheobj *tobj, vmoff_t toff, struct cacheobj *fobj, vmoff_t foff)
{
  /*
    Assumes: both objects are locked.
  */
  pfn_t pfn = cobj_locked_getpfn(fobj, foff);
  assert (pfn != PFN_INVALID);
  struct physmem_page *page = _get_entry(pfn);

  spinlock(&page->lock);
  switch (page->type)
    {
    case PGTY_ZEROSHARED:
      /*
	Zero shared. Just map.
      */
      assert (pfn == zeropfn);
      cobj_locked_map(tobj, toff, pfn, VMOFF_ROSHARED);
      break;
    case PGTY_ROSHARED:
      /*
	Already shared. Map and add link.
      */
      cobj_locked_map(tobj, toff, pfn, VMOFF_ROSHARED);
      add_link(page, tobj, toff);
      break;
    case PGTY_PRIVATE:
      /*
	Make page shared.
      */
      assert (page->private.obj == fobj);
      assert (page->private.off == foff);
      cobj_locked_map(fobj, foff, pfn, VMOFF_ROSHARED);
      page->type = PGTY_ROSHARED;
      LIST_INIT(&page->roshared.links);
      cobj_locked_map(fobj, foff, pfn, VMOFF_ROSHARED);
      add_link(page, fobj, foff);
      cobj_locked_map(tobj, toff, pfn, VMOFF_ROSHARED);
      add_link(page, tobj, toff);
      break;
    default:
      fatal ("Invalid page type %d\n", page->type);
      break;
    }


  /*
    Add the page back at the top of the clock.
  */
  spinlock(&physcache_lock);
  TAILQ_REMOVE(&physcache, page, pageq);
  TAILQ_INSERT_TAIL(&physcache, page, pageq); /* XXX: THIS SHOULD BE PUT AFTER THE CLOCK. */
  spinunlock(&physcache_lock);
}

void
memcache_unshare(struct cacheobj *obj, vmoff_t off)
{
  /*
    Assumes: both objects are locked.
   */
  pfn_t pfn = cobj_locked_getpfn(obj, off);
  assert (pfn != PFN_INVALID);
  struct physmem_page *page = _get_entry(pfn);

  
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
physmem_init (void)
{
  zeropfn = pfn_alloc(0);
  assert(zeropfn != PFN_INVALID);
  total_pages = pfn_avail();
  reserved_pages = MIN(RESERVED_MEMORY/PAGE_SIZE, total_pages >> 4);

  physmemdb_init();
  zeropage = _get_entry(zeropfn);


  info ("Total Memory:    \t%ld Kb.", total_pages * PAGE_SIZE / 1024 );
  info ("Reserved Memory: \t%ld Kb.", reserved_pages * PAGE_SIZE / 1024 );
  info ("Available Memory:\t%ld Kb.", (total_pages - reserved_pages) * PAGE_SIZE / 1024);

  nux_set_allocator(physmem_pfnalloc, physmem_pfnfree);

#if 0
  slab_register(&cobjlinks, "COBJLINKS", sizeof(struct cobj_link), NULL, 0);
#endif
}

