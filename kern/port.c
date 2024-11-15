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

void
msgq_init(msgqueue_t *msgq)
{
  TAILQ_INIT(msgq);
}

mcn_return_t
msgq_enq(msgqueue_t *msgq, mcn_msgheader_t *msgh)
{
  struct msgq_entry *msgq_entry = slab_alloc(&msgqs);
  if (msgq_entry == NULL)
    return KERN_RESOURCE_SHORTAGE;

  msgq_entry->msgh = msgh;
  TAILQ_INSERT_TAIL(msgq, msgq_entry, queue);
  return KERN_SUCCESS;
}

bool
msgq_deq(msgqueue_t *msgq, mcn_msgheader_t **msghp)
{
  struct msgq_entry *msgq_entry = TAILQ_FIRST(msgq);
  if (msgq_entry == NULL)
    return false;
  TAILQ_REMOVE(msgq, msgq_entry, queue);
  *msghp = msgq_entry->msgh;
  slab_free(msgq_entry);
  return true;
}

void
portqueue_init(struct port_queue *queue, unsigned limit)
{
  msgq_init(&queue->msgq);
  waitq_init(&queue->recv_waitq);
  waitq_init(&queue->send_waitq);
  queue->entries = 0;
  queue->capacity = limit;
}

mcn_msgioret_t
portqueue_enq(struct port_queue *pq, unsigned long timeout, bool force, mcn_msgheader_t *msgh)
{
  if (!force && (!waitq_empty(&pq->send_waitq) || (pq->capacity == pq->entries)))
    {
      sched_wait(&pq->send_waitq, timeout);
      return KERN_RETRY;
    }

  msgq_enq(&pq->msgq, msgh);
  pq->entries++;
  sched_wakeone(&pq->recv_waitq);
  return  MSGIO_SUCCESS;
}

mcn_return_t portqueue_deq(struct port_queue *pq, unsigned long timeout, mcn_msgheader_t **msghp)
{
  if (!msgq_deq(&pq->msgq, msghp))
    {
      sched_wait(&pq->recv_waitq, timeout);
      return KERN_RETRY;
    }
  pq->entries--;
  sched_wakeone(&pq->send_waitq);
  return KERN_SUCCESS;
}

mcn_msgioret_t
port_enqueue(mcn_msgheader_t *msgh, unsigned long timeout, bool force)
{
  mcn_return_t rc;
  struct port *port;

  port = ipcport_unsafe_get(msgh->msgh_local);
  if (port == NULL)
    return MSGIO_SEND_INVALID_DEST;

  spinlock(&port->lock);
  switch(port->type)
    {
    case PORT_KERNEL:
      rc = msgq_enq(&cur_cpu()->kernel_msgq, msgh);
      break;

    case PORT_DEAD:
      rc = MSGIO_SEND_INVALID_DEST;
      break;

    case PORT_QUEUE:
      rc = portqueue_enq(&port->queue, timeout,force, msgh);
      break;

    default:
      fatal("Wrong port type %d\n", port->type);
      rc = MSGIO_MSG_IPC_SPACE | KERN_FAILURE;
      break;
    }
  spinunlock(&port->lock);

  return rc;
}
			  
mcn_return_t
port_dequeue(struct port *port, unsigned long timeout, mcn_msgheader_t **msghp)
{
  mcn_return_t rc;

  spinlock(&port->lock);
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
      rc = portqueue_deq(&port->queue, timeout, msghp);
      spinunlock(&port->lock);
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
  portqueue_init(&p->queue, 2* cpu_num() );
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
