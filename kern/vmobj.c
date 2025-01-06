/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include "vm.h"

#ifndef VMOBJ_DEBUG
#define VMOBJ_PRINT(...)
#else
#define VMOBJ_PRINT printf
#endif

struct slab vmobjlocks;
struct slab vmobjs;
struct slab cobjmappings;

static bool
_default_pgreq_empty(void *opq, struct cacheobj *cobj, mcn_vmoff_t off, mcn_vmprot_t reqprot)
{
  /*
    Zero-fill objects. No protection mask by the pager.
  */
  memcache_zeropage_new (cobj, off, true, MCN_VMPROT_NONE);
  return true;
}

static struct objpager _default_pager =
  {
    .opq = NULL,
    .pgreq_empty = _default_pgreq_empty,
  };


unsigned long *
vmobj_refcnt (struct vmobj *vmobj)
{
  return &vmobj->_ref_count;
}

/*
  Assumption: We have just booted, and we're running on the BSP. The
  userspace binary of the bootstrap process is mapped.
*/
struct vmobjref
vmobj_bootstrap (uaddr_t *outbase, size_t *outsize)
{
  struct vmobjref ref;
  hal_l1e_t base_l1e;
  uaddr_t base, i;
  size_t size;

  /*
    First Loop: Find base and size.
  */
  size = 0;
  base = hal_umap_next (NULL, 0, NULL, &base_l1e);
  for (i = base; i != UADDR_INVALID; i = hal_umap_next (NULL, i, NULL, NULL))
    size = i - base;
  size += PAGE_SIZE;

  /*
    Create object.
  */
  ref = vmobj_new (NULL, size);
  struct vmobj *vmobj = vmobjref_unsafe_get (&ref);

  VMOBJ_PRINT ("VMOBJ %p BOOTSTRAP: BASE %lx Size %lx\n",
	       vmobj, base, size);

  /*
    Second loop: Add pages to object.
  */
  hal_l1e_t l1e = base_l1e;
  for (i = base; i != UADDR_INVALID; i = hal_umap_next (NULL, i, NULL, &l1e))
    {
      pfn_t pfn;
      unsigned flags;
      mcn_vmprot_t protmask;

      VMOBJ_PRINT ("VMOBJ %p BOOTSTRAP: ADDR %lx PTE %lx\n", vmobj, i, l1e);

      hal_l1e_unbox (l1e, &pfn, &flags);
      assert (pfn != PFN_INVALID);
      assert (flags & HAL_PTE_P);
      assert (flags & HAL_PTE_U);
      protmask = 0;
      protmask |= (flags & HAL_PTE_W) ? 0 : MCN_VMPROT_WRITE;
      protmask |= (flags & HAL_PTE_X) ? 0 : MCN_VMPROT_EXECUTE;

      memcache_existingpage (&vmobj->cobj, i - base, pfn, protmask);
    }

  cpu_umap_exit();
  *outbase = base;
  *outsize = size;
  return ref;
}

struct vmobjref
vmobj_new (struct objpager *pager, size_t size)
{
  struct vmobj *obj;
  struct vmobjref ref;

  obj = slab_alloc (&vmobjs);
  obj->lock = slab_alloc (&vmobjlocks);
  VMOBJ_PRINT("VMOBJ %p: Alloc lock %p\n", obj, obj->lock);
  port_alloc_kernel ((void *) obj, KOT_VMOBJ, &obj->control_port);
  port_alloc_kernel ((void *) obj, KOT_VMOBJ_NAME, &obj->name_port);
  spinlock_init (obj->lock);

  if (pager == NULL)
    obj->pager = &_default_pager;
  else
    obj->pager = pager;

  cacheobj_init (&obj->cobj, size);
  obj->shadow = VMOBJREF_NULL;
  obj->copy = NULL;
  obj->_ref_count = 1;
  ref.obj = obj;
  return ref;
}

struct vmobjref
vmobj_shadowcopy (struct vmobjref *ref)
{
  struct vmobj *new = slab_alloc (&vmobjs);
  struct vmobj *obj = vmobjref_unsafe_get (ref);

  spinlock (obj->lock);
  new->lock = obj->lock;	/* Shadow chains share the same lock. */
  cacheobj_shadow (&obj->cobj, &new->cobj);
  port_alloc_kernel ((void *) new, KOT_VMOBJ, &new->control_port);
  port_alloc_kernel ((void *) new, KOT_VMOBJ_NAME, &new->name_port);

  struct vmobjref newref = (struct vmobjref)
  {.obj = new, };
  new->_ref_count = 1;

  /* Insert new object between current object and its copy. */
  new->shadow = REF_DUP (*ref);
  new->copy = obj->copy;
  obj->copy = new;
  if (new->copy)
    new->copy->shadow = REF_DUP (newref);

  spinunlock (obj->lock);

  return newref;
}

