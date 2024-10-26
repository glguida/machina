/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <rbtree.h>
#include <nux/slab.h>
#include <machina/error.h>

struct slab portentries;

/*
  Mach (and hence, Machina) port right rules in a IPC space are a bit
  complex:

  1. Send and Receive right are coalesced in a single entry, and
     managed through counters.

  2. Each send-once right has a single entry.

  In order to quickly fetch entries in a port space, rb-trees are
  used. There are two maps for the send/receive: one that index by
  port id, the other that indexes by port, to allow to find if the IPC
  space has already a name for that port.

  The latter is not necessary for the send-once rights. Furthermore,
  when send-once rights are in a IPC space there's no unique
  port-to-name relationship.

  This means that in the port-to-entry map, only the send/receive
  rights are added.
*/

enum portentry_type {
  ENTRY_DEAD,
  ENTRY_SENDRECV,
  ENTRY_SENDONCE,
  ENTRY_PSET
};

struct portentry {
  mcn_portid_t id;
  struct portref portref;

  enum portentry_type type;
  union {
    struct {
      bool has_receive_right;
      unsigned long send_count;
    } sendrecv;
    struct {} once;
    struct {} dead;
  };

  struct rb_node idsearch;
  struct rb_node portsearch;
};


static int
idsearch_cmpkey (void *ctx, const void *n, const void *key)
{
  const struct portentry *pe = n;
  const mcn_portid_t id = * (mcn_portid_t *) key;

  return (id < pe->id) ? 1 : ((id > pe->id) ? -1 : 0);
}

static int
idsearch_cmpnode (void *ctx, const void *n1, const void *n2)
{
  const struct portentry *pe1 = n1;
  const struct portentry *pe2 = n2;

  return (pe1->id < pe2->id) ? -1 : ((pe1->id > pe2->id) ? 1 : 0);
}

const rb_tree_ops_t  idsearch_ops = {
  .rbto_compare_nodes = idsearch_cmpnode,
  .rbto_compare_key = idsearch_cmpkey,
  .rbto_node_offset = offsetof (struct portentry, idsearch),
  .rbto_context = NULL
};

static int
portsearch_cmpkey (void *ctx, const void *n, const void *key)
{
  const struct portentry *pe = n;
  const struct port *p = * (struct port **) key;

  return (p < pe->portref.obj) ? 1 : ((p > pe->portref.obj) ? -1 : 0);
}

static int
portsearch_cmpnode (void *ctx, const void *n1, const void *n2)
{
  const struct portentry *pe1 = n1;
  const struct portentry *pe2 = n2;

  return (pe1->portref.obj < pe2->portref.obj) ? -1 : ((pe1->portref.obj > pe2->portref.obj) ? 1 : 0);
}

const rb_tree_ops_t portsearch_ops = {
  .rbto_compare_nodes = portsearch_cmpnode,
  .rbto_compare_key = portsearch_cmpkey,
  .rbto_node_offset = offsetof (struct portentry, portsearch),
  .rbto_context = NULL
};

#define PORTENTRY_INVALID ((struct portentry *)NULL)
#define PORTENTRY_DEAD ((struct portentry *)-1)

static mcn_return_t
portspace_allocid (struct portspace *ps, mcn_portid_t *id)
{
  mcn_portid_t max;
  struct portentry *pe;

  pe = RB_TREE_MAX(&ps->idsearch_rb_tree);
  max = pe == NULL ? 0 : pe->id;

  if (max + 1 == 0)
    return KERN_NO_SPACE;

  *id = max + 1;
  return KERN_SUCCESS;
}

void
portspace_lock(struct portspace *ps)
{
  spinlock(&ps->lock);
}

void
portspace_unlock(struct portspace *ps)
{
  spinunlock(&ps->lock);
}

