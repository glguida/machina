/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef MACHINA_VM_H
#define MACHINA_VM_H

#include <rbtree.h>
#include <nux/hal.h>
#include <machina/types.h>

#include "ref.h"

enum ipte_npstatus
{
  IPTE_EMPTY = 0,
  IPTE_PAGINGIN = 1,
  IPTE_PAGINGOUT = 2,
  IPTE_PAGED = 3,
};

typedef union
{
  struct
  {
    /* Following fields are valid when p = 1. */
    uint64_t pfn:56;
    uint64_t roshared:1;
    uint64_t protmask:3;
    /* Following fields are valid when p = 0. */
    uint64_t status:3;
    /* Present bit. */
    uint64_t p:1;
  };
  uint64_t raw;
} ipte_t;

enum ipte_status
{
  STIPTE_EMPTY = 0,		/* Page has never been seen by the kernel. */
  STIPTE_PAGINGIN = 1,		/* Page is being paged in. */
  STIPTE_PAGINGOUT = 2,		/* Page is being paged out. */
  STIPTE_PAGED = 3,		/* Page is not cached in memory. */
  STIPTE_ROSHARED = 4,		/* Page is present and shared with other objects. */
  STIPTE_PRIVATE = 5,		/* Page is present and mapped only by this object. */
};

static inline bool
ipte_empty (ipte_t * i)
{
  return ((i->p == 0) && (i->status == IPTE_EMPTY));
}

static inline bool
ipte_pagingin (ipte_t * i)
{
  return ((i->p == 0) && (i->status == IPTE_PAGINGIN));
}

static inline bool
ipte_pagingout (ipte_t * i)
{
  return ((i->p == 0) && (i->status == IPTE_PAGINGOUT));
}

static inline bool
ipte_paged (ipte_t * i)
{
  return ((i->p == 0) && (i->status == IPTE_PAGED));
}

static inline bool
ipte_present (ipte_t * i)
{
  return i->p;
}

static inline bool
ipte_roshared (ipte_t * i)
{
  return i->p && i->roshared;
}

static inline bool
ipte_private (ipte_t * i)
{
  return i->p && !i->roshared;
}

static inline mcn_vmprot_t
ipte_protmask (ipte_t * i)
{
  assert (i->p);
  return i->protmask;
}

#define ipte_pfn(_i) ({ assert ((_i)->p); (_i)->pfn; })

static inline enum ipte_status
ipte_status (ipte_t * i)
{
  return
    ipte_empty (i) ? STIPTE_EMPTY :
    ipte_pagingin (i) ? STIPTE_PAGINGIN :
    ipte_pagingout (i) ? STIPTE_PAGINGOUT :
    ipte_paged (i) ? STIPTE_PAGED :
    ipte_roshared (i) ? STIPTE_ROSHARED :
    ipte_private (i) ? STIPTE_PRIVATE : (
						{
						fatal
						("Invalid ipte entry %"PRIx64"\n",
						 i->raw);
						STIPTE_EMPTY;
						}
  );
}

#define IPTE_EMPTY 	((ipte_t){ .p = 0, .status = IPTE_EMPTY })


/*
  Indirect Map.

  A pagetable-like structure that mimics inode maps in old UNIX
  filesystems. Allows to keep a single page for indexing small
  objects, and gradually deeper indirect tables for larger objects.
*/
struct imap
{
  ipte_t l1;
  ipte_t l2;
  ipte_t l3;
};

void imap_init (struct imap *im);
ipte_t imap_map (struct imap *im, unsigned long off, pfn_t pfn, bool roshared,
		 mcn_vmprot_t mask);
ipte_t imap_lookup (struct imap *im, unsigned long off);
void imap_foreach (struct imap *im, void (*fn)(unsigned long off, ipte_t *ipte));

/**INDENT-OFF**/
struct cacheobj_mapping
{
  struct umap *umap;
  vaddr_t start;
  size_t size;
  unsigned long off;
  LIST_ENTRY (cacheobj_mapping) list;
};

struct cacheobj
{
  rwlock_t lock;
  /*
     Size of the memory object.
   */
  size_t size;

  /*
     Offset to PFN map.
   */
  struct imap map;

  /*
     VM regions this cache is mapped into.
   */
  LIST_HEAD (, cacheobj_mapping) mappings;
};
/**INDENT-ON**/

struct vm_region;
void cacheobj_init (struct cacheobj *cobj, size_t size);
void cacheobj_addmapping (struct cacheobj *cobj,
			  struct cacheobj_mapping *cobjm);
void cacheobj_delmapping (struct cacheobj *cobj,
			  struct cacheobj_mapping *cobjm);
ipte_t cacheobj_map (struct cacheobj *cobj, mcn_vmoff_t off, pfn_t pfn,
		     bool roshared, mcn_vmprot_t protmask);
ipte_t cacheobj_lookup (struct cacheobj *cobj, mcn_vmoff_t off);
void cacheobj_shadow (struct cacheobj *orig, struct cacheobj *shadow);

void memcache_init (void);
pfn_t memcache_zeropage_new (struct cacheobj *obj, mcn_vmoff_t off,
			     bool roshared, mcn_vmprot_t protmask);
void memcache_share (pfn_t pfn, struct cacheobj *obj, mcn_vmoff_t off, mcn_vmprot_t protmask);
pfn_t memcache_unshare (pfn_t pfn, struct cacheobj *obj, mcn_vmoff_t off,
			mcn_vmprot_t protmask);

/**INDENT-OFF**/
struct cobj_link
{
  struct cacheobj *cobj;
  mcn_vmoff_t off;
  LIST_ENTRY (cobj_link) list;
};

