/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <rbtree.h>
#include <assert.h>
#include <nux/slab.h>

#define MIN(_a,_b) ((_a) < (_b) ? (_a) : (_b))

struct slab msgbufs;

#define __ZADDR_T unsigned
/**INDENT-OFF**/
struct msgbuf_zentry
{
  rb_node_t rb_node;
  LIST_ENTRY (msgbuf_zentry) list;
  unsigned addr;
  size_t size;
};
/**INDENT-ON**/

static int
rb_regs_compare_key (void *ctx, const void *n, const void *key)
{
  const struct msgbuf_zentry *ze = n;
  const unsigned addr = *(unsigned *) key;

  if (addr < ze->addr)
    return 1;
  if (addr >= ze->addr + ze->size)
    return -1;
  return 0;
}

static int
rb_regs_compare_nodes (void *ctx, const void *n1, const void *n2)
{
  const struct msgbuf_zentry *ze1 = n1;
  const struct msgbuf_zentry *ze2 = n2;

  /* Assert non overlapping */
  assert (ze1->addr < ze2->addr || ze1->addr >= (ze2->addr + ze2->size));
  assert (ze2->addr < ze1->addr || ze2->addr >= (ze1->addr + ze1->size));

  if (ze1->addr < ze2->addr)
    return -1;
  if (ze1->addr > ze2->addr)
    return 1;
  return 0;
}

const rb_tree_ops_t msgbuf_tree_ops = {
  .rbto_compare_nodes = rb_regs_compare_nodes,
  .rbto_compare_key = rb_regs_compare_key,
  .rbto_node_offset = offsetof (struct msgbuf_zentry, rb_node),
  .rbto_context = NULL
};

#define __ZENTRY msgbuf_zentry
#define __ZONE_T struct msgbuf_zone
#define __ZORDMAX MSGBUF_ORDMAX

static void
___get_neighbors (unsigned start, size_t size,
		  struct msgbuf_zentry **pv,
		  struct msgbuf_zentry **nv, uintptr_t opq)
{
  struct msgbuf_zone *z = (struct msgbuf_zone *) opq;
  unsigned end = start + size;
  unsigned key;
  struct msgbuf_zentry *prev = NULL, *next = NULL;

  if (start == 0)
    goto _next;

  key = start - 1;
  prev = rb_tree_find_node (&z->rbtree, (void *) &key);
  if (prev != NULL)
    *pv = prev;
_next:

  if (end == (unsigned) -1)
    goto _done;

  key = end + 1;
  next = rb_tree_find_node (&z->rbtree, (void *) &key);
  if (next != NULL)
    *nv = next;
_done:
}

static struct msgbuf_zentry *
___mkptr (unsigned addr, size_t size, uintptr_t opq)
{
  struct msgbuf_zone *z = (struct msgbuf_zone *) opq;
  struct msgbuf_zentry *ze = slab_alloc (&msgbufs);

  rb_tree_insert_node (&z->rbtree, (void *) ze);
  ze->addr = addr;
  ze->size = size;
  return ze;
}

static void
___freeptr (struct msgbuf_zentry *ze, uintptr_t opq)
{
  struct msgbuf_zone *z = (struct msgbuf_zone *) opq;
  rb_tree_remove_node (&z->rbtree, (void *) ze);
  slab_free (ze);
}

#include "alloc.h"

void
msgbuf_free (struct umap *umap, struct msgbuf_zone *z, struct msgbuf *mb)
{
  long uidx;

  unshare_kva (umap, mb->uaddr, MSGBUF_SIZE);
  kmap_ensure_range (mb->kaddr, MSGBUF_SIZE, 0);
  kva_free (mb->kaddr, MSGBUF_SIZE);
  uidx = mb->uaddr >> MSGBUF_SHIFT;
  zone_free (z, uidx, 1);
}


bool
msgbuf_alloc (struct umap *umap, struct msgbuf_zone *z, struct msgbuf *mb)
{
  long uidx;
  vaddr_t uaddr, kaddr;

  uidx = zone_alloc (z, 1);
  if (uidx == -1)
    return false;

  uaddr = (vaddr_t) uidx << MSGBUF_SHIFT;

  kaddr = kva_alloc (MSGBUF_SIZE);
  if (kaddr == VADDR_INVALID)
    {
      zone_free (z, uidx, 1);
      return false;
    }
  if (kmap_ensure_range (kaddr, MSGBUF_SIZE, HAL_PTE_W | HAL_PTE_P))
    {
      kva_free (kaddr, MSGBUF_SIZE);
      zone_free (z, uidx, 1);
      return false;
    }

  if (!share_kva (kaddr, MSGBUF_SIZE, umap, uaddr, true))
    {
      kmap_ensure_range (kaddr, MSGBUF_SIZE, 0);
      kva_free (kaddr, MSGBUF_SIZE);
      zone_free (z, uaddr, 1);
      return false;
    }

  mb->uaddr = uaddr;
  mb->kaddr = kaddr;

  return true;
}

void
msgbuf_new (struct msgbuf_zone *z, vaddr_t vastart, vaddr_t vaend)
{
  unsigned start, end;
  size_t size;

  zone_init (z, (uintptr_t) z);
  rb_tree_init (&z->rbtree, &msgbuf_tree_ops);

  start = vastart >> MSGBUF_SHIFT;
  end = vaend >> MSGBUF_SHIFT;
  assert (start < end);
  size = end - start;

  zone_free (z, start, size);
}

void
msgbuf_init (void)
{
  slab_register (&msgbufs, "MSGBUFS", sizeof (struct msgbuf_zentry), NULL, 0);
}