static void
_cacheobj_unlink_page (struct cacheobj *cobj, unsigned long off, ipte_t *ipte)
{
  assert (ipte->p);
  VMOBJ_PRINT("CACHEOBJ: UNLINKING OBJ %p OFF %d ipte: %lx\n",
	 cobj, off, *ipte);

  memcache_cobjremove (ipte_pfn (ipte), cobj, off);
}

void
vmobj_zeroref (struct vmobj *obj)
{
  VMOBJ_PRINT("VMOBJ %p: CTRL PORT IS %p\n", obj, portref_unsafe_get(&obj->control_port));
  VMOBJ_PRINT("VMOBJ %p: NAME PORT IS %p\n", obj, portref_unsafe_get(&obj->name_port));
  if (vmobjref_isnull(&obj->shadow))
    {
      VMOBJ_PRINT("VMOBJ %p: FREE LOCK %p\n", obj, obj->lock);
      slab_free((void *)obj->lock);
    }
  else
    {
      VMOBJ_PRINT("VMOBJ %p: CONSUME SHADOW  %p\n", obj, vmobjref_unsafe_get(&obj->shadow));
      vmobjref_consume(&obj->shadow);
    }
  VMOBJ_PRINT("VMOBJ %p: UNLINK CTRL PORT %p\n", obj, portref_unsafe_get(&obj->control_port));
  port_unlink_kernel(&obj->control_port);
  VMOBJ_PRINT("VMOBJ %p: UNLINK NAME PORT %p\n", obj, portref_unsafe_get(&obj->name_port));
  port_unlink_kernel(&obj->name_port);
  VMOBJ_PRINT("VMOBJ %p: DESTROY CACHEOBJ %p\n", obj, &obj->cobj);

  cacheobj_foreach(&obj->cobj, (void (*)(void *, mcn_vmoff_t, ipte_t *))_cacheobj_unlink_page);
  cacheobj_destroy(&obj->cobj);
  slab_free(obj);
}

/*
  Request the PFN for the object, with permission reqprot.
*/
bool
vmobj_fault (struct vmobj *tgtobj, mcn_vmoff_t off, mcn_vmprot_t reqprot,
	     struct vm_region *vmreg)
{
  bool ret;
  ipte_t ipte;
  struct vmobj *vmobj;
#define PAGER(_o) ((_o)->pager)
#define PAGER_OPQ(_o) ((_o)->pager->opq)

  spinlock (tgtobj->lock);
  vmobj = tgtobj;

  nuxperf_inc (&pmachina_vmobj_faults);
  VMOBJ_PRINT ("=== VMOBJ: FAULT FOR OBJECT %p START: COBJ %p OFF %lx ===\n",
	       tgtobj, &vmobj->cobj, off);

 _fault_redo:

  /*
    First, make sure that the page table is up-to-date with the cache object.
  */
  ipte = cacheobj_updatemapping (&vmobj->cobj, off, vmreg->cobjm);

