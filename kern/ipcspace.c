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

enum portentry_type
{
  PORTENTRY_NORMAL,
  PORTENTRY_ONCE,
};

struct portentry
{
  mcn_portid_t id;
  struct portref portref;

  enum portentry_type type;
  union
  {
    struct
    {
      bool recv;
      unsigned long send_count;
    } normal;
    struct
    {
    } once;
  };

  struct rb_node idsearch;
  struct rb_node portsearch;
};

static int
idsearch_cmpkey (void *ctx, const void *n, const void *key)
{
  const struct portentry *pe = n;
  const mcn_portid_t id = *(mcn_portid_t *) key;

  return (id < pe->id) ? 1 : ((id > pe->id) ? -1 : 0);
}

static int
idsearch_cmpnode (void *ctx, const void *n1, const void *n2)
{
  const struct portentry *pe1 = n1;
  const struct portentry *pe2 = n2;

  return (pe1->id < pe2->id) ? -1 : ((pe1->id > pe2->id) ? 1 : 0);
}

const rb_tree_ops_t idsearch_ops = {
  .rbto_compare_nodes = idsearch_cmpnode,
  .rbto_compare_key = idsearch_cmpkey,
  .rbto_node_offset = offsetof (struct portentry, idsearch),
  .rbto_context = NULL
};

static int
portsearch_cmpkey (void *ctx, const void *n, const void *key)
{
  const struct portentry *pe = n;
  const struct port *p = *(struct port **) key;

  return (p < pe->portref.obj) ? 1 : ((p > pe->portref.obj) ? -1 : 0);
}

static int
portsearch_cmpnode (void *ctx, const void *n1, const void *n2)
{
  const struct portentry *pe1 = n1;
  const struct portentry *pe2 = n2;

  return (pe1->portref.obj <
	  pe2->portref.obj) ? -1 : ((pe1->portref.obj >
				     pe2->portref.obj) ? 1 : 0);
}

const rb_tree_ops_t portsearch_ops = {
  .rbto_compare_nodes = portsearch_cmpnode,
  .rbto_compare_key = portsearch_cmpkey,
  .rbto_node_offset = offsetof (struct portentry, portsearch),
  .rbto_context = NULL
};

static mcn_return_t
ipcspace_allocid (struct ipcspace *ps, mcn_portid_t * id)
{
  mcn_portid_t max;
  struct portentry *pe;

  pe = RB_TREE_MAX (&ps->idsearch_rb_tree);
  max = pe == NULL ? 0 : pe->id;

  if (max + 1 == 0)
    return KERN_NO_SPACE;

  *id = max + 1;
  return KERN_SUCCESS;
}

static inline mcn_return_t
_check_op (uint8_t op, bool send_only, struct portentry *pe)
{
  mcn_return_t rc;

  rc = KERN_INVALID_NAME;

  switch (op)
    {

    case MCN_MSGTYPE_COPYSEND:
      if (pe->type != PORTENTRY_NORMAL)
	break;
      if (pe->normal.send_count == 0)
	break;
      rc = KERN_SUCCESS;
      break;

    case MCN_MSGTYPE_MOVESEND:
      if (pe->type != PORTENTRY_NORMAL)
	break;
      if (pe->normal.send_count == 0)
	break;
      rc = KERN_SUCCESS;
      break;

    case MCN_MSGTYPE_MAKESEND:
      if (pe->type != PORTENTRY_NORMAL)
	break;
      if (!pe->normal.recv)
	break;
      rc = KERN_SUCCESS;
      break;

    case MCN_MSGTYPE_MOVEONCE:
      if (pe->type != PORTENTRY_ONCE)
	break;
      rc = KERN_SUCCESS;
      break;

    case MCN_MSGTYPE_MAKEONCE:
      if (pe->type != PORTENTRY_NORMAL)
	break;
      if (!pe->normal.recv)
	break;
      rc = KERN_SUCCESS;
      break;

    case MCN_MSGTYPE_MOVERECV:
      if (send_only)
	break;
      if (pe->type != PORTENTRY_NORMAL)
	break;
      if (!pe->normal.recv)
	break;
      rc = KERN_SUCCESS;
      break;

    default:
      break;
    }

  return rc;
}

