/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

/* Modified version of NUX's alloc.h */
#include <limits.h>
#include <stdint.h>
#include <assert.h>
#include <queue.h>
#include <string.h>
#include <machina/vm_param.h>

#include "internal.h"


struct slab regions_cache;

#define dbgprintf(...)

static void
_print_regions(struct vmmap *map)
{
  struct vm_region *next;

  RB_TREE_FOREACH(next, &map->regions)
    {
      printf("\t%016lx:%016lx\t%s\t%p", next->start, next->start + next->size, next->type == VMR_TYPE_FREE ? "FREE" : "BUSY", next->umap);
      if (next->type == VMR_TYPE_USED)
	{
	  printf("\tOBJ:%lx\tOFF:%lx", next->objref.obj, next->off);
	}
      printf("\n");
    }
}


/*
  VM Regions RB-Tree.

  This tree contains all the regions in a vmmap, including free
  space.
*/

static struct vm_region *
region_find (struct vmmap *map, vaddr_t va)
{
  return rb_tree_find_node (&map->regions, (void *) &va);
}

static void
region_remove (struct vmmap *map, struct vm_region *reg)
{

  rb_tree_remove_node (&map->regions, (void *) reg);
  if (reg->type == VMR_TYPE_FREE)
    map->free -= reg->size;
  else
    {
      vmobj_delregion (vmobjref_unsafe_get(&reg->objref), reg);
      vmobjref_consume (reg->objref);
    }
  slab_free(reg);
}

static struct vm_region *
freeregion_insert (struct vmmap *map, vaddr_t start, vaddr_t size)
{
  struct vm_region *reg;

  reg = slab_alloc(&regions_cache);
  assert (reg != NULL);
  reg->type = VMR_TYPE_FREE;
  reg->start = start;
  reg->size = size;
  rb_tree_insert_node (&map->regions, (void *) reg);
  map->free += reg->size;

  return reg;
}

static int
rb_regs_compare_key (void *ctx, const void *n, const void *key)
{
  const struct vm_region *reg = n;
  const unsigned long va = *(const unsigned long *) key;

  if (va < reg->start)
    return 1;
  if (va > reg->start + reg->size)
    return -1;
  return 0;
}

static int
rb_regs_compare_nodes (void *ctx, const void *n1, const void *n2)
{
  const struct vm_region *reg1 = n1;
  const struct vm_region *reg2 = n2;

  /* Assert non overlapping */
  assert (reg1->start < reg2->start || reg1->start >= (reg2->start + reg2->size));
  assert (reg2->start < reg1->start || reg2->start >= (reg1->start + reg1->size));

  if (reg1->start < reg2->start)
    return -1;
  if (reg1->start > reg2->start)
    return 1;
  return 0;
}

const rb_tree_ops_t regions_tree_ops = {
  .rbto_compare_nodes = rb_regs_compare_nodes,
  .rbto_compare_key = rb_regs_compare_key,
  .rbto_node_offset = offsetof (struct vm_region, rb_regs),
  .rbto_context = NULL
};

/*
  Allocator for VM_regions.

  Uses the Regions tree for searching entries.
*/

#define vm_region vm_region
#define __ZADDR_T vaddr_t
typedef __ZADDR_T zaddr_t;

static void
___get_neighbors (struct vmmap *map, unsigned long addr, size_t size, struct vm_region **pv,
		  struct vm_region **nv)
{
  unsigned long end = addr + size;
  struct vm_region *pvme = NULL, *nvme = NULL;

  if (addr == 0)
    goto _next;

  pvme = region_find (map, addr - 1);
  if (pvme != NULL && pvme->type == VMR_TYPE_FREE)
    *pv = pvme;

_next:
  nvme = region_find (map, end);
  if (nvme != NULL && nvme->type == VMR_TYPE_FREE)
    *nv = nvme;
}

static struct vm_region *
___mkptr (struct vmmap *map, unsigned long addr, size_t size)
{
  return freeregion_insert (map, addr, size);
}

static void
___freeptr (struct vmmap *map, struct vm_region *vme)
{

  region_remove (map, vme);
}


