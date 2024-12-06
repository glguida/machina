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

unsigned long *
vmobj_refcnt (struct vmobj *vmobj)
{
  return &vmobj->_ref_count;
}

struct vmobjref
vmobj_new (bool private, size_t size)
{
  struct vmobj *obj;
  struct vmobjref ref;

  obj = slab_alloc (&vmobjs);
  obj->lock = slab_alloc (&vmobjlocks);
  port_alloc_kernel ((void *) obj, KOT_VMOBJ, &obj->control_port);
  port_alloc_kernel ((void *) obj, KOT_VMOBJ_NAME, &obj->name_port);
  spinlock_init (obj->lock);
  obj->private = private;
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
  obj->private = true;

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


/*
  Insert a zero page in the VM object.

  If we're requesting a writable map, push the shared zero page to the copy.
*/
static pfn_t
_vmobj_insertzeropage (struct vmobj *obj, mcn_vmoff_t off,
		       mcn_vmprot_t reqprot)
{
  if (reqprot & MCN_VMPROT_WRITE)
    {
      if (obj->copy)
	{
	  struct vmobj *copy_obj = obj->copy;
	  ipte_t copy_ipte = cacheobj_lookup (&copy_obj->cobj, off);
	  printf ("VMOBJ: PUSHING ZERO PAGE to OBJ %p OFF %lx (ipte: %" PRIx64
		  "\n", &copy_obj->cobj, off, copy_ipte.raw);
	  if (ipte_empty (&copy_ipte))
	    memcache_zeropage_new (&copy_obj->cobj, off, true,
				   MCN_VMPROT_NONE);
	}
      return memcache_zeropage_new (&obj->cobj, off, false, MCN_VMPROT_NONE);
    }
  else
    return memcache_zeropage_new (&obj->cobj, off, true, MCN_VMPROT_NONE);
}


/*
  Request the PFN for the object, with permission reqprot.
*/
bool
vmobj_fault (struct vmobj *tgtobj, mcn_vmoff_t off, mcn_vmprot_t reqprot,
	     pfn_t * outpfn)
{
  bool ret;
  ipte_t ipte;
  struct vmobj *vmobj;

  spinlock (tgtobj->lock);
  printf ("VMOBJ: FAULT FOR OBJECT %p START =============================\n",
	  tgtobj);
  vmobj = tgtobj;
_fault_redo:

  info ("PAGE FAULT AT CACHE OBJECT %p OFFSET %lx\n", &vmobj->cobj, off);
  ipte = cacheobj_lookup (&vmobj->cobj, off);

  info ("IPTE IS %" PRIx64 "\n", ipte.raw);
  switch (ipte_status (&ipte))
    {
    case STIPTE_EMPTY:
      info ("IPTE EMPTY");

      /*
         If we have a shadow, search for a page there.
       */
      if (vmobjref_unsafe_get (&vmobj->shadow) != NULL)
	{
	  vmobj = vmobjref_unsafe_get (&vmobj->shadow);
	  goto _fault_redo;
	}
      else if (vmobj->private)
	{
	  /*
	     Private objects can request zero pages directly.
	   */
	  *outpfn = _vmobj_insertzeropage (tgtobj, off, reqprot);
	  ret = true;
	}
      else
	{
	  /*
	     XXX: PAGER REQUEST HERE.
	   */
	  fatal ("PAGER REQUEST UNIMPLEMENTED.");
	  ret = false;
	}
      break;

    case STIPTE_PAGINGIN:
      info ("IPTE PAGING IN");
      /*
         XXX: WAIT.
       */
      fatal ("PAGEIN UNIMPLEMENTED.");
      ret = false;
      break;

    case STIPTE_PAGINGOUT:
      info ("IPTE PAGING OUT");
      /*
         XXX: WAIT.
       */
      fatal ("PAGEIN UNIMPLEMENTED.");
      ret = false;
      break;

    case STIPTE_PAGED:
      info ("IPTE PAGED");

      /*
         XXX: PAGER REQUEST.
       */
      fatal ("PAGER REQUEST UNIMPLEMENTED.");
      ret = false;
      break;

    case STIPTE_ROSHARED:
      info ("IPTE ROSHARED (reqprot: %x, protmask: %x)", reqprot,
	    ipte_protmask (&ipte));
      if (reqprot & ipte_protmask (&ipte))
	{
	  ret = false;
	  break;
	}
      printf ("IPTE ROSHARED IS SHADOW? %d\n", tgtobj != vmobj);
      if (tgtobj != vmobj)
	{
	  /*
	     If we are in shadow fault, we have to share it with the
	     target imap fisrt.
	   */
	  memcache_share (ipte_pfn (&ipte), &tgtobj->cobj, off,
			  ipte_protmask (&ipte));
	}
      if (reqprot & MCN_VMPROT_WRITE)
	{
	  if (tgtobj->copy)
	    {
	      struct vmobj *copy_obj = tgtobj->copy;
	      ipte_t copy_ipte = cacheobj_lookup (&copy_obj->cobj, off);
	      printf ("VMOBJ: PUSHING PFN %lx to OBJ %p OFF %lx (ipte: %"
		      PRIx64 "\n", ipte_pfn (&ipte), &copy_obj->cobj, off,
		      ipte.raw);
	      if (ipte_empty (&copy_ipte))
		memcache_share (ipte_pfn (&ipte), &copy_obj->cobj, off,
				ipte_protmask (&ipte));
	    }
	  *outpfn =
	    memcache_unshare (ipte_pfn (&ipte), &tgtobj->cobj, off,
			      ipte_protmask (&ipte));
	}
      else
	*outpfn = ipte_pfn (&ipte);
      ret = true;
      break;
    case STIPTE_PRIVATE:
      info ("IPTE PRIVATE (reqprot: %x, protmask: %x", reqprot,
	    ipte_protmask (&ipte));
      assert (tgtobj == vmobj);	/* A shadowed page cannot point to a private page. */
      if (reqprot & ipte_protmask (&ipte))
	{
	  ret = false;
	  break;
	}
      *outpfn = ipte_pfn (&ipte);
      ret = true;
      break;
    }
  printf ("VMOBJ FAULT END =============================\n");
  spinunlock (tgtobj->lock);
  return ret;
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


#if 0
/*
  Assumption: We have just booted, and we're running on the BSP. The
  userspace binary of the bootstrap process is mapped.
*/
uaddr_t
memobj_bootstrap (struct memobj **out)
{
  volatile struct vmobj *obj;
  struct pglist clean, dirty;

  pglist_init (&clean);
  pglist_init (&dirty);

  mobj = memobj_new (true);

  hal_l1e_t l1e;
  uaddr_t i, base;
  uaddr_t maxaddr = 0;
  i = hal_umap_next (NULL, 0, NULL, &l1e);
  base = i;
  while (i != UADDR_INVALID)
    {
      maxaddr = i;
      printf ("%lx -> %lx\n", i - base, l1e);
      _memobj_addl1e ((struct memobj *) mobj, i - base, l1e, &clean, &dirty);
      i = hal_umap_next (NULL, i, NULL, &l1e);
    }

  maxaddr += PAGE_SIZE;
  mobj->size = maxaddr;
  if (out)
    *out = (struct memobj *) mobj;

  assert (pglist_pages (&clean) == 0);
  assert (pglist_pages (&dirty) == 0);
  return base;
}
#endif