static inline void
_exec_op (struct ipcspace *ps, uint8_t op, bool send_only,
	  struct portentry *pe, struct portref *pref)
{
  switch (op)
    {
    case MCN_MSGTYPE_COPYSEND:
      assert (pe->type == PORTENTRY_NORMAL);
      assert (pe->normal.send_count != 0);
      *pref = REF_DUP (pe->portref);
      break;

    case MCN_MSGTYPE_MOVESEND:
      assert (pe->type == PORTENTRY_NORMAL);
      assert (pe->normal.send_count != 0);
      pe->normal.send_count--;
      if ((pe->normal.send_count == 0) && !pe->normal.recv)
	{
	  *pref = REF_MOVE (pe->portref);
	  rb_tree_remove_node (&ps->idsearch_rb_tree, pe);
	  rb_tree_remove_node (&ps->portsearch_rb_tree, pe);
	  slab_free (pe);
	}
      else
	{
	  *pref = REF_DUP (pe->portref);
	}
      break;

    case MCN_MSGTYPE_MAKESEND:
      assert (pe->type == PORTENTRY_NORMAL);
      assert (pe->normal.recv);
      *pref = REF_DUP (pe->portref);
      break;

    case MCN_MSGTYPE_MOVEONCE:
      assert (pe->type == PORTENTRY_ONCE);
      *pref = REF_MOVE (pe->portref);
      rb_tree_remove_node (&ps->idsearch_rb_tree, pe);
      slab_free (pe);
      break;

    case MCN_MSGTYPE_MAKEONCE:
      assert (pe->type == PORTENTRY_NORMAL);
      assert (pe->normal.recv);
      *pref = REF_DUP (pe->portref);
      break;

    case MCN_MSGTYPE_MOVERECV:
      assert (!send_only);
      pe->normal.recv = false;
      if (pe->normal.send_count == 0)
	{
	  *pref = REF_MOVE (pe->portref);
	  rb_tree_remove_node (&ps->idsearch_rb_tree, pe);
	  rb_tree_remove_node (&ps->portsearch_rb_tree, pe);
	  slab_free (pe);
	}
      else
	{
	  *pref = REF_DUP (pe->portref);
	}
      break;

    default:
      fatal ("Unexpected MSGBITS value %d\n", op);
      break;
    }
}

mcn_return_t
ipcspace_resolve_receive (struct ipcspace *ps, mcn_portid_t id,
			  struct portref *portref)
{
  struct portentry *pe;

  pe = rb_tree_find_node (&ps->idsearch_rb_tree, &id);
  if (pe == NULL)
    return KERN_INVALID_NAME;

  if ((pe->type != PORTENTRY_NORMAL) && !pe->normal.recv)
    return KERN_INVALID_NAME;

  *portref = REF_DUP (pe->portref);
  return KERN_SUCCESS;
}

mcn_return_t
ipcspace_resolve (struct ipcspace *ps, uint8_t bits, mcn_portid_t id,
		  struct portref *portref)
{
  mcn_return_t rc;
  struct portentry *pe;

  pe = rb_tree_find_node (&ps->idsearch_rb_tree, &id);
  if (pe == NULL)
    return KERN_INVALID_NAME;

  rc = _check_op (bits, false, pe);
  if (rc)
    return KERN_INVALID_NAME;

  _exec_op (ps, bits, false, pe, portref);
  return KERN_SUCCESS;
}

