/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <assert.h>
#include <string.h>
#include <nux/apxh.h>

#include "internal.h"


static struct physpage **pfndb;

#define NUM_ENTRIES (PAGE_SIZE/sizeof(struct physpage))
#define L1OFF(_pfn) ((_pfn)/NUM_ENTRIES)
#define L0OFF(_pfn) ((_pfn)%NUM_ENTRIES)

static void
_populate_entry (pfn_t pfn)
{
  struct physpage **ptr;

  ptr = pfndb + L1OFF (pfn);

  if (*ptr == NULL)
    {
      pfn_t pfn;
      void *new;

      pfn = pfn_alloc (0);
      assert (pfn != PFN_INVALID);
      new = kva_map (pfn, HAL_PTE_P | HAL_PTE_W);
      *ptr = new;
    }

  assert (*ptr != NULL);
}

static struct physpage *
_get_entry (pfn_t pfn, bool init)
{
  struct physpage **ptr;
  assert (pfn <= hal_physmem_maxpfn ());

  if (init)
    _populate_entry (pfn);

  ptr = pfndb + L1OFF (pfn);

  if (*ptr == NULL)
    return NULL;
  else
    return *ptr + L0OFF (pfn);
}

static void
_pfndb_init (pfn_t pfn)
{
  struct physpage *ptr;

  ptr = _get_entry (pfn, true);
  assert (ptr != NULL);
  ptr->pfn = pfn;
  ptr->type = TYPE_UNKNOWN;
}

static void
_pfndb_inittype (pfn_t pfn, unsigned type)
{
  struct physpage *ptr;

  ptr = _get_entry (pfn, true);
  assert (ptr != NULL);

  /*
    There's an implicit priority in PFN types. At init time, types
    with a higher numerical value can overwrite types with with a
    lower numerical value.

    This is only valid in init. During runtime, the switch from one
    type to another follows a fixed state machine.
  */
  if (type < ptr->type)
    {
      assert(ptr->pfn = pfn);
      ptr->type = type;
      /* XXX: More init here as struct grows. */
    }
}


static void pfndb_init(void)
{
  unsigned long maxpfn = hal_physmem_maxpfn ();
  unsigned long l0_entries = NUM_ENTRIES;
  unsigned long l1_entries = (maxpfn + l0_entries - 1) / l0_entries;
  size_t l1_size = sizeof (struct physpage *) * l1_entries;

  pfndb = (struct physpage **) kmem_alloc (0, l1_size);
  memset (pfndb, 0, l1_size);
  for (pfn_t i = 0; i < maxpfn; i++)
    _pfndb_init(i);
  info ("PFNDB L1: %p:%p\n", pfndb, (void *)pfndb + l1_size);

  unsigned n = hal_physmem_numregions ();
  unsigned i;
  pfn_t j;

  info ("Platform mappable memory:\n");
  for (i = 0; i < n; i++)
    {
      struct apxh_region *r = hal_physmem_region (i);
      switch (r->type)
	{
	case APXH_REGION_UNKNOWN:
	case APXH_REGION_MMIO:
	  info ("\tMMIO: %016" PRIx64 " : %016" PRIx64,
		(uint64_t) r->pfn << PAGE_SHIFT,
		(r->pfn + r->len) << PAGE_SHIFT);

	  for (j = r->pfn; j <= (r->pfn + r->len); j++)
	    {
	      _pfndb_inittype (j, TYPE_NONRAM);
	    }
	  break;
	case APXH_REGION_BSY:
	  info ("\tBIOS: %016" PRIx64 " : %016" PRIx64,
		(uint64_t) r->pfn << PAGE_SHIFT,
		(r->pfn + r->len) << PAGE_SHIFT);
	  for (j = r->pfn; j <= (r->pfn + r->len); j++)
	    {
	      _pfndb_inittype (j, TYPE_NONRAM);
	    }
	  break;

	case APXH_REGION_RAM:
	  info ("\tRAM : %016" PRIx64 " : %016" PRIx64,
		(uint64_t) r->pfn << PAGE_SHIFT,
		(r->pfn + r->len) << PAGE_SHIFT);
	  for (j = r->pfn; j < (r->pfn + r->len); j++)
	    {
	      _pfndb_inittype (j, TYPE_UNKNOWN);
	    }
	  break;
	default:
	  break;
	}
    }
}

struct physpage *
physpage_get(pfn_t pfn)
{
  return _get_entry(pfn, false);
}


struct pglist pglist_reserved = { 0, };
struct pglist pglist_free = { 0, };

static bool
reserved_memory_need(void)
{
  bool need;

  spinlock(&pglist_reserved.lock);
  need = pglist_reserved.pages < (RESERVED_MEMORY/PAGE_SIZE);
  spinunlock(&pglist_reserved.lock);

  return need;
}

pfn_t pfn_alloc_kernel(bool mayfail)
{
  struct physpage *pg = pglist_rem (&pglist_free);
  void *va;

  if (!pg && mayfail)
    return PFN_INVALID;

  if (!pg)
    {
      warn ("Using reserved memory!");
      pg = pglist_rem (&pglist_reserved);
      assert(pg != NULL);
      assert(pg->type == TYPE_RESERVED);
    }
  else assert (pg->type == TYPE_FREE);

  pg->type = TYPE_SYSTEM;

  va = pfn_get (pg->pfn);
  memset (va, 0, PAGE_SIZE);
  pfn_put (pg->pfn, va);
  
  return pg->pfn;
}

void pfn_free_kernel(pfn_t pfn)
{
  struct physpage *pg = physpage_get(pfn);

  assert(pg->pfn == pfn);
  assert((pg->type == TYPE_SYSTEM) || (pg->type == TYPE_UNKNOWN));
  if (reserved_memory_need())
    {
      pg->type = TYPE_RESERVED;
      pglist_add(&pglist_reserved, pg);
    }
  else
    {
      pg->type = TYPE_FREE;
      pglist_add(&pglist_free, pg);
    }
}

static pfn_t nux_pfn_alloc(int unused)
{
  return pfn_alloc_kernel(false);
}

static void nux_pfn_free(pfn_t pfn)
{
  return pfn_free_kernel(pfn);
}

void physmem_init(void)
{
  pfn_t pfn;
  struct physpage *pg;

  /* Allocate PFN database. */
  pfndb_init ();

  /*
    Steal all pages and switch NUX allocator.
  */
  pglist_init(&pglist_reserved);
  pglist_init(&pglist_free);

  unsigned reserved_pages = RESERVED_MEMORY / PAGE_SIZE;
  for (unsigned i = 0; i < reserved_pages; i++)
    {
      pfn_t pfn = pfn_alloc(0);
      assert(pfn != PFN_INVALID);
      pg = physpage_get(pfn);
      assert(pg->pfn == pfn);
      pg->type = TYPE_RESERVED;
      pglist_add(&pglist_reserved, pg);
    }

  while ((pfn = pfn_alloc(0)) != PFN_INVALID)
    {
      pg = physpage_get(pfn);
      assert(pg->pfn == pfn);
      pg->type = TYPE_FREE;
      pglist_add(&pglist_free, pg);
    }

  info ("Allocator Reserved Memory:\t%"PRId64" Kb.", (uint64_t)(pglist_reserved.pages * PAGE_SIZE) >> 10);
  info ("Allocator Available Memory:\t%"PRId64" Kb.", (uint64_t)(pglist_free.pages * PAGE_SIZE) >> 10);

  nux_set_allocator(nux_pfn_alloc, nux_pfn_free);
}