struct physmem_page
{
  lock_t lock;
  unsigned long links_no;
  LIST_HEAD (, cobj_link) links;
  TAILQ_ENTRY (physmem_page) pageq;
};
/**INDENT-ON**/

void memctrl_newpage (struct physmem_page *page);

struct vmobj;
struct vmobjref
{
  struct vmobj *obj;
};

#define VMOBJREF_NULL ((struct vmobjref){.obj = NULL})

struct vmobj
{
  unsigned long _ref_count;

  /*
     The lock in a VM object is shared with all objects in the shadow
     chain.
   */
  lock_t *lock;
  struct portref control_port;
  struct portref name_port;
  bool private;
  struct cacheobj cobj;
  /*
    Shadow is a reference. Copy is not.
    This is to avoid a loop in references.
  */
  struct vmobjref shadow;
  struct vmobj *copy;
};

void vmobj_init (void);
struct vmobjref vmobj_new (bool private, size_t size);
struct vmobjref vmobj_shadowcopy (struct vmobjref *ref);
void vmobj_addregion (struct vmobj *vmobj, struct vm_region *vmreg,
		      struct umap *umap);
void vmobj_delregion (struct vmobj *vmobj, struct vm_region *vmreg);
bool vmobj_fault (struct vmobj *vmobj, mcn_vmoff_t off, mcn_vmprot_t reqprot,
		  pfn_t * outpfn);
struct portref vmobj_getctrlport (struct vmobj *vmobj);
struct portref vmobj_getnameport (struct vmobj *vmobj);

static inline struct vmobjref
vmobjref_from_raw (struct vmobj *vmobj)
{
  if (vmobj == NULL)
    return VMOBJREF_NULL;

  struct vmobjref ref = ((struct vmobjref){.obj=vmobj});
  return REF_DUP(ref);
}

static inline struct vmobjref
vmobjref_move (struct vmobjref *objref)
{
  return REF_MOVE (*objref);
}

static inline struct vmobjref
vmobjref_clone (struct vmobjref *objref)
{
  return REF_DUP (*objref);
}

static inline struct vmobj *
vmobjref_unsafe_get (struct vmobjref *objref)
{
  return REF_GET (*objref);
}

static inline bool
vmobjref_isnull (struct vmobjref *objref)
{
  return vmobjref_unsafe_get (objref) == NULL;
}

static inline void
vmobjref_consume (struct vmobjref objref)
{
  /* XXX: DELETE IF */ REF_DESTROY (objref);
}

struct vm_region;
LIST_HEAD (zlist, vm_region);
struct reg_alloc
{
  uintptr_t opq;
  unsigned long bmap;
#define VM_ORDMAX LONG_BIT
  struct zlist zlist[VM_ORDMAX];
  unsigned nfree;
};

/**INDENT-OFF**/
struct vm_region
{
#define VMR_TYPE_FREE 0
#define VMR_TYPE_USED 1
  uint8_t type;

  /*
     Region Allocator.
   */
  struct rb_node rb_regs;
  /*
     This is used as freelist when the region is free.
   */
  LIST_ENTRY (vm_region) list;

  struct cacheobj_mapping *cobjm;
  vaddr_t start;
  size_t size;

  mcn_vmprot_t curprot;
  mcn_vmprot_t maxprot;
  struct vmobjref objref;
  unsigned long off;
};
/**INDENT-ON**/

#define MSGBUF_ORDMAX 1
struct msgbuf_zentry;
LIST_HEAD (msgbuflist, msgbuf_zentry);
struct msgbuf_zone
{
  rb_tree_t rbtree;
  uintptr_t opq;
  unsigned long bmap;
  struct msgbuflist zlist[MSGBUF_ORDMAX];
  unsigned nfree;
};

void msgbuf_new (struct msgbuf_zone *z, vaddr_t vastart, vaddr_t vaend);

/*
  Machina VM Map.
*/
struct vmmap
{
  lock_t lock;

  /*
     VM Regions handling.
   */
  struct rb_tree regions;
  struct reg_alloc zones;
  size_t total;
  size_t free;

  /*
     User pagetable mappings.
   */
  struct umap umap;

  /*
     Message Buffer allocation.
   */
  struct msgbuf_zone msgbuf_zone;
};

struct msgbuf;
bool vmmap_allocmsgbuf (struct vmmap *map, struct msgbuf *msgbuf);
bool vmmap_alloctls (struct vmmap *map, uaddr_t * tls);
void vmmap_enter (struct vmmap *map);
void vmmap_bootstrap (struct vmmap *map);
void vmmap_setup (struct vmmap *map);

mcn_return_t vmmap_alloc (struct vmmap *map, struct vmobjref objref,
			  mcn_vmoff_t off, size_t size, mcn_vmprot_t curprot,
			  mcn_vmprot_t maxprot, vaddr_t * addrout);
void vmmap_map (struct vmmap *map, vaddr_t start, struct vmobjref objref,
		unsigned long off, size_t size, mcn_vmprot_t curprot,
		mcn_vmprot_t maxprot);
void vmmap_free (struct vmmap *map, vaddr_t start, size_t size);
mcn_return_t vmmap_region (struct vmmap *map, vaddr_t *addr, size_t *size, mcn_vmprot_t *curprot, mcn_vmprot_t *maxprot, mcn_vminherit_t *inherit, bool *shared, struct portref *portref, mcn_vmoff_t *off);
bool vmmap_fault (struct vmmap *map, vaddr_t va, mcn_vmprot_t reqfault);
void vmmap_setupregions (struct vmmap *map);
void vmmap_printregions (struct vmmap *map);



void vmreg_init (void);

#endif