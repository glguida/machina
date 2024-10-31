/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <nux/slab.h>
#include <machina/error.h>
#include <machina/message.h>

struct slab ports;

void
port_lock_dual(struct port *p1, struct port *p2)
{
  spinlock_dual(&p1->lock, &p2->lock);
}

void
port_unlock_dual(struct port *p1, struct port *p2)
{
  spinunlock_dual(&p1->lock, &p2->lock);
}

bool
port_dead(struct port *port)
{
  bool r;

  spinlock(&port->lock);
  r = port->type == PORT_DEAD;
  spinunlock(&port->lock);
  return r;
}

bool
port_kernel(struct port *port)
{
  bool r;

  spinlock(&port->lock);
  r = port->type == PORT_KERNEL;
  spinunlock(&port->lock);
  return r;
}

enum port_type
port_type(struct port *port)
{
  enum port_type ty;

  spinlock(&port->lock);
  ty = port->type;
  spinunlock(&port->lock);
  return ty;
}

struct portspace *
port_getportspace(struct port *port)
{
  spinlock(&port->lock);
  assert(port->type == PORT_QUEUE);
  portspace_lock(&port->queue.portspace);
  spinunlock(&port->lock);

  return &port->queue.portspace;
}

void
port_putportspace(struct port *port, struct portspace *ps)
{
  spinlock(&port->lock);
  assert(port->type == PORT_QUEUE);
  assert(&port->queue.portspace == ps);
  portspace_unlock(&port->queue.portspace);
  spinunlock(&port->lock);
}

mcn_return_t port_enqueue(struct port *port, mcn_msgsend_t *inmsgh,
			  struct portright *local_right, struct portright *remote_right,
			  volatile void *body, size_t body_size)
{
  mcn_return_t rc;
  
  spinlock(&port->lock);
  switch(port->type)
    {
    case PORT_KERNEL:
      /*
	Machina supports only MSGCALL to kernel ports.
      */
      rc = MSGIO_SEND_INVALID_DEST;
      break;

    case PORT_DEAD:
      /* Messages to dead ports are ignored. */
      portright_consume(local_right);
      portright_consume(remote_right);
      rc = MSGIO_SUCCESS;
      break;

    case PORT_QUEUE: {
      /* Transform into a receive message. */

      struct portspace *ps;
      mcn_portid_t localid, remoteid;
      enum portright_type localtype, remotetype;

      localtype = local_right->type;
      remotetype = remote_right->type;

      portspace_lock(&port->queue.portspace);
      ps = &port->queue.portspace;

      printf("local_right type is %d\n", local_right->type);
      rc = portspace_insertright(ps, local_right, &localid);
      if (rc)
	{
	  portspace_unlock(&port->queue.portspace);
	  spinunlock(&port->lock);
	  return rc;
	}

      printf("remote_right type is %d\n", remote_right->type);
      rc = portspace_insertright(ps, remote_right, &remoteid);
      if (rc)
	{
	  /* Get local_right back. */
	  // XXX: GIANLUCA THIS IS IMPORTANT	  assert(portspace_resolve(ps, MCN_MSGTYPE_MOVESEND, localid, local_right) == MSGIO_SUCCESS);
	  portspace_unlock(&port->queue.portspace);
	  spinunlock(&port->lock);
	  return rc;
	}
      portspace_unlock(&port->queue.portspace);

      mcn_msgsize_t recv_size = sizeof(mcn_msgrecv_t) + body_size;
      void *msg = (void *)kmem_alloc(0, sizeof(struct msgq_entry) + recv_size);
      struct msgq_entry *msgq = (struct msgq_entry *)msg;
      mcn_msgrecv_t *outmsgh = (mcn_msgrecv_t *)(msgq + 1);
      void *outbody = (void *)(outmsgh + 1);

      outmsgh->msgr_bits = 0;
      switch(remotetype)
	{
	case RIGHT_SEND:
	  outmsgh->msgr_bits |= MCN_MSGTYPE_PORTSEND;
	  break;
	case RIGHT_ONCE:
	  outmsgh->msgr_bits |= MCN_MSGTYPE_PORTONCE;
	  break;
	default:
	  fatal("Wrong type %d for port receive remote portright.\n", remotetype);
	  break;
	}

      switch(localtype)
	{
	case RIGHT_SEND:
	  outmsgh->msgr_bits |= (MCN_MSGTYPE_PORTSEND << 8);
	  break;
	case RIGHT_ONCE:
	  outmsgh->msgr_bits |= (MCN_MSGTYPE_PORTONCE << 8);
	  break;
	default:
	  fatal("Wrong type %d for port receive local portright.\n", localtype);
	  break;
	}

      outmsgh->msgr_size = recv_size;
      outmsgh->msgr_remote = remoteid;
      outmsgh->msgr_local = localid;
      outmsgh->msgr_seqno = 0; //port_seqno(port);
      outmsgh->msgr_msgid = inmsgh->msgs_msgid;

      memcpy(outbody, (void *)body, body_size);
      printf("Outbody!\n");
      portspace_print(&port->queue.portspace);
      info("inserting %p to %p\n", msgq, port);
      TAILQ_INSERT_TAIL(&port->queue.msgq, msgq, queue);
      rc = KERN_SUCCESS;
    }
      break;

    default:
      fatal("Wrong port type %d\n", port->type);
      rc = KERN_FAILURE;
      break;
    }
  spinunlock(&port->lock);

  return rc;
}
			  