static inline unsigned
lsbit (unsigned long x)
{
  assert (x != 0);
  return __builtin_ffsl (x) - 1;
}

static inline unsigned
msbit (unsigned long x)
{
  assert (x != 0);
  return LONG_BIT - __builtin_clzl (x) - 1;
}

static inline void
_regalloc_detachentry (struct reg_alloc *z, struct vm_region *ze)
{
  uint32_t msb;

  assert (ze->size != 0);
  msb = msbit (ze->size);
  assert (msb < VM_ORDMAX);
  LIST_REMOVE (ze, list);
  dbgprintf ("LIST_REMOVE: %p (%lx ->", ze, z->bmap);
  if (LIST_EMPTY (z->zlist + msb))
    z->bmap &= ~(1UL << msb);
  dbgprintf (" %lx)", z->bmap);
  z->nfree -= ze->size;
  dbgprintf ("D<%p>(%lx,%lx)", ze, ze->start, ze->size);
}

static inline void
_regalloc_attachentry (struct reg_alloc *z, struct vm_region *ze)
{
  uint32_t msb;

  assert (ze->size != 0);
  msb = msbit (ze->size);
  assert (msb < VM_ORDMAX);

  dbgprintf ("LIST_INSERT(%p + %d, %p), bmap (%lx ->", z->zlist, msb,
	     ze, z->bmap);
  z->bmap |= (1UL << msb);
  dbgprintf (" %lx", z->bmap);


  LIST_INSERT_HEAD (z->zlist + msb, ze, list);
  z->nfree += ze->size;
  dbgprintf ("A<%p>(%lx,%lx)", ze, ze->start, ze->size);
}

static inline struct vm_region *
_regalloc_findfree (struct reg_alloc *zn, size_t size)
{
  unsigned long tmp;
  unsigned int minbit;
  struct vm_region *ze = NULL;

  minbit = msbit (size);

  if (size != (1 << minbit))
    minbit += 1;

  if (minbit >= VM_ORDMAX)
    {
      /* Wrong size */
      return NULL;
    }

  tmp = zn->bmap >> minbit;
  if (tmp)
    {
      tmp = lsbit (tmp);
      ze = LIST_FIRST (zn->zlist + minbit + tmp);
      dbgprintf ("LIST_FIRST(%p + %d + %d) = %p", zn->zlist, minbit, tmp, ze);
    }
  return ze;
}

static inline void
map_remove_freeregion (struct vmmap *map, struct vm_region *ze)
{
  _regalloc_detachentry (&map->zones, ze);
  ___freeptr (map, ze);
}

static inline void
map_create_freeregion (struct vmmap *map, zaddr_t zaddr, size_t size)
{
  struct vm_region *ze, *pze = NULL, *nze = NULL;
  zaddr_t fprev = zaddr, lnext = zaddr + size;

  dbgprintf ("HHH1");
  ___get_neighbors (map, zaddr, size, &pze, &nze);
  dbgprintf ("HHH2");
  if (pze)
    {
      fprev = pze->start;
      map_remove_freeregion (map, pze);
    }
  dbgprintf ("HHH3");
  if (nze)
    {
      lnext = nze->start + nze->size;
      map_remove_freeregion (map, nze);
    }
  dbgprintf ("HHH4");
  ze = ___mkptr (map, fprev, lnext - fprev);
  dbgprintf ("MKPTR(%p): %lx,%lx", ze, ze->start, ze->size);
  _regalloc_attachentry (&map->zones, ze);
}

void
reg_alloc_free (struct vmmap *map, zaddr_t zaddr, size_t size)
{

  assert (size != 0);
  dbgprintf ("Freeing %lx", zaddr);
  map_create_freeregion (map, zaddr, size);
}

static inline zaddr_t
reg_alloc_alloc (struct vmmap *map, size_t size)
{
  struct vm_region *ze;
  zaddr_t addr = (zaddr_t) - 1;
  long diff;

  assert (size != 0);

  ze = _regalloc_findfree (&map->zones, size);
  if (ze == NULL)
    goto out;

  addr = ze->start;
  diff = ze->size - size;
  assert (diff >= 0);
  map_remove_freeregion (map, ze);
  if (diff > 0)
    map_create_freeregion (map, addr + size, diff);

out:
  dbgprintf ("Allocating %lx", addr);
  return addr;
}