  VMOBJ_PRINT ("IPTE IS %" PRIx64 "\n", ipte.raw);
  switch (ipte_status (&ipte))
    {
    case STIPTE_EMPTY:
      /*
	Never accessed this page before.
      */

      nuxperf_inc (&pmachina_vmobj_fault_empty);
      VMOBJ_PRINT ("IPTE EMPTY");
      if (vmobjref_unsafe_get (&vmobj->shadow) != NULL)
	{
	  /*
	    If we have a shadow, search for a page there.
	  */
	  nuxperf_inc (&pmachina_vmobj_fault_empty_shdw);
	  vmobj = vmobjref_unsafe_get (&vmobj->shadow);
	  goto _fault_redo;
	}
      else
	{
	  /*
	    Issue a request to the pager.
	  */
	  nuxperf_inc (&pmachina_vmobj_pgreq_empty);
	  VMOBJ_PRINT("PAGER %p REQUEST\n", vmobj->pager);
	  ret = PAGER(vmobj)->pgreq_empty(PAGER_OPQ(vmobj), &tgtobj->cobj, off, reqprot);
	}
      break;

    case STIPTE_PAGINGIN:
      nuxperf_inc (&pmachina_vmobj_pgreq_pgin);
      VMOBJ_PRINT ("IPTE PAGING IN");
      ret = PAGER(vmobj)->pgreq_pgin(PAGER_OPQ(vmobj), &tgtobj->cobj, off, reqprot);
      break;

    case STIPTE_PAGINGOUT:
      nuxperf_inc (&pmachina_vmobj_pgreq_pgout);
      VMOBJ_PRINT ("IPTE PAGING OUT");
      ret = PAGER(vmobj)->pgreq_pgout(PAGER_OPQ(vmobj), &tgtobj->cobj, off, reqprot);
      break;

    case STIPTE_PAGED:
      nuxperf_inc (&pmachina_vmobj_pgreq_paged);
      VMOBJ_PRINT ("IPTE PAGED");
      ret = PAGER(vmobj)->pgreq_paged(PAGER_OPQ(vmobj), &tgtobj->cobj, off, reqprot);
      break;

    case STIPTE_ROSHARED:
      nuxperf_inc (&pmachina_vmobj_fault_ro);
      VMOBJ_PRINT ("IPTE ROSHARED (reqprot: %x, protmask: %x)", reqprot,
	    ipte_protmask (&ipte));

      if (reqprot & ipte_protmask (&ipte))
	{
	  nuxperf_inc (&pmachina_vmobj_fault_ro_unlock);
	  ret = false;
	  break;
	}

      VMOBJ_PRINT ("IPTE ROSHARED IS SHADOW? %d\n", tgtobj != vmobj);
      if (tgtobj != vmobj)
	{
	  /*
	     If we are in shadow fault, we have to share it with the
	     target imap fisrt.
	   */
	  nuxperf_inc (&pmachina_vmobj_fault_ro_shdw);
	  memcache_share (ipte_pfn (&ipte), &tgtobj->cobj, off,
			  ipte_protmask (&ipte));
	}
      if (reqprot & MCN_VMPROT_WRITE)
	{
	  if (tgtobj->copy)
	    {
	      struct vmobj *copy_obj = tgtobj->copy;
	      ipte_t copy_ipte = cacheobj_lookup (&copy_obj->cobj, off);

	      VMOBJ_PRINT ("PUSHING PFN %lx to OBJ %p OFF %lx (ipte: %"
		      PRIx64 "\n", ipte_pfn (&ipte), &copy_obj->cobj, off,
		      ipte.raw);
	      if (ipte_empty (&copy_ipte))
		{
		  nuxperf_inc (&pmachina_vmobj_fault_ro_push);
		  memcache_share (ipte_pfn (&ipte), &copy_obj->cobj, off,
				  ipte_protmask (&ipte));
		}
	    }
	  nuxperf_inc (&pmachina_vmobj_fault_ro_unshare);
	  memcache_unshare (ipte_pfn (&ipte), &tgtobj->cobj, off,
			    ipte_protmask (&ipte));
	}
      else
	ipte_pfn (&ipte);
      ret = true;
      break;
    case STIPTE_PRIVATE:
      nuxperf_inc (&pmachina_vmobj_fault_priv);
      VMOBJ_PRINT ("IPTE PRIVATE (reqprot: %x, protmask: %x", reqprot,
	    ipte_protmask (&ipte));
      assert (tgtobj == vmobj);	/* A shadowed page cannot point to a private page. */
      if (reqprot & ipte_protmask (&ipte))
	{
	  nuxperf_inc (&pmachina_vmobj_fault_priv_unlock);
	  ret = false;
	  break;
	}
      ipte_pfn (&ipte);
      ret = true;
      break;
    }

  VMOBJ_PRINT ("VMOBJ FAULT END =============================\n");

  spinunlock (tgtobj->lock);
  return ret;

#undef PAGER_OPQ
#undef PAGER
}

void
vmobj_addregion (struct vmobj *vmobj, struct vm_region *vmreg,
		 struct umap *umap)
{
  struct cacheobj_mapping *cobjm;
  assert (vmreg->type == VMR_TYPE_USED);

  cobjm = slab_alloc (&cobjmappings);
  cobjm->umap = umap;
  cobjm->start = vmreg->start;
  cobjm->size = vmreg->size;
  cobjm->off = vmreg->off;
  vmreg->cobjm = cobjm;
  spinlock (vmobj->lock);
  cacheobj_addmapping (&vmobj->cobj, cobjm);
  spinunlock (vmobj->lock);
}

void
vmobj_delregion (struct vmobj *vmobj, struct vm_region *vmreg)
{
  assert (vmreg->type == VMR_TYPE_USED);
  spinlock (vmobj->lock);
  cacheobj_delmapping (&vmobj->cobj, vmreg->cobjm);
  slab_free (vmreg->cobjm);
  vmreg->cobjm = NULL;
  spinunlock (vmobj->lock);
}

struct portref
vmobj_getctrlport (struct vmobj *vmobj)
{
  struct portref pr;

  spinlock (vmobj->lock);
  pr = portref_dup (&vmobj->control_port);
  spinunlock (vmobj->lock);
  return pr;
}

struct portref
vmobj_getnameport (struct vmobj *vmobj)
{
  struct portref pr;

  spinlock (vmobj->lock);
  pr = portref_dup (&vmobj->name_port);
  spinunlock (vmobj->lock);
  return pr;
}

void
vmobj_init (void)
{
  slab_register (&vmobjs, "VMOBJS", sizeof (struct vmobj), NULL, 0);
  slab_register (&vmobjlocks, "VMOBJSLOCKS", sizeof (lock_t), NULL, 0);
  slab_register (&cobjmappings, "COBJMAPPINGS",
		 sizeof (struct cacheobj_mapping), NULL, 0);
}