mcn_return_t
port_dequeue(mcn_portid_t localid, struct port *port, struct portspace *outps, mcn_msgrecv_t *outmsgh, size_t outsize)
{
  mcn_return_t rc;
  
  spinlock(&port->lock);
  switch(port->type)
    {
    default:
      spinunlock(&port->lock);
      rc = MSGIO_RCV_INVALID_NAME;
      break;

    case PORT_DEAD: {
      /* Messages to dead ports are ignored. */
      spinunlock(&port->lock);
      fatal("XXX: DESTRY PORT HERE");
      rc = MSGIO_RCV_PORT_DIED;
      break;
    }

    case PORT_QUEUE: {
      info("Checking %p\n", port);
      if (TAILQ_EMPTY(&port->queue.msgq))
	{
	  spinunlock(&port->lock);
	  memset(outmsgh, 0, sizeof(*outmsgh));
	  warn("wait not implemented");
	  rc = MSGIO_SUCCESS;
	  break;
	}

      struct msgq_entry *msgq = TAILQ_FIRST(&port->queue.msgq);
      assert(msgq != NULL);
      TAILQ_REMOVE(&port->queue.msgq, msgq, queue);
      mcn_msgrecv_t *msgh = (mcn_msgrecv_t *)(msgq + 1);

      /*
	Move local and remote right to current task.
      */
      struct portright local_right, remote_right;
      struct portspace *ps = &port->queue.portspace;
      rc = portspace_resolve(ps, MCN_MSGBITS_LOCAL(msgh->msgr_bits), msgh->msgr_local, &local_right);
      if (rc)
	{
	  fatal("Couldn't retrieve local right from id %ld from queued message: %d\n",msgh->msgr_local, rc);
	}
      rc = portspace_resolve(ps, MCN_MSGBITS_REMOTE(msgh->msgr_bits), msgh->msgr_remote, &remote_right);
      if (rc)
	{
	  fatal("Couldn't retrieve remote right from id %ld from queued message: %d\n",msgh->msgr_remote, rc);
	}
      spinunlock(&port->lock);

      assert((local_right.type == RIGHT_ONCE) || (local_right.type == RIGHT_SEND));
      /* Consume right used to send. */
      msgh->msgr_local = localid;

      mcn_portid_t id;
      assert((remote_right.type == RIGHT_ONCE) || (remote_right.type == RIGHT_SEND));
      rc = portspace_insertright(outps, &remote_right, &id);
      if (rc)
	{
	  msgh->msgr_remote = MCN_PORTID_NULL;
	  portright_consume(&remote_right);
	  warn("Couldn't enter remote right for port. Right lost.");
	}
      else
	{
	  msgh->msgr_remote = id;
	}

      if (msgh->msgr_size >= outsize)
	{
	  warn("Port dequeue failed. Message too large. Message Lost.");
	  rc = MSGIO_RCV_TOO_LARGE;
	  break;
	}
      memcpy(outmsgh, msgh, msgh->msgr_size);
      printf("Freeing %p (%d bytes)\n", msgq, sizeof(struct msgq_entry) + msgh->msgr_size);
      kmem_free(0, (vaddr_t)msgq, sizeof(struct msgq_entry) + msgh->msgr_size);
      rc = MSGIO_SUCCESS;
      break;
    }

      
    }
  return rc;
}

void
port_double_lock(struct port *porta, struct port *portb)
{
  /*
    This is used to ensure atomicity on message send receive.
  */
  if (porta == portb)
    spinlock(&porta->lock);
  else
    {
      /* Pointer value defines lock ordering. */
      spinlock(&MIN(porta,portb)->lock);
      spinlock(&MAX(porta,portb)->lock);
    }
}

void
port_double_unlock(struct port *porta, struct port *portb)
{
  /* Reverse of 'port_double_lock' */
  if (porta == portb)
    spinunlock(&porta->lock);
  else
    {
      spinunlock(&MAX(porta,portb)->lock);
      spinunlock(&MIN(porta,portb)->lock);
    }
}

mcn_return_t
port_alloc_kernel(void *ctx, struct portref *portref)
{
  struct port *p;

  p = slab_alloc(&ports);
  spinlock_init(&p->lock);
  p->type = PORT_KERNEL;
  p->kernel.ctx = ctx;

  portref->obj = p;
  p->_ref_count = 1;

  return KERN_SUCCESS;
}

mcn_return_t
port_alloc_queue(struct portref *portref)
{
  struct port *p;

  p = slab_alloc(&ports);
  spinlock_init(&p->lock);
  p->type = PORT_QUEUE;
  portspace_setup(&p->queue.portspace);
  TAILQ_INIT(&p->queue.msgq);
  portref->obj = p;
  p->_ref_count = 1;
  return KERN_SUCCESS;
}

void
port_init(void)
{
  slab_register(&ports, "PORTS", sizeof(struct port), NULL, 0);

}