static void
vmreg_copy(struct vm_region *to, struct vm_region *from)
{
  to->type = from->type;
  to->umap = from->umap;
  to->start = from->start;
  to->size = from->size;
  if (from->type == VMR_TYPE_USED)
    to->objref = vmobjref_clone(&from->objref);
  else
    to->objref = (struct vmobjref){ .obj = NULL, };
  to->off = from->off;
}

static void
vmreg_move(struct vm_region *to, struct vm_region *from)
{
  to->type = from->type;
  to->umap = from->umap;
  to->start = from->start;
  to->size = from->size;
  if (from->type == VMR_TYPE_USED)
    to->objref = vmobjref_move(&from->objref);
  else
    to->objref = (struct vmobjref){ .obj = NULL, };
  to->off = from->off;
}

static void
vmreg_consume(struct vm_region *reg)
{
  if (reg->type == VMR_TYPE_USED)
    vmobjref_consume(reg->objref);
}

static void
_make_hole (struct vmmap *map, vaddr_t start, size_t size)
{
  vaddr_t end;
  struct vm_region *reg, *next, first, last;

  start = trunc_page(start);
  end = round_page(start + size);
  size = end - start;

  /*
    Save first and last section.
  */
  reg = region_find(map, end);
  assert(reg->start <= end);
  assert(end <= (reg->start + reg->size));
  vmreg_copy(&last, reg);
  last.size = last.start + last.size - end;
  last.off = end - last.start + last.off;
  last.start = end;

  reg = region_find(map, start);
  assert(reg->start <= start);
  assert(start <= (reg->start + reg->size));
  vmreg_copy(&first, reg);
  first.size = start - first.start;

  /*
    Iterate until the end of the range and remove all regions.
  */
  next = rb_tree_iterate(&map->regions, reg, RB_DIR_RIGHT);
  region_remove (map, reg);
  while (next)
    {
      reg = next;
      assert(reg->start >= start);
      if (start + size <= reg->start)
	break;

      next = rb_tree_iterate(&map->regions, reg, RB_DIR_RIGHT);
      region_remove (map, reg);
    }

  /*
    Enter if not empty
  */
  if (first.size != 0)
    {
      if (first.type == VMR_TYPE_USED)
	{
	  reg = slab_alloc(&regions_cache);
	  assert (reg != NULL);
	  vmreg_move(reg, &first);
	  vmobj_addregion (vmobjref_unsafe_get(&reg->objref), reg);
	  rb_tree_insert_node (&map->regions, (void *) reg);
	}
      else
	{
	  reg_alloc_free(map, first.start, first.size);
	}
    }
  else
    {
      vmreg_consume(&first);
    }

  if (last.size != 0)
    {
      if (last.type == VMR_TYPE_USED)
	{
	  reg = slab_alloc(&regions_cache);
	  assert (reg != NULL);
	  vmreg_move(reg, &last);
	  vmobj_addregion (vmobjref_unsafe_get(&reg->objref), reg);
	  rb_tree_insert_node (&map->regions, (void *) reg);
	}
      else
	{
	  reg_alloc_free(map, last.start, last.size);
	}
    }
  else
    {
      vmreg_consume(&last);
    }
}

void
vmreg_del (struct vmmap *map, vaddr_t start, size_t size)
{
#if 0
  vaddr_t end;
  struct vm_region *reg, *next, first, last;
#endif
  printf("DEL %lx %lx\n", start, size);

  spinlock(&map->lock);
  _make_hole(map, start, size);
  reg_alloc_free(map, start, size);
  spinunlock(&map->lock);
}