mcn_msgioret_t
ipcspace_resolve_sendmsg (struct ipcspace *ps,
			  uint8_t rembits, mcn_portid_t remid,
			  struct portref *rempref, uint8_t locbits,
			  mcn_portid_t locid, struct portref *locpref)
{
  mcn_msgioret_t rc;
  const bool locid_is_null = (locid == MCN_PORTID_NULL);
  struct portentry *rempe, *locpe = NULL;

  rempe = rb_tree_find_node (&ps->idsearch_rb_tree, &remid);
  if (rempe == NULL)
    return MSGIO_SEND_INVALID_DEST;

  if (locid_is_null && (locid != 0))
    return MSGIO_SEND_INVALID_HEADER;

  if (!locid_is_null)
    {
      locpe = rb_tree_find_node (&ps->idsearch_rb_tree, &locid);
      if (locpe == NULL)
	return MSGIO_SEND_INVALID_REPLY;
    }

  rc = _check_op (rembits, true, rempe);
  if (rc)
    return MSGIO_SEND_INVALID_DEST;

  if (!locid_is_null)
    {
      rc = _check_op (locbits, true, locpe);
      if (rc)
	return MSGIO_SEND_INVALID_REPLY;
    }

  /*
     Interesting aberration case, completely supported by Mach's (hence
     Machina's) IPC: Remote and Local port being the same.

     The Atomicity rules of the IPC forces us to check that a case of
     of MOVESEND MOVESEND would fail if there's only one send
     right. The latter is the only case where two user references would
     be removed from a port right.
   */
  if (!locid_is_null && locid == remid)
    {
      if ((locbits == MCN_MSGTYPE_MOVESEND)
	  && (rembits == MCN_MSGTYPE_MOVESEND))
	{
	  /* Both moves. the reference count must be at least 2. */
	  assert (locpe->type == PORTENTRY_NORMAL);
	  if (locpe->normal.send_count < 2)
	    {
	      /* We can chose any error here, based on Mach manual. */
	      return MSGIO_SEND_INVALID_REPLY;
	    }
	}
      if ((locbits == MCN_MSGTYPE_MOVEONCE)
	  && (rembits == MCN_MSGTYPE_MOVEONCE))
	{
	  /* Can't move a Send-Once right more than, as name implies, once. Fail. */
	  return MSGIO_SEND_INVALID_REPLY;
	}
    }

  /*
     Another aberration for 'local == remote'. If an op is MOVESEND and
     the other is COPYSEND, the call will succeed regardless of the
     which one is copy and which one is send.
   */
  if (!locid_is_null && (locid == remid) && (locbits == MCN_MSGTYPE_MOVESEND)
      && (rembits == MCN_MSGTYPE_COPYSEND))
    {
      /* Execute remote before op. */
      _exec_op (ps, rembits, true, rempe, rempref);
      _exec_op (ps, locbits, true, locpe, locpref);
    }
  else if (!locid_is_null && (locid == remid)
	   && (locbits == MCN_MSGTYPE_COPYSEND)
	   && (rembits == MCN_MSGTYPE_MOVESEND))
    {
      /* Execute local before op. */
      _exec_op (ps, locbits, true, locpe, locpref);
      _exec_op (ps, rembits, true, rempe, rempref);
    }
  else
    {
      /* Pick any order. */
      if (locid_is_null)
	{
	  locpref->obj = NULL;
	}
      else
	{
	  _exec_op (ps, locbits, true, locpe, locpref);
	}
      _exec_op (ps, rembits, true, rempe, rempref);
    }

  return MSGIO_SUCCESS;
}

mcn_portid_t
ipcspace_lookup (struct ipcspace *ps, struct port *port)
{
  struct portentry *pe;

  pe = rb_tree_find_node (&ps->portsearch_rb_tree, &port);
  assert (pe != NULL);
  assert (pe->type == PORTENTRY_NORMAL);
  return pe->id;
}

mcn_return_t
ipcspace_insertsendrecv (struct ipcspace *ps, struct portright *pr,
			 mcn_portid_t * idout)
{
  mcn_portid_t id;
  struct portentry *pe;

  /*
     Adding a send/recv port right. Search if there's an entry for that port.
   */
  pe = rb_tree_find_node (&ps->portsearch_rb_tree, &pr->portref.obj);
  if (pe == NULL)
    {
      mcn_return_t rc;

      /* 
         Add new send/receive right.
       */
      rc = ipcspace_allocid (ps, &id);
      if (rc)
	return rc;

      pe = slab_alloc (&portentries);
      if (pe == NULL)
	return KERN_RESOURCE_SHORTAGE;

      pe->id = id;
      pe->type = PORTENTRY_NORMAL;
      if (pr->type == RIGHT_SEND)
	{
	  pe->normal.send_count = 1;
	  pe->normal.recv = false;
	}
      else
	{
	  assert (pr->type == RIGHT_RECV);
	  pe->normal.send_count = 0;
	  pe->normal.recv = true;
	}
      pe->portref = portright_movetoportref (pr);
      rb_tree_insert_node (&ps->portsearch_rb_tree, pe);
      rb_tree_insert_node (&ps->idsearch_rb_tree, pe);
      *idout = id;
      return KERN_SUCCESS;
    }
  else
    {
      switch (pe->type)
	{
	case PORTENTRY_NORMAL:
	  /*
	     A right already indexes that port. Update the refcount only.
	   */

	  if (pr->type == RIGHT_SEND)
	    {
	      pe->normal.send_count++;
	      if (pe->normal.send_count == 0)
		{
		  pe->normal.send_count--;
		  return KERN_UREFS_OVERFLOW;
		}
	    }
	  else
	    {
	      assert (pr->type == RIGHT_RECV);
	      assert (pe->normal.recv == false);
	      pe->normal.recv = true;
	    }
	  portright_consume (pr);
	  *idout = pe->id;
	  return KERN_SUCCESS;
	  break;
	default:
	  fatal ("An entry in a IPC port map is of type %d.", pe->type);
	  return KERN_FAILURE;
	}
    }
}

mcn_return_t
ipcspace_insertright (struct ipcspace *ps, struct portright *pr,
		      mcn_portid_t * idout)
{
  struct portentry *pe;
  mcn_return_t rc;
  mcn_portid_t id = 0;

  /* ASSUME: ps locked. */
  switch (pr->type)
    {
    case RIGHT_SEND:
    case RIGHT_RECV:
      rc = ipcspace_insertsendrecv (ps, pr, &id);
      if (rc)
	return rc;
      break;

    case RIGHT_ONCE:
      {
	/*
	   Always add a new right for send-once.
	 */
	rc = ipcspace_allocid (ps, &id);
	if (rc)
	  return rc;

	pe = slab_alloc (&portentries);
	if (pe == NULL)
	  return KERN_RESOURCE_SHORTAGE;

	pe->id = id;
	pe->type = PORTENTRY_ONCE;
	pe->portref = portright_movetoportref (pr);
	rb_tree_insert_node (&ps->idsearch_rb_tree, pe);
	/* DO NOT ADD TO PORT MAP. */
	id = pe->id;
	break;
      }

    default:
      fatal ("Invalid portright type %d\n", pr->type);
      /* Pass-through */
    case RIGHT_INVALID:
      error ("Attempt to add an invalid name!");
      return KERN_INVALID_NAME;
    }

  assert (id != 0);
  *idout = id;
  return KERN_SUCCESS;
}

void
ipcspace_destroy (struct ipcspace *ps)
{
  struct portentry *next;

  while ((next = RB_TREE_MIN(&ps->idsearch_rb_tree)) != NULL)
    {
      rb_tree_remove_node (&ps->idsearch_rb_tree, (void *) next);

      switch (next->type)
	{
	case PORTENTRY_NORMAL:
	  rb_tree_remove_node (&ps->portsearch_rb_tree, (void *) next);
	  if (next->normal.recv)
	    {
	      port_unlink_queue(&next->portref);
	    }
	  break;
	case PORTENTRY_ONCE:
	  break;
	default:
	  fatal ("Invalid Port Entry type %d\n", next->type);
	  break;
	}
      portref_consume (&next->portref);
      slab_free(next);
    }
}

void
ipcspace_setup (struct ipcspace *ps)
{
  rb_tree_init (&ps->idsearch_rb_tree, &idsearch_ops);
  rb_tree_init (&ps->portsearch_rb_tree, &portsearch_ops);
}

void
ipcspace_init (void)
{
  slab_register (&portentries, "PORTENTRIES", sizeof (struct portentry), NULL,
		 0);
}

void
ipcspace_debug (struct ipcspace *ps)
{
  struct portentry *pe;
  RB_TREE_FOREACH (pe, &ps->idsearch_rb_tree)
  {
    printf
      ("Port Entry %ld: Port %p Type: %s [has_receiveright: %d send_count: %d]\n",
       pe->id, pe->portref.obj,
       pe->type == PORTENTRY_NORMAL ? "SENDRECV" : pe->type ==
       PORTENTRY_ONCE ? "ONCE" : "UNKNOWN", pe->normal.recv,
       pe->normal.send_count);
  }
}
