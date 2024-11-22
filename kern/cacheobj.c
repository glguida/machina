/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include "vm.h"

void
cacheobj_addmapping (struct cacheobj *cobj, struct cacheobj_mapping *cobjm)
{

  printf ("CACHEOBJECT: adding region to object: %lx %lx\n", cobjm->start,
	  cobjm->size);
  writelock (&cobj->lock);
  LIST_INSERT_HEAD (&cobj->mappings, cobjm, list);
  writeunlock (&cobj->lock);
}

void
cacheobj_delmapping (struct cacheobj *cobj, struct cacheobj_mapping *cobjm)
{
  printf ("CACHEOBJECT: delete region to object: %lx %lx\n", cobjm->start,
	  cobjm->size);
  writelock (&cobj->lock);
  LIST_REMOVE (cobjm, list);
  for (vaddr_t i = cobjm->start; i < cobjm->start + cobjm->size;
       i += PAGE_SIZE)
    umap_unmap (cobjm->umap, i);
  umap_commit (cobjm->umap);
  writeunlock (&cobj->lock);

}

ipte_t
cacheobj_map (struct cacheobj *cobj, mcn_vmoff_t off, pfn_t pfn,
	      bool roshared, mcn_vmprot_t protmask)
{
  ipte_t ret;
  off = trunc_page (off);

  printf
    ("CACHEOBJ: mapping object %p offset %lx with pfn %lx (roshared: %d prot: %x)\n",
     cobj, off, pfn, roshared, protmask);
  writelock (&cobj->lock);
  ret = imap_map (&cobj->map, off, pfn, roshared, protmask);
#if 0
  LIST_FOREACH (cobjm, &cobj->mappings, list)
  {
    mcn_vmoff_t obj_offstart = trunc_page (cobjm->off);
    mcn_vmoff_t obj_offend = round_page (cobjm->off + cobjm->size);
    printf ("CACHEOBJ: attempting mapping %lx to va %lx [%lx %lx]\n",
	    pfn, cobjm->start + off - obj_offstart, obj_offstart, obj_offend);
    if ((off < obj_offstart) || (off >= obj_offend))
      continue;
    printf ("CACHEOBJ: mapping %lx to va %lx (%s)\n",
	    pfn, cobjm->start + off - obj_offstart,
	    writable ? "writable" : "read-only");
    umap_map (cobjm->umap, cobjm->start + off - obj_offstart, pfn,
	      HAL_PTE_P | HAL_PTE_U | (writable ? HAL_PTE_W : 0), NULL);
    umap_commit (cobjm->umap);
  }
#endif
  writeunlock (&cobj->lock);

  return ret;
}

ipte_t
cacheobj_lookup (struct cacheobj *cobj, mcn_vmoff_t off)
{
  ipte_t ret;

  readlock (&cobj->lock);
  ret = imap_lookup (&cobj->map, off);
  readunlock (&cobj->lock);
  return ret;
}

void
cacheobj_init (struct cacheobj *cobj, size_t size)
{
  rwlock_init (&cobj->lock);
  cobj->size = size;
  imap_init (&cobj->map);
  LIST_INIT (&cobj->mappings);
}
