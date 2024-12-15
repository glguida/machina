/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include "vm.h"

//#define CACHEOBJ_DEBUG
#ifndef CACHEOBJ_DEBUG
#define COBJ_PRINT(...)
#else
#define COBJ_PRINT(...) printf(__VA_ARGS__)
#endif

void
_ipte_print (unsigned long off, ipte_t * ipte)
{
  COBJ_PRINT ("CACHEOBJ: DUMP: %lx %lx\n", off, *ipte);
}

void
cacheobj_addmapping (struct cacheobj *cobj, struct cacheobj_mapping *cobjm)
{

  COBJ_PRINT ("CACHEOBJECT: %p: adding region to object: %lx %lx\n", cobj,
	  cobjm->start, cobjm->size);

  writelock (&cobj->lock);
  LIST_INSERT_HEAD (&cobj->mappings, cobjm, list);
  writeunlock (&cobj->lock);
}

void
cacheobj_delmapping (struct cacheobj *cobj, struct cacheobj_mapping *cobjm)
{
  COBJ_PRINT ("CACHEOBJECT: %p: delete region to object: %lx %lx\n", cobj,
	  cobjm->start, cobjm->size);

  writelock (&cobj->lock);
  LIST_REMOVE (cobjm, list);
  for (vaddr_t i = cobjm->start; i < cobjm->start + cobjm->size;
       i += PAGE_SIZE)
    {
      umap_unmap (cobjm->umap, i);
    }
  umap_commit (cobjm->umap);
  writeunlock (&cobj->lock);
}

static void
_ipte_roshare (unsigned long off, ipte_t * ipte)
{
  assert (ipte->p);
  ipte->roshared = 1;
}

void
cacheobj_shadow (struct cacheobj *orig, struct cacheobj *shadow)
{
  COBJ_PRINT ("CACHEOBJ: shadow %p to %p\n", orig, shadow);

  writelock (&orig->lock);
  /* shadow is unitialised. Shouldn't get the lock. */
  cacheobj_init (shadow, orig->size);
  imap_foreach (&orig->map, _ipte_roshare);
  writeunlock (&orig->lock);
}

ipte_t
cacheobj_map (struct cacheobj *cobj, mcn_vmoff_t off, pfn_t pfn,
	      bool roshared, mcn_vmprot_t protmask)
{
  ipte_t ret;
  off = trunc_page (off);

  COBJ_PRINT
    ("CACHEOBJ: %p: mapping offset %lx with pfn %lx (roshared: %d prot: %x)\n",
     cobj, off, pfn, roshared, protmask);

  writelock (&cobj->lock);
  ret = imap_map (&cobj->map, off, pfn, roshared, protmask);
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

static void
_ipte_unlink_page (void *cobjopq, unsigned long off, ipte_t *ipte)
{
  assert (ipte->p);
  COBJ_PRINT("CACHEOBJ: UNLINKING OBJ %p OFF %d ipte: %lx\n",
	 cobjopq, off, *ipte);
  memcache_cobjremove (ipte_pfn (ipte), cobjopq, off);
}

void
cacheobj_destroy (struct cacheobj *cobj)
{
  imap_free (&cobj->map, _ipte_unlink_page, cobj);
  assert (LIST_EMPTY(&cobj->mappings));
}

void
cacheobj_init (struct cacheobj *cobj, size_t size)
{
  rwlock_init (&cobj->lock);
  cobj->size = size;
  imap_init (&cobj->map);
  LIST_INIT (&cobj->mappings);
}