mcn_return_t
portspace_insertsendrecv(struct portspace *ps, struct portright *pr, mcn_portid_t *idout)
{
  mcn_portid_t id;
  struct portentry *pe;

  /*
    Adding a send/recv port right. Search if there's an entry for that port.
  */
  pe = rb_tree_find_node(&ps->portsearch_rb_tree, &pr->portref.obj);
  if (pe == NULL)
    {
      mcn_return_t rc;

      /* 
	 Add new send/receive right.
      */
      rc = portspace_allocid(ps, &id);
      if (rc)
	return rc;

      pe = slab_alloc(&portentries);
      if (pe == NULL)
	return KERN_RESOURCE_SHORTAGE;

      pe->id = id;
      pe->type = ENTRY_SENDRECV;
      if (pr->type == RIGHT_SEND)
	{
	  pe->sendrecv.send_count = 1;
	  pe->sendrecv.has_receive_right = false;
	}
      else
	{
	  assert(pr->type == RIGHT_RECV);
	  pe->sendrecv.send_count = 0;
	  pe->sendrecv.has_receive_right = true;
	}
      pe->portref = portright_movetoportref(pr);
      rb_tree_insert_node(&ps->portsearch_rb_tree, pe);
      rb_tree_insert_node(&ps->idsearch_rb_tree, pe);
      *idout = id;
      return KERN_SUCCESS;
    }
  else
    {
      switch (pe->type)
	{
	case ENTRY_DEAD:
	  /*
	    We are carrying a dead port right.
	  */
	  assert(port_dead(pr->portref.obj));
	  portright_consume(pr);
	  *idout = MCN_PORTID_DEAD;
	  return KERN_SUCCESS;
	  break;
	case ENTRY_SENDRECV:
	  /*
	    A right already indexes that port. Update the refcount only.
	  */
	  
	  if (pr->type == RIGHT_SEND)
	    {
	      pe->sendrecv.send_count++;
	      if (pe->sendrecv.send_count == 0)
		{
		  pe->sendrecv.send_count--;
		  return KERN_UREFS_OVERFLOW;
		}
	    }
	  else
	    {
	      assert(pr->type == RIGHT_RECV);
	      assert(pe->sendrecv.has_receive_right == false);
	      pe->sendrecv.has_receive_right = true;
	    }
	  portright_consume(pr);
	  *idout = pe->id;
	  return KERN_SUCCESS;
	  break;
	default:
	  fatal("An entry in a IPC port map is of type %d.", pe->type);
	  return KERN_FAILURE;
	}
    }
}

mcn_return_t
portspace_insertright(struct portspace *ps, struct portright *pr, mcn_portid_t *idout)
{
  struct portentry *pe;
  mcn_return_t rc;
  mcn_portid_t id = 0;

  /* ASSUME: ps locked. */
  switch(pr->type)
    {
    case RIGHT_SEND:
    case RIGHT_RECV:
      rc = portspace_insertsendrecv(ps, pr, &id);
      if (rc)
	return rc;
      break;

    case RIGHT_ONCE: {
      /*
	Always add a new right for send-once.
      */
      rc = portspace_allocid(ps, &id);
      if (rc)
	return rc;

      pe = slab_alloc(&portentries);
      if (pe == NULL)
	return KERN_RESOURCE_SHORTAGE;

      pe->id = id;
      pe->type = ENTRY_SENDONCE;
      pe->portref = portright_movetoportref(pr);
      rb_tree_insert_node(&ps->idsearch_rb_tree, pe);
      /* DO NOT ADD TO PORT MAP. */
      id = pe->id;
    }

    case RIGHT_INVALID:
      error("Attempt to add an invalid name!");
      return KERN_INVALID_NAME;
    }

  assert(id != 0);
  *idout = id;
  return KERN_SUCCESS;
}


static struct portentry *
_portentry_get(struct portspace *ps, mcn_portid_t id)
{
  struct portentry *pe;

  /* ASSUME: ps locked. */
  pe = rb_tree_find_node(&ps->idsearch_rb_tree, &id);
  if (pe == NULL)
    return PORTENTRY_INVALID;

  switch (pe->type)
    {
    case ENTRY_DEAD:
      pe = PORTENTRY_DEAD;
      break;
    case ENTRY_SENDRECV:
      /*
	Check for port death.
      */
      struct port *p = REF_GET(pe->portref);
      if (port_dead(p))
	{
	  p = NULL;
	  pe->type = ENTRY_DEAD;
	  pe = PORTENTRY_DEAD;
	  break;
	}
      p = NULL;
      break;
    case ENTRY_PSET:
      fatal("IMPLEMENT PORT SETS!");
      break;
    default:
      fatal("Invalid entry type %d\n", pe->type);
    }

  return pe;
}

mcn_return_t
portspace_moveonce(struct portspace *ps, mcn_portid_t id, struct portright *pr)
{
  mcn_return_t rc;
  struct portentry *pe;

  spinlock(&ps->lock);
  pe = _portentry_get(ps, id);

  if (pe == PORTENTRY_INVALID)
    {
      spinunlock(&ps->lock);
      return KERN_INVALID_NAME;
    }

  if (pe == PORTENTRY_DEAD)
    {
      spinunlock(&ps->lock);
      return KERN_INVALID_NAME;
    }

  switch (pe->type)
    {
    case RIGHT_ONCE:
      pr->type = RIGHT_ONCE;
      pr->portref = REF_MOVE(pe->portref);
      rb_tree_remove_node(&ps->idsearch_rb_tree, pe);
      slab_free(pe);
      rc = KERN_SUCCESS;
      break;
    default:
      rc = KERN_INVALID_NAME;
      break;
    }

  spinunlock(&ps->lock);
  return rc;
}

mcn_return_t
portspace_copysend(struct portspace *ps, mcn_portid_t id, struct portright *pr)
{
  struct portentry *pe;

  /* ASSUME: portspace locked. */
  pe = _portentry_get(ps, id);

  if (pe == PORTENTRY_INVALID)
    {
      spinunlock(&ps->lock);
      return KERN_INVALID_NAME;
    }
  
  if (pe == PORTENTRY_DEAD)
    {
      spinunlock(&ps->lock);
      return KERN_INVALID_NAME;
    }

  switch(pe->type)
    {
    case ENTRY_SENDRECV:
      if (pe->sendrecv.send_count == 0)
	return KERN_INVALID_CAPABILITY;

      pr->type = RIGHT_SEND;
      pr->portref = REF_DUP(pe->portref);
      return KERN_SUCCESS;
      break;
    default:
      return KERN_INVALID_NAME;
      break;
    }
}

mcn_return_t
portspace_movesend(struct portspace *ps, mcn_portid_t id, struct portright *pr)
{
  struct portentry *pe;

  /* ASSUME: portspace locked. */
  pe = _portentry_get(ps, id);

  if (pe == PORTENTRY_INVALID)
    {
      spinunlock(&ps->lock);
      return KERN_INVALID_NAME;
    }
  
  if (pe == PORTENTRY_DEAD)
    {
      spinunlock(&ps->lock);
      return KERN_INVALID_NAME;
    }

  switch(pe->type)
    {
    case ENTRY_SENDRECV:
      if (pe->sendrecv.send_count == 0)
	return KERN_INVALID_CAPABILITY;

      pe->sendrecv.send_count--;

      pr->type = RIGHT_SEND;

      if ((pe->sendrecv.send_count == 0)
	  && (!pe->sendrecv.has_receive_right))
	{
	  pr->portref = REF_MOVE(pe->portref);
	  rb_tree_remove_node(&ps->idsearch_rb_tree, pe);
	  rb_tree_remove_node(&ps->portsearch_rb_tree, pe);
	  slab_free(pe);
	}
      else
	{
	  pr->portref = REF_DUP(pe->portref);
	}
      return KERN_SUCCESS;
      break;
    default:
      return KERN_INVALID_NAME;
      break;
    }
}

