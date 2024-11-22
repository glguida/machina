/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include "vm.h"

struct slab vmobjlocks;
struct slab vmobjs;
struct slab cobjmappings;

struct vmobjref
vmobj_new(bool private, size_t size)
{
  struct vmobj *obj;
  struct vmobjref ref;

  obj = slab_alloc(&vmobjs);
  obj->lock = slab_alloc(&vmobjlocks);
  spinlock_init(obj->lock);
  obj->private = private;
  cacheobj_init (&obj->cobj, size);
  obj->shadow = VMOBJREF_NULL;
  obj->copy = VMOBJREF_NULL;
  obj->_ref_count = 1;
  ref.obj = obj;
  return ref;
}

/*
  Request the PFN for the object, with permission reqprot.
*/
bool
vmobj_fault(struct vmobj *vmobj, mcn_vmoff_t off, mcn_vmprot_t reqprot, pfn_t *outpfn)
{
  bool ret;
  ipte_t ipte;

  spinlock(vmobj->lock);
  info("PAGE FAULT AT CACHE OBJECT %p OFFSET %lx\n", &vmobj->cobj, off);
  ipte = cacheobj_lookup(&vmobj->cobj, off);
  info("IPTE IS %lx\n", ipte.raw);
  switch (ipte_status(&ipte))
    {
    case STIPTE_EMPTY:
      info("IPTE EMPTY");
      /*
	XXX: WALK SHADOW HERE.
      */
      if (vmobj->private)
	{
	  /*
	    Private objects can request zero pages with no permission limits.
	  */
	  *outpfn = memcache_zeropage_new (&vmobj->cobj, off, reqprot & MCN_VMPROT_WRITE ? false : true, MCN_VMPROT_NONE);
	  ret = true;
	}
      else
	{
	  /*
	    XXX: PAGER REQUEST HERE.
	  */
	  fatal("PAGER REQUEST UNIMPLEMENTED.");
	  ret = false;
	}
      break;

    case STIPTE_PAGINGIN:
      info("IPTE PAGING IN");
      /*
	XXX: WAIT.
      */
      fatal ("PAGEIN UNIMPLEMENTED.");
      ret = false;
      break;

    case STIPTE_PAGINGOUT:
      info("IPTE PAGING OUT");
      /*
	XXX: WAIT.
      */
      fatal ("PAGEIN UNIMPLEMENTED.");
      ret = false;
      break;

    case STIPTE_PAGED:
      info("IPTE PAGED");
      
      /*
	XXX: PAGER REQUEST.
      */
      fatal("PAGER REQUEST UNIMPLEMENTED.");
      ret = false;
      break;

    case STIPTE_ROSHARED:
      info("IPTE ROSHARED (reqprot: %x, protmask: %x", reqprot, ipte_protmask(&ipte));
      if (reqprot & ipte_protmask(&ipte))
	{
	  ret = false;
	  break;
	}
      if (reqprot & MCN_VMPROT_WRITE)
	*outpfn = memcache_unshare(ipte_pfn(&ipte), &vmobj->cobj, off, ipte_protmask(&ipte));
      else
	*outpfn = ipte_pfn(&ipte);
      ret = true;
      break;
    case STIPTE_PRIVATE:
      info("IPTE PRIVATE (reqprot: %x, protmask: %x", reqprot, ipte_protmask(&ipte));
      if (reqprot & ipte_protmask(&ipte))
	{
	  ret = false;
	  break;
	}
      *outpfn = ipte_pfn(&ipte);
      ret = true;
      break;
    }
  spinunlock(vmobj->lock);
  return ret;
}

void
vmobj_addregion(struct vmobj *vmobj, struct vm_region *vmreg, struct umap *umap)
{
  struct cacheobj_mapping *cobjm;
  assert (vmreg->type == VMR_TYPE_USED);

  cobjm = slab_alloc(&cobjmappings);
  cobjm->umap = umap;
  cobjm->start = vmreg->start;
  cobjm->size = vmreg->size;
  cobjm->off = vmreg->off;
  vmreg->cobjm = cobjm;
  spinlock(vmobj->lock);
  cacheobj_addmapping(&vmobj->cobj, cobjm);
  spinunlock(vmobj->lock);
}

void
vmobj_delregion(struct vmobj *vmobj, struct vm_region *vmreg)
{
  assert (vmreg->type == VMR_TYPE_USED);
  spinlock(vmobj->lock);
  cacheobj_delmapping(&vmobj->cobj, vmreg->cobjm);
  slab_free(vmreg->cobjm);
  vmreg->cobjm = NULL;
  spinunlock(vmobj->lock);
}

void
vmobj_init(void)
{
  slab_register(&vmobjs, "VMOBJS", sizeof(struct vmobj), NULL, 0);
  slab_register(&vmobjlocks, "VMOBJSLOCKS", sizeof(lock_t), NULL, 0);
  slab_register(&cobjmappings, "COBJMAPPINGS", sizeof(struct cacheobj_mapping), NULL, 0);
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
