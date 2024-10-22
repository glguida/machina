/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <rbtree.h>
#include <nux/slab.h>
#include <machina/error.h>

struct slab portrights;

enum portright_type {
  PORTRIGHT_SEND,
  PORTRIGHT_ONCE,
  PORTRIGHT_RECV,
};

struct portright {
  mcn_portid_t id;
  struct rb_node rb_node;

  enum portright_type type;
  union {
    struct {
      struct portref portref;
    } send;
    struct {
      struct portref portref;
    } once;
    struct {
      struct portref portref;
    } recv;
  };
};

static int
rb_compare_key (void *ctx, const void *n, const void *key)
{
  const struct portright *pr = n;
  const mcn_portid_t id = * (mcn_portid_t *) key;

  return (id < pr->id) ? 1 : ((id > pr->id) ? -1 : 0);
}

static int
rb_compare_nodes (void *ctx, const void *n1, const void *n2)
{
  const struct portright *pr1 = n1;
  const struct portright *pr2 = n2;

  return (pr1->id < pr2->id) ? -1 : ((pr1->id > pr2->id) ? 1 : 0);
}

const rb_tree_ops_t portspace_ops = {
  .rbto_compare_nodes = rb_compare_nodes,
  .rbto_compare_key = rb_compare_key,
  .rbto_node_offset = offsetof (struct portright, rb_node),
  .rbto_context = NULL
};

void
portspace_lock (struct portspace *ps)
{
  spinlock (&ps->lock);
}

void
portspace_unlock (struct portspace *ps)
{
  spinunlock (&ps->lock);
}

mcn_return_t
portspace_allocid (struct portspace *ps, mcn_portid_t *id)
{
  mcn_portid_t max;
  struct portright *pr;

  pr = RB_TREE_MAX(&ps->rb_tree);
  max = pr == NULL ? 0 : pr->id;

  if (max + 1 == 0)
    return KERN_NO_SPACE;

  *id = max + 1;
  return KERN_SUCCESS;
}

mcn_return_t
portspace_movesend(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portright *pr;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  if (pr == NULL)
    return KERN_NOT_FOUND;

  switch (pr->type)
    {
    case PORTRIGHT_SEND:
      rb_tree_remove_node(&ps->rb_tree, pr);
      sr->type = SENDTYPE_SEND;
      sr->portref = REF_MOVE(pr->send.portref);
      break;
    default:
      return KERN_PORT_INVALID;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_copysend(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portright *pr;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  printf("ps: %p id: %ld pr = %p", &ps->rb_tree, id, pr);
  if (pr == NULL)
    return KERN_NOT_FOUND;

  switch (pr->type)
    {
    case PORTRIGHT_SEND:
      sr->type = SENDTYPE_SEND;
      sr->portref = REF_DUP(pr->send.portref);
      break;
    default:
      return KERN_PORT_INVALID;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_moveonce(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portright *pr;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  if (pr == NULL)
    return KERN_NOT_FOUND;

  switch (pr->type)
    {
    case PORTRIGHT_ONCE:
      rb_tree_remove_node(&ps->rb_tree, pr);
      sr->type = SENDTYPE_ONCE;
      sr->portref = REF_MOVE(pr->once.portref);
      slab_free(pr);
      break;

    default:
      return KERN_PORT_INVALID;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_makesend(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portright *pr;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  if (pr == NULL)
    return KERN_NOT_FOUND;

  switch (pr->type)
    {
    case PORTRIGHT_RECV:
      sr->type = SENDTYPE_SEND;
      sr->portref = REF_DUP(pr->recv.portref);
      break;

    default:
      return KERN_PORT_INVALID;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_makeonce(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portright *pr;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  if (pr == NULL)
    return KERN_NOT_FOUND;

  switch (pr->type)
    {
    case PORTRIGHT_RECV:
      sr->type = SENDTYPE_ONCE;
      sr->portref = REF_DUP(pr->recv.portref);
      break;

    default:
      return KERN_PORT_INVALID;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_addsendright(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portright *pr, *tmp;
  mcn_return_t rc;

  pr = slab_alloc(&portrights);
  assert (pr != NULL);

  pr->id = id;
  switch (sr->type) {
  case SENDTYPE_SEND:
    pr->type = PORTRIGHT_SEND;
    pr->send.portref = REF_MOVE(sr->portref);
    break;
  case SENDTYPE_ONCE:
    pr->type = PORTRIGHT_ONCE;
    pr->once.portref = REF_MOVE(sr->portref);
    break;
  default:
    fatal ("Wrong SENDTYPE.");
  }

  printf("Inserting in tree %p node at %ld", &ps->rb_tree, pr->id);
  tmp = rb_tree_insert_node(&ps->rb_tree, pr);
  if (tmp != pr)
    {
      slab_free(pr);
      rc = KERN_PORTID_BUSY;
    }
  else
    rc = KERN_SUCCESS;
  return rc;
}

void
portspace_setup(struct portspace *ps)
{
  spinlock_init (&ps->lock);
  rb_tree_init (&ps->rb_tree, &portspace_ops);
}

void
portspace_init(void)
{
  slab_register(&portrights, "PORTRIGHTS", sizeof(struct portright), NULL, 0);
}
