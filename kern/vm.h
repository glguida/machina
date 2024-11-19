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

/*
  Structure of a ipte:

  - V (valid) bit: implies that the entry is valid: the map has an
    entry in this.

  - PFN: If zero, the map has an entry but it's not available. In the
    case of VM objects, it means the entry has to be searched in the
    pager associated with this object.
    If non-zero, the page number the data is currently stored in.
*/
typedef union {
  struct {
    uint64_t pfn : 62;
    uint64_t w: 1;
    uint64_t v : 1;
  };
  uint64_t raw;
} ipte_t;

#define IPTE_EMPTY ((ipte_t){ .v = 0, .w = 0, .pfn = 0 })
#define IPTE_NOPFN ((ipte_t){ .v = 1, .w = 0, .pfn = 0 })

/*
  Indirect Map.

  A pagetable-like structure that mimics inode maps in old UNIX
  filesystems. Allows to keep a single page for indexing small
  objects, and gradually deeper indirect tables for larger objects.
*/
struct imap {
  ipte_t l1;
  ipte_t l2;
  ipte_t l3;
};

void imap_init(struct imap *im);
ipte_t imap_map(struct imap *im, unsigned long off, pfn_t pfn, bool writable);
ipte_t imap_lookup(struct imap *im, unsigned long off);

struct cacheobj {
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
  LIST_HEAD(,vm_region) regions;
};

void cacheobj_init (struct cacheobj *cobj, size_t size);
void cacheobj_addregion(struct cacheobj *cobj, struct vm_region *vmreg);
void cacheobj_delregion(struct cacheobj *cobj, struct vm_region *vmreg);
ipte_t cacheobj_map(struct cacheobj *cobj, vmoff_t off, pfn_t pfn, bool writable);
ipte_t cacheobj_lookup(struct cacheobj *cobj, vmoff_t off);

struct vmobj
{
  bool private;
  unsigned long _ref_count;
  struct cacheobj cobj;
};

void vmobj_init(void);
struct vmobjref vmobj_new(bool private, size_t size);
void vmobj_addregion(struct vmobj *vmobj, struct vm_region *vmreg);
void vmobj_delregion(struct vmobj *vmobj, struct vm_region *vmreg);

struct vmobjref {
  struct vmobj *obj;
};

static inline struct vmobjref
vmobjref_move(struct vmobjref *objref)
{
  return REF_MOVE(*objref);
}

static inline struct vmobjref
vmobjref_clone(struct vmobjref *objref)
{
  return REF_DUP(*objref);
}

static inline struct vmobj *
vmobjref_unsafe_get(struct vmobjref *objref)
{
  return REF_GET(*objref);
}

static inline void
vmobjref_consume(struct vmobjref objref)
{
  /* XXX: DELETE IF */REF_DESTROY(objref);
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
    Used as a cache object list when it is not.
  */
  LIST_ENTRY(vm_region) list;

  struct umap *umap;
  vaddr_t start;
  size_t size;

  struct vmobjref objref;
  unsigned long off;
};

#define MSGBUF_ORDMAX 1
struct msgbuf_zentry;
LIST_HEAD (msgbuflist, msgbuf_zentry);
struct msgbuf_zone {
  rb_tree_t rbtree;
  uintptr_t opq;
  unsigned long bmap;
  struct msgbuflist zlist[MSGBUF_ORDMAX];
  unsigned nfree;
};

void msgbuf_new(struct msgbuf_zone *z, vaddr_t vastart, vaddr_t vaend);

/*
  Machina VM Map.
*/
struct vmmap {
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
bool vmmap_alloctls (struct vmmap *map, uaddr_t *tls);
void vmmap_enter(struct vmmap *map);
void vmmap_bootstrap(struct vmmap *map);
void vmmap_setup(struct vmmap *map);

void vmreg_new(struct vmmap *map, vaddr_t start, struct vmobjref objref, unsigned long off, size_t size);
void vmreg_del (struct vmmap *map, vaddr_t start, size_t size);
void vmreg_print (struct vmmap *map);
void vmreg_setup(struct vmmap *map);
void vmreg_init(void);

#endif
