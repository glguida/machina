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
      struct cacheobj *obj;
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

pfn_t _zeropage_private(struct cacheobj *obj, vmoff_t off)
{
  pfn_t pfn;
  struct physmem_page *page;

  pfn = pfn_alloc(0);
  assert(pfn != PFN_INVALID);
  page = _get_entry(pfn);
  spinlock_init(&page->lock);
  page->type = PGTY_PRIVATE;
  page->private.obj = obj;
  page->private.off = off;

  spinlock (&physcache_lock);
  TAILQ_INSERT_HEAD(&physcache, page, pageq);
  spinunlock(&physcache_lock);

  return pfn;
}

pfn_t _duplicate_private(pfn_t pfn, struct cacheobj *obj, vmoff_t off)
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
  page->type = PGTY_PRIVATE;
  page->private.obj = obj;
  page->private.off = off;

  spinlock (&physcache_lock);
  TAILQ_INSERT_HEAD(&physcache, page, pageq);
  spinunlock(&physcache_lock);

  return pfn;
}

pfn_t
memcache_zeropage_new (struct cacheobj *obj, vmoff_t off, bool roshared, vm_prot_t protmask)
{
  pfn_t pfn;

  assert (zeropfn != PFN_INVALID);
  if (roshared)
      pfn = zeropfn;
  else
    {
      /* Allow writes. */
      pfn = _zeropage_private(obj, off);
    }
  ipte_t old = cacheobj_map(obj, off, pfn, roshared, protmask);
  printf("ipte old: %lx (sizeof ipte: %lx)\n", old.raw, sizeof(old));
  return pfn;
}

/*
  Unshare.
*/
pfn_t
memcache_unshare (pfn_t pfn, struct cacheobj *obj, vmoff_t off, vm_prot_t protmask)
{
  pfn_t outpfn;
  struct physmem_page *page = _get_entry(pfn);

  printf("MEMCACHE: unshared %lx %p %lx (mask: %x)\n", pfn, obj, off, protmask);
  spinlock(&page->lock);

  switch (page->type)
    {
    case PGTY_PRIVATE:
      fatal ("MEMCACHE: can't unshare private page (obj: %p off: %lx pfn: %lx)\n",
	     obj, off, pfn);
      outpfn = PFN_INVALID;
      break;

    case PGTY_ZEROSHARED:
      outpfn = _zeropage_private(obj, off);
      printf ("MEMCACHE: unsharing zero page to %lx\n", outpfn);
      break;

#if 0
    case PGTY_ROSHARED:
      {
	struct cobj_link *l = _find_link(page, obj, off);
	assert(l != NULL);
	LIST_REMOVE (l, list);
	slab_free(l);
	outpfn = _duplicate_private(pfn, obj, off);
      }
      break;
#endif

    default:
      fatal ("MEMCACHE: invalid page type (obj: %p off: %lx pfn: %lx type: %d)\n",
	     obj, off, pfn, page->type);
      outpfn = PFN_INVALID;
      break;
    }

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
  zeropage->type = PGTY_ZEROSHARED;

  slab_register(&cobjlinks, "COBJLINKS", sizeof(struct cobj_link), NULL, 0);
}
