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
struct slab msgqs;

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

mcn_return_t portqueue_add(struct port_queue *queue, mcn_msgheader_t *msgh)
{
  struct msgq_entry *msgq = slab_alloc(&msgqs);

  if (msgq == NULL)
    return KERN_RESOURCE_SHORTAGE;

  msgq->msgh = msgh;
  TAILQ_INSERT_TAIL(&queue->msgq, msgq, queue);
  return  KERN_SUCCESS;
}

mcn_return_t
port_enqueue(mcn_msgheader_t *msgh)
{
  mcn_return_t rc;
  struct port *port;

  port = ipcport_unsafe_get(msgh->msgh_local);
  if (port == NULL)
    return MSGIO_SEND_INVALID_DEST;

  printf("enqueueing message %p to port %p\n", msgh, port);
  
  spinlock(&port->lock);
  switch(port->type)
    {
    case PORT_KERNEL:
      rc = portqueue_add(&cur_cpu()->kernel_queue, msgh);
      break;

    case PORT_DEAD:
      rc = MSGIO_SEND_INVALID_DEST;
      break;

    case PORT_QUEUE:
      rc = portqueue_add(&port->queue, msgh);
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
port_dequeue(struct port *port, mcn_msgheader_t **msghp)
{
  mcn_return_t rc;

  spinlock(&port->lock);
  printf("Checking %p (%d)\n", port, port->type);
  switch(port->type)
    {
    default:
      spinunlock(&port->lock);
      rc = MSGIO_RCV_INVALID_NAME;
      break;

    case PORT_DEAD:
      /* Messages to dead ports are ignored. */
      spinunlock(&port->lock);
      rc = MSGIO_RCV_PORT_DIED;
      break;

    case PORT_QUEUE: {
      info("Checking %p\n", port);
      if (TAILQ_EMPTY(&port->queue.msgq))
	{
	  spinunlock(&port->lock);
	  warn("wait not implemented");
	  rc = MSGIO_RCV_TIMED_OUT;
	  break;
	}

      struct msgq_entry *msgq = TAILQ_FIRST(&port->queue.msgq);
      assert(msgq != NULL);
      TAILQ_REMOVE(&port->queue.msgq, msgq, queue);
      spinunlock(&port->lock);
      
      mcn_msgheader_t *msgh = msgq->msgh;
      slab_free(msgq);
      *msghp = msgh;
      rc = KERN_SUCCESS;
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
  //  p->kernel.ctx = ctx;

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
  TAILQ_INIT(&p->queue.msgq);
  portref->obj = p;
  p->_ref_count = 1;
  return KERN_SUCCESS;
}

void
port_init(void)
{
  slab_register(&msgqs, "MSGQS", sizeof(struct msgq_entry), NULL, 0);
  slab_register(&ports, "PORTS", sizeof(struct port), NULL, 0);

}
