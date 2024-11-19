/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include "vm.h"

struct slab vmobjs;

struct vmobjref
vmobj_new(bool private, size_t size)
{
  struct vmobj *obj;
  struct vmobjref ref;

  obj = slab_alloc(&vmobjs);
  obj->private = private;
  cacheobj_init (&obj->cobj, size);
  obj->_ref_count = 1;
  ref.obj = obj;
  return ref;
}

void
vmobj_fault(struct vmobj *vmobj, vmoff_t off)
{
  ipte_t ipte;

  info("PAGE FAULT AT CACHE OBJECT %p OFFSET %lx\n", &vmobj->cobj, off);
  ipte = cacheobj_lookup(&vmobj->cobj, off);
  info("IPTE IS %lx\n", ipte.raw);
}

void
vmobj_addregion(struct vmobj *vmobj, struct vm_region *vmreg)
{
  if (vmreg->type == VMR_TYPE_USED)
    cacheobj_addregion(&vmobj->cobj, vmreg);
}

void
vmobj_delregion(struct vmobj *vmobj, struct vm_region *vmreg)
{
  if (vmreg->type == VMR_TYPE_USED)
    cacheobj_delregion(&vmobj->cobj, vmreg);
}

void
vmobj_init(void)
{
  slab_register(&vmobjs, "VMOBJS", sizeof(struct vmobj), NULL, 0);
}


#if 0
/*
  Assumption: We have just booted, and we're running on the BSP. The
  userspace binary of the bootstrap process is mapped.
*/
uaddr_t
memobj_bootstrap(struct memobj **out)
{
  volatile struct vmobj *obj;
  struct pglist clean, dirty;

  pglist_init(&clean);
  pglist_init(&dirty);

  mobj = memobj_new(true);

  hal_l1e_t l1e;
  uaddr_t i, base;
  uaddr_t maxaddr = 0;
  i = hal_umap_next (NULL, 0, NULL, &l1e);
  base = i;
  while (i != UADDR_INVALID)
    {
      maxaddr = i;
      printf("%lx -> %lx\n", i - base, l1e);
      _memobj_addl1e((struct memobj *)mobj, i - base, l1e, &clean, &dirty);
      i = hal_umap_next (NULL, i, NULL, &l1e);
    }

  maxaddr += PAGE_SIZE;
  mobj->size = maxaddr;
  if (out)
    *out = (struct memobj *)mobj;

  assert(pglist_pages(&clean) == 0);
  assert(pglist_pages(&dirty) == 0);
  return base;
}
#endif
