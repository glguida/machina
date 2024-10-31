/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <rbtree.h>
#include <nux/slab.h>
#include <machina/error.h>

struct slab portentry;

struct portentry {
  mcn_portid_t id;
  unsigned long send_refs;
  struct portright portright;
};

static int
rb_compare_key (void *ctx, const void *n, const void *key)
{
  const struct portentry *pe = n;
  const mcn_portid_t id = * (mcn_portid_t *) key;

  return (id < pe->id) ? 1 : ((id > pe->id) ? -1 : 0);
}

static int
rb_compare_nodes (void *ctx, const void *n1, const void *n2)
{
  const struct portentry *pe1 = n1;
  const struct portentry *pe2 = n2;

  return (pr1->id < pr2->id) ? -1 : ((pr1->id > pr2->id) ? 1 : 0);
}

const rb_tree_ops_t portspace_ops = {
  .rbto_compare_nodes = rb_compare_nodes,
  .rbto_compare_key = rb_compare_key,
  .rbto_node_offset = offsetof (struct portentry, rb_node),
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

static struct portentry *
_portentry_get_dual(struct portspace *ps, mcn_portid_t id1, mcn_portid_t id2)
{
  struct portentry *send_pe, *reply_pe;

  send_pe = rb_tree_find_node(&ps->rb_tree, &send_portid);
  if (send_pe == NULL)
    return MSGIO_SEND_INVALID_DEST;

  reply_pe = rb_tree_find_node(&ps->rb_tree, &reply_portid);
  if (reply_pe == NULL)
    return MSGIO_SEND_INVALID_REPLY;

  struct port *send_port = REF_GET(send_pe->portref);
  struct port *reply_port = REF_GET(reply_pe->portref);
  spinlock_dual(send_port, reply_port);
  switch(send_port->
  spiunlock_dual(REF_GET(send_pe->portref), REF_GET(reply_pe->portref));
}

mcn_msgioret_t
portspace_get_msg_ports(struct portspace *ps,
			uint8_t reply_bits, mcn_portid_t reply_portid, struct portright *reply_right,
			uint8_t send_bits, mcn_portid_t send_portid, struct portright *send_righ)
{


  
}

mcn_return_t
portspace_allocid (struct portspace *ps, mcn_portid_t *id)
{
  mcn_portid_t max;
  struct portentry *pe;

  pr = RB_TREE_MAX(&ps->rb_tree);
  max = pr == NULL ? 0 : pe->id;

  if (max + 1 == 0)
    return KERN_NO_SPACE;

  *id = max + 1;
  return KERN_SUCCESS;
}

mcn_return_t
portspace_movesend(struct portspace *ps, mcn_portid_t id, struct portright *pr)
{
  struct portentry *pe;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  if (pr == NULL)
    return KERN_INVALID_NAME;

  switch (pe->type)
    {
    case PORTENTRY_SEND:
      rb_tree_remove_node(&ps->rb_tree, pr);
      sr->type = SENDTYPE_SEND;
      sr->portref = REF_MOVE(pe->send.portref);
      break;
    default:
      return KERN_INVALID_CAPABILITY;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_copysend(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portentry *pe;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  printf("ps: %p id: %ld pr = %p", &ps->rb_tree, id, pr);
  if (pr == NULL)
    return KERN_INVALID_NAME;

  switch (pe->type)
    {
    case PORTENTRY_SEND:
      sr->type = SENDTYPE_SEND;
      sr->portref = REF_DUP(pe->send.portref);
      break;
    default:
      return KERN_INVALID_CAPABILITY;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_moveonce(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portentry *pe;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  if (pr == NULL)
    return KERN_INVALID_NAME;

  switch (pe->type)
    {
    case PORTENTRY_ONCE:
      rb_tree_remove_node(&ps->rb_tree, pr);
      sr->type = SENDTYPE_ONCE;
      sr->portref = REF_MOVE(pe->once.portref);
      sl-ab_free(pr);
      break;

    default:
      return KERN_INVALID_CAPABILITY;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_makesend(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portentry *pe;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  if (pr == NULL)
    return KERN_INVALID_NAME;

  switch (pe->type)
    {
    case PORTENTRY_RECV:
      sr->type = SENDTYPE_SEND;
      sr->portref = REF_DUP(pe->recv.portref);
      break;

    default:
      return KERN_INVALID_CAPABILITY;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_makeonce(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portentry *pe;

  pr = rb_tree_find_node(&ps->rb_tree, &id);
  if (pr == NULL)
    return KERN_INVALID_NAME;

  switch (pe->type)
    {
    case PORTENTRY_RECV:
      sr->type = SENDTYPE_ONCE;
      sr->portref = REF_DUP(pe->recv.portref);
      break;

    default:
      return KERN_INVALID_CAPABILITY;
    }

  return KERN_SUCCESS;
}

mcn_return_t
portspace_addsendright(struct portspace *ps, mcn_portid_t id, struct sendright *sr)
{
  struct portentry *pe, *tmp;
  mcn_return_t rc;

  pr = slab_alloc(&portentrys);
  assert (pr != NULL);

  pe->id = id;
  switch (sr->type) {
  case SENDTYPE_SEND:
    pe->type = PORTENTRY_SEND;
    pe->send.portref = REF_MOVE(sr->portref);
    break;
  case SENDTYPE_ONCE:
    pe->type = PORTENTRY_ONCE;
    pe->once.portref = REF_MOVE(sr->portref);
    break;
  default:
    return KERN_INVALID_RIGHT;
  }

  tmp = rb_tree_insert_node(&ps->rb_tree, pr);
  if (tmp != pr)
    {
      slab_free(pr);
      rc = KERN_RIGHT_EXISTS;
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
  slab_register(&portentries, "PORTENTRIES", sizeof(struct portentry), NULL, 0);
}
