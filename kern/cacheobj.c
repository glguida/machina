/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include "vm.h"

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

static void
ipte_unbox (ipte_t ipte, pfn_t *outpfn, unsigned *outflags)
{
  unsigned flags;
  mcn_vmprot_t prot;
  bool p, w, x;

  COBJ_PRINT ("IPTE IS %lx (pfn: %lx protmask: %lx p: %d) ",
	      ipte, ipte_present (&ipte) ? ipte_pfn (&ipte) : 0,
	      ipte_present (&ipte) ? ipte_protmask (&ipte) : 0,
	      ipte_present (&ipte));

  if (!ipte_present (&ipte))
    {
      COBJ_PRINT ("\n");
      if (outpfn)
	*outpfn = PFN_INVALID;
      if (outflags)
	*outflags = 0;
      return;
    }

  prot = (MCN_VMPROT_ALL & ~ipte_protmask (&ipte));

  p = prot & MCN_VMPROT_READ;
  w = !ipte_roshared (&ipte) && (prot &  MCN_VMPROT_WRITE);
  x = prot & MCN_VMPROT_EXECUTE;

  COBJ_PRINT ("P: %d W: %d X: %d ", p, w, x);

  if (!p)
    {
      COBJ_PRINT ("\n");
      if (outpfn)
	*outpfn = PFN_INVALID;
      if (outflags)
	*outflags = 0;
      return;
    }

  flags = HAL_PTE_U | HAL_PTE_P;
  flags |= w ? HAL_PTE_W : 0;
  flags |= x ? HAL_PTE_X : 0;

  COBJ_PRINT ("FLAGS: %lx\n", flags);

  if (outpfn)
    *outpfn = ipte_pfn (&ipte);
  if (outflags)
    *outflags = flags;
}

void
cacheobj_addmapping (struct cacheobj *cobj, struct cacheobj_mapping *cobjm)
{
  nuxperf_inc (&pmachina_cacheobj_addmapping);

  COBJ_PRINT ("CACHEOBJECT: %p: adding region to object: %lx %lx\n", cobj,
	  cobjm->start, cobjm->size);

  writelock (&cobj->lock);
  LIST_INSERT_HEAD (&cobj->mappings, cobjm, list);
  /*
    Unneeded really, but let's be paranoid for now.
  */
  for (vaddr_t i = cobjm->start; i < cobjm->start + cobjm->size;
       i += PAGE_SIZE)
    {
      umap_unmap (cobjm->umap, i);
    }
  umap_commit (cobjm->umap);
  writeunlock (&cobj->lock);
}

ipte_t
cacheobj_updatemapping (struct cacheobj *cobj, mcn_vmoff_t off, struct cacheobj_mapping *cobjm)
{
  mcn_vmoff_t obj_offstart = trunc_page (cobjm->off);
  mcn_vmoff_t obj_offend = round_page (cobjm->off + cobjm->size);
 ipte_t ipte;
 pfn_t pfn;
 unsigned flags;

 nuxperf_inc (&pmachina_cacheobj_updatemapping);

 off = trunc_page (off);

 COBJ_PRINT ("CACHEOBJ: %p: filling-in cobj-mapping %p\n",
	     cobj, off, cobjm);

 writelock (&cobj->lock);
 ipte = imap_lookup (&cobj->map, off);
 ipte_unbox (ipte, &pfn, &flags);
 assert (off >= obj_offstart);
 assert (off < obj_offend);

 COBJ_PRINT ("UMAP %p: map at va %lx pfn %lx with flags %d\n",
	     cobjm->umap, cobjm->start + off - obj_offstart, pfn, flags);
 umap_map (cobjm->umap, cobjm->start + off - obj_offstart, pfn, flags, NULL);
 umap_commit (cobjm->umap);
 writeunlock (&cobj->lock);

 return ipte;
}

void
cacheobj_delmapping (struct cacheobj *cobj, struct cacheobj_mapping *cobjm)
{
  COBJ_PRINT ("CACHEOBJECT: %p: delete region to object: %lx %lx\n", cobj,
	  cobjm->start, cobjm->size);

  nuxperf_inc (&pmachina_cacheobj_delmapping);

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

  nuxperf_inc (&pmachina_cacheobj_shadow);

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
  ipte_t old;
  struct cacheobj_mapping *cobjm;

  nuxperf_inc (&pmachina_cacheobj_map);

  off = trunc_page (off);

  COBJ_PRINT
    ("CACHEOBJ: %p: mapping offset %lx with pfn %lx (roshared: %d prot: %x)\n",
     cobj, off, pfn, roshared, protmask);

  writelock (&cobj->lock);
  old = imap_map (&cobj->map, off, pfn, roshared, protmask);

  /*
    Update all cobj mappings.
  */

  LIST_FOREACH (cobjm, &cobj->mappings, list)
  {
    mcn_vmoff_t obj_offstart = trunc_page (cobjm->off);
    mcn_vmoff_t obj_offend = round_page (cobjm->off + cobjm->size);
    if ((off < obj_offstart) || (off >= obj_offend))
      continue;
    COBJ_PRINT ("CACHEOBJ %p: Clearing umap %p va %lx (off %lx)\n",
	    cobj, cobjm->umap, cobjm->start + off - obj_offstart, off);
    umap_unmap (cobjm->umap, cobjm->start + off - obj_offstart);
    umap_commit (cobjm->umap);
  }
  writeunlock (&cobj->lock);

  return old;
}

ipte_t
cacheobj_lookup (struct cacheobj *cobj, mcn_vmoff_t off)
{
  ipte_t ret;

  nuxperf_inc (&pmachina_cacheobj_lookup);

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
