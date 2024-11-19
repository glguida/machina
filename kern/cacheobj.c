/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include "vm.h"

void
cacheobj_addregion(struct cacheobj *cobj, struct vm_region *vmreg)
{
  printf("CACHEOBJECT: adding region to object: %lx %lx\n", vmreg->start, vmreg->size);
  assert (vmreg->type == VMR_TYPE_USED);
  writelock (&cobj->lock);
  LIST_INSERT_HEAD (&cobj->regions, vmreg, list);
  writeunlock (&cobj->lock);
}

void
cacheobj_delregion(struct cacheobj *cobj, struct vm_region *vmreg)
{
  printf("CACHEOBJECT: delete region to object: %lx %lx\n", vmreg->start, vmreg->size);
  assert (vmreg->type == VMR_TYPE_USED);
  writelock (&cobj->lock);
  LIST_REMOVE (vmreg, list);
  for (vaddr_t i = vmreg->start; i < vmreg->start + vmreg->size; i += PAGE_SIZE)
    umap_unmap(vmreg->umap, i);
  umap_commit (vmreg->umap);
  writeunlock (&cobj->lock);
}

ipte_t
cacheobj_map(struct cacheobj *cobj, vmoff_t off, pfn_t pfn, bool writable)
{
  ipte_t ret;

  writelock(&cobj->lock);
  ret = imap_map(&cobj->map, off, pfn, writable);
  writeunlock(&cobj->lock);

  return ret;
}

ipte_t
cacheobj_lookup(struct cacheobj *cobj, vmoff_t off)
{
  ipte_t ret;

  readlock(&cobj->lock);
  ret = imap_lookup(&cobj->map, off);
  readunlock(&cobj->lock);
  return ret;
}

void
cacheobj_init (struct cacheobj *cobj, size_t size)
{
  rwlock_init(&cobj->lock);
  cobj->size = size;
  imap_init (&cobj->map);
  LIST_INIT (&cobj->regions);
}