mcn_return_t
portspace_moverecv(struct portspace *ps, mcn_portid_t id, struct portright *pr)
{
  struct portentry *pe;

  /* ASSUME: portspace locked. */
  pe = _portentry_get(ps, id);
  
  if (pe == PORTENTRY_INVALID)
    return KERN_INVALID_NAME;

  if (pe == PORTENTRY_DEAD)
      return KERN_INVALID_NAME;

  switch(pe->type)
    {
    case ENTRY_SENDRECV:
      if (!pe->sendrecv.has_receive_right)
	return KERN_INVALID_CAPABILITY;

      pe->sendrecv.has_receive_right = false;
      pr->type = RIGHT_RECV;

      if ((pe->sendrecv.send_count == 0)
	  && (!pe->sendrecv.has_receive_right))
	{
	  pr->portref = REF_MOVE(pe->portref);
	  rb_tree_remove_node(&ps->idsearch_rb_tree, pe);
	  rb_tree_remove_node(&ps->portsearch_rb_tree, pe);
	  slab_free(pe);
	}
      else
	{
	  pr->portref = REF_DUP(pe->portref);
	}
      return KERN_SUCCESS;
      break;
    default:
      return KERN_INVALID_NAME;
      break;
    }
}

mcn_return_t
portspace_makesend(struct portspace *ps, mcn_portid_t id, struct portright *pr)
{
  struct portentry *pe;

  /* ASSUME: portspace locked. */
  pe = _portentry_get(ps, id);
  
  if (pe == PORTENTRY_INVALID)
    return KERN_INVALID_NAME;

  if (pe == PORTENTRY_DEAD)
      return KERN_INVALID_NAME;

  switch(pe->type)
    {
    case ENTRY_SENDRECV:
      if (!pe->sendrecv.has_receive_right)
	return KERN_INVALID_CAPABILITY;

      pr->type = RIGHT_SEND;
      pr->portref = REF_DUP(pe->portref);
      return KERN_SUCCESS;
      break;
    default:
      return KERN_INVALID_NAME;
      break;
    }
}

mcn_return_t
portspace_makeonce(struct portspace *ps, mcn_portid_t id, struct portright *pr)
{
  struct portentry *pe;

  /* ASSUME: portspace locked. */
  pe = _portentry_get(ps, id);
  
  if (pe == PORTENTRY_INVALID)
    return KERN_INVALID_NAME;

  if (pe == PORTENTRY_DEAD)
      return KERN_INVALID_NAME;

  switch(pe->type)
    {
    case ENTRY_SENDRECV:
      if (!pe->sendrecv.has_receive_right)
	return KERN_INVALID_CAPABILITY;

      pr->type = RIGHT_ONCE;
      pr->portref = REF_DUP(pe->portref);
      return KERN_SUCCESS;
      break;
    default:
      return KERN_INVALID_NAME;
      break;
    }
}

void
portspace_print(struct portspace *ps)
{
  struct portentry *pe;
  spinlock(&ps->lock);
  RB_TREE_FOREACH(pe, &ps->idsearch_rb_tree) {
    printf("Port Entry %ld: Port %p Type: %s [has_receiveright: %d send_count: %d]\n",
	   pe->id, pe->portref.obj,
	   pe->type == ENTRY_DEAD ? "DEAD" : pe->type == ENTRY_SENDRECV ? "SENDRECV" : pe->type == ENTRY_SENDONCE ? "ONCE" : pe->type == ENTRY_PSET ? "PSET" : "UNKNOWN",
	   pe->sendrecv.has_receive_right, pe->sendrecv.send_count);
  }
  spinunlock(&ps->lock);
}

void
portspace_setup(struct portspace *ps)
{
  spinlock_init (&ps->lock);
  rb_tree_init (&ps->idsearch_rb_tree, &idsearch_ops);
  rb_tree_init (&ps->portsearch_rb_tree, &portsearch_ops);
}

void
portspace_init(void)
{
  slab_register(&portentries, "PORTENTRIES", sizeof(struct portentry), NULL, 0);
}