void
vmreg_new (struct vmmap *map, vaddr_t start, struct vmobjref objref, vmoff_t off, size_t size)
{
#if 0
  vaddr_t end;
  struct vm_region *reg, *next, first, last;
#endif

  spinlock(&map->lock);
#if 0
  printf("NEW %lx %lx\n", start, size);
  start = trunc_page(start);
  end = round_page(start + size);
  size = end - start;

  /*
    Save first and last section.
  */
  reg = region_find(map, end);
  assert(reg->start <= end);
  assert(end <= (reg->start + reg->size));
  vmreg_copy(&last, reg);
  last.size = last.start + last.size - end;
  last.start = end;

  reg = region_find(map, start);
  assert(reg->start <= start);
  assert(start <= (reg->start + reg->size));
  vmreg_copy(&first, reg);
  first.size = start - first.start;

  /*
    Iterate until the end of the range and remove all regions.
  */
  next = rb_tree_iterate(&map->regions, reg, RB_DIR_RIGHT);
  region_remove (map, reg);
  while (next)
    {
      reg = next;
      assert(reg->start >= start);
      if (start + size <= reg->start)
	break;

      next = rb_tree_iterate(&map->regions, reg, RB_DIR_RIGHT);
      region_remove (map, reg);
    }

  /*
    Enter if not empty
  */
  if (first.size != 0)
    {
      if (first.type == VMR_TYPE_USED)
	{
	  reg = slab_alloc(&regions_cache);
	  assert (reg != NULL);
	  vmreg_move(reg, &first);
	  vmobj_addregion (vmobjref_unsafe_get(&reg->objref), reg);
	  rb_tree_insert_node (&map->regions, (void *) reg);
	}
      else
	{
	  reg_alloc_free(map, first.start, first.size);
	}
    }
  else
    {
      vmreg_consume(&first);
    }

  if (last.size != 0)
    {
      if (last.type == VMR_TYPE_USED)
	{
	  reg = slab_alloc(&regions_cache);
	  assert (reg != NULL);
	  vmreg_move(reg, &last);
	  vmobj_addregion (vmobjref_unsafe_get(&reg->objref), reg);
	  rb_tree_insert_node (&map->regions, (void *) reg);
	}
      else
	{
	  reg_alloc_free(map, last.start, last.size);
	}
    }
  else
    {
      vmreg_consume(&last);
    }
#endif
  _make_hole(map, start, size);

  /* Insert the new region. */
  struct vm_region *reg = (void *)kmem_alloc (1, sizeof(struct vm_region));
  assert (reg != NULL);
  reg->umap = &map->umap;
  reg->start = start;
  reg->size = size;
  reg->type = VMR_TYPE_USED;
  reg->objref = vmobjref_move(&objref);
  vmobj_addregion (vmobjref_unsafe_get(&reg->objref), reg);
  rb_tree_insert_node (&map->regions, (void *) reg);
  spinunlock(&map->lock);
}


/*
  Setup regions structures for an empty VM Map.
*/
void
vmreg_setup (struct vmmap *map)
{
  struct reg_alloc *z = &map->zones;
  int i;

  z->bmap = 0;
  z->nfree = 0;
  for (i = 0; i < VM_ORDMAX; i++)
    LIST_INIT (z->zlist + i);

  rb_tree_init (&map->regions, &regions_tree_ops);

  map->total = VM_MAP_USER_SIZE;
  map->free = 0;

  reg_alloc_free(map, VM_MAP_USER_START, VM_MAP_USER_END);
}


void
vmreg_print (struct vmmap *map)
{
#if 0
  struct vm_region *next;
#endif

  printf("VM REGIONS for map %p\n", map);
  
  spinlock(&map->lock);
  _print_regions(map);
#if 0
  RB_TREE_FOREACH(next, &map->regions)
    {
      printf("\t%016lx:%016lx\t%s\t%p", next->start, next->start + next->size, next->type == VMR_TYPE_FREE ? "FREE" : "BUSY", next->umap);
      if (next->type == VMR_TYPE_USED)
	{
	  printf("\tOBJ:%lx\tOFF:%lx", next->objref.obj, next->off);
	}
      printf("\n");
    }
#endif
  spinunlock(&map->lock);

  printf("\n");
}

void
vmreg_init(void)
{
  slab_register(&regions_cache, "VM Regions", sizeof (struct vm_region), NULL, 1);
}
