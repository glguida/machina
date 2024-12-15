/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <nux/slab.h>
#include <machina/error.h>
#include <machina/message.h>

#ifdef PORT_DEBUG
#define PORT_PRINT printf
#else
#define PORT_PRINT(...)
#endif

struct slab ports;
struct slab msgqs;

unsigned long *
port_refcnt (struct port *p)
{
  return &p->_ref_count;
}

void
port_lock_dual (struct port *p1, struct port *p2)
{
  spinlock_dual (&p1->lock, &p2->lock);
}

void
port_unlock_dual (struct port *p1, struct port *p2)
{
  spinunlock_dual (&p1->lock, &p2->lock);
}

bool
port_dead (struct port *port)
{
  bool r;

  spinlock (&port->lock);
  r = port->type == PORT_DEAD;
  spinunlock (&port->lock);
  return r;
}

bool
port_kernel (struct port *port)
{
  bool r;

  spinlock (&port->lock);
  r = port->type == PORT_KERNEL;
  spinunlock (&port->lock);
  return r;
}

enum port_type
port_type (struct port *port)
{
  enum port_type ty;

  spinlock (&port->lock);
  ty = port->type;
  spinunlock (&port->lock);
  return ty;
}

void
msgq_init (msgqueue_t * msgq)
{
  TAILQ_INIT (msgq);
}

mcn_return_t
msgq_enq (msgqueue_t * msgq, mcn_msgheader_t * msgh)
{
  struct msgq_entry *msgq_entry = slab_alloc (&msgqs);
  if (msgq_entry == NULL)
    return KERN_RESOURCE_SHORTAGE;

  msgq_entry->msgh = msgh;
  TAILQ_INSERT_TAIL (msgq, msgq_entry, queue);
  return KERN_SUCCESS;
}

bool
msgq_deq (msgqueue_t * msgq, mcn_msgheader_t ** msghp)
{
  struct msgq_entry *msgq_entry = TAILQ_FIRST (msgq);
  if (msgq_entry == NULL)
    return false;
  TAILQ_REMOVE (msgq, msgq_entry, queue);
  *msghp = msgq_entry->msgh;
  slab_free (msgq_entry);
  return true;
}

void
msgq_discard (msgqueue_t *msgq)
{
  struct msgq_entry *n, *t;

  TAILQ_FOREACH_SAFE(n, msgq, queue, t)
    {
      TAILQ_REMOVE (msgq, n, queue);
      ipc_intmsg_consume (n->msgh);
      slab_free(n);
    }
}

void
portqueue_init (struct port_queue *queue, unsigned limit)
{
  msgq_init (&queue->msgq);
  waitq_init (&queue->recv_waitq);
  waitq_init (&queue->send_waitq);
  queue->entries = 0;
  queue->capacity = limit;
}

mcn_msgioret_t
portqueue_enq (struct port_queue *pq, unsigned long timeout, bool force,
	       mcn_msgheader_t * msgh)
{
  if (!force
      && (!waitq_empty (&pq->send_waitq) || (pq->capacity == pq->entries)))
    {
      thread_wait (&pq->send_waitq, timeout);
      return KERN_RETRY;
    }
  msgq_enq (&pq->msgq, msgh);
  pq->entries++;
  thread_wakeone (&pq->recv_waitq);
  return MSGIO_SUCCESS;
}

mcn_return_t
portqueue_deq (struct port_queue *pq, unsigned long timeout,
	       mcn_msgheader_t ** msghp)
{
  if (!msgq_deq (&pq->msgq, msghp))
    {
      thread_wait (&pq->recv_waitq, timeout);
      return KERN_RETRY;
    }
  pq->entries--;
  thread_wakeone (&pq->send_waitq);
  return KERN_SUCCESS;
}

mcn_msgioret_t
port_enqueue (mcn_msgheader_t * msgh, unsigned long timeout, bool force)
{
  mcn_return_t rc;
  struct port *port;

  port = ipcport_unsafe_get (msgh->msgh_local);
  if (port == NULL)
    return MSGIO_SEND_INVALID_DEST;

  spinlock (&port->lock);
  switch (port->type)
    {
    case PORT_KERNEL:
      rc = msgq_enq (&cur_cpu ()->kernel_msgq, msgh);
      break;

    case PORT_DEAD:
      rc = MSGIO_SEND_INVALID_DEST;
      break;

    case PORT_QUEUE:
      rc = portqueue_enq (&port->queue, timeout, force, msgh);
      break;

    default:
      fatal ("Wrong port type %d\n", port->type);
      rc = MSGIO_MSG_IPC_SPACE | KERN_FAILURE;
      break;
    }
  spinunlock (&port->lock);
  return rc;
}

mcn_return_t
port_dequeue (struct port *port, unsigned long timeout,
	      mcn_msgheader_t ** msghp)
{
  mcn_return_t rc;

  spinlock (&port->lock);
  switch (port->type)
    {
    default:
      spinunlock (&port->lock);
      rc = MSGIO_RCV_INVALID_NAME;
      break;

    case PORT_DEAD:
      /* Messages to dead ports are ignored. */
      spinunlock (&port->lock);
      rc = MSGIO_RCV_PORT_DIED;
      break;

    case PORT_QUEUE:
      {
	rc = portqueue_deq (&port->queue, timeout, msghp);
	spinunlock (&port->lock);
	break;
      }
    }
  return rc;
}

void
port_double_lock (struct port *porta, struct port *portb)
{
  /*
     This is used to ensure atomicity on message send receive.
   */
  if (porta == portb)
    spinlock (&porta->lock);
  else
    {
      /* Pointer value defines lock ordering. */
      spinlock (&MIN (porta, portb)->lock);
      spinlock (&MAX (porta, portb)->lock);
    }
}

void
port_double_unlock (struct port *porta, struct port *portb)
{
  /* Reverse of 'port_double_lock' */
  if (porta == portb)
    spinunlock (&porta->lock);
  else
    {
      spinunlock (&MAX (porta, portb)->lock);
      spinunlock (&MIN (porta, portb)->lock);
    }
}

static void *
port_getkobj (struct port *port, enum kern_objtype kot)
{
  if (port->type != PORT_KERNEL)
    return NULL;

  if (port->kernel.kot != kot)
    return NULL;

  return port->kernel.obj;
}

struct taskref
port_get_taskref (struct port *port)
{
  struct task *t;
  struct taskref ret;

  spinlock (&port->lock);
  t = port_getkobj (port, KOT_TASK);
  if (t != NULL)
    ret = taskref_fromraw (t);
  else
    ret = TASKREF_NULL;
  spinunlock (&port->lock);
  return ret;
}

struct threadref
port_get_threadref (struct port *port)
{
  struct thread *th;
  struct threadref ret;

  spinlock (&port->lock);
  th = port_getkobj (port, KOT_THREAD);
  if (th != NULL)
    ret = threadref_fromraw (th);
  else
    ret = THREADREF_NULL;
  spinunlock (&port->lock);
  return ret;
}

struct vmobjref
port_get_vmobjref (struct port *port)
{
  struct vmobj *vmobj;
  struct vmobjref ret;

  spinlock (&port->lock);
  vmobj = port_getkobj (port, KOT_VMOBJ);
  if (vmobj != NULL)
    {
      ret = vmobjref_fromraw (vmobj);
    }
  else
    ret = VMOBJREF_NULL;
  spinunlock (&port->lock);
  return ret;
}

struct vmobjref
port_get_vmobjref_from_name (struct port *port)
{
  struct vmobj *vmobj;
  struct vmobjref ret;

  spinlock (&port->lock);
  vmobj = port_getkobj (port, KOT_VMOBJ_NAME);
  if (vmobj != NULL)
    {
      ret = vmobjref_fromraw (vmobj);
    }
  else
    ret = VMOBJREF_NULL;
  spinunlock (&port->lock);
  return ret;
}

struct host *
port_get_host_from_name (struct port *port)
{
  struct host *host;

  spinlock (&port->lock);
  host = port_getkobj (port, KOT_HOST_NAME);
  spinunlock (&port->lock);

  return host;
}

struct host *
port_get_host_from_ctrl (struct port *port)
{
  struct host *host;

  spinlock (&port->lock);
  host = port_getkobj (port, KOT_HOST_CTRL);
  spinunlock (&port->lock);

  return host;
}

void
port_alloc_kernel (void *obj, enum kern_objtype kot, struct portref *portref)
{
  struct port *p;

  p = slab_alloc (&ports);
  spinlock_init (&p->lock);
  p->type = PORT_KERNEL;
  p->kernel.obj = obj;
  p->kernel.kot = kot;

  portref->obj = p;
  p->_ref_count = 1;
}

void
port_unlink_kernel (struct portref *portref)
{
  struct port *p = portref_unsafe_get(portref);

  spinlock (&p->lock);
  assert (p->type == PORT_KERNEL);
  p->kernel.obj = NULL;
  p->kernel.kot = 0;
  p->type = PORT_DEAD;
  spinunlock (&p->lock);

  portref_consume (portref);
}

mcn_return_t
port_alloc_queue (struct portref *portref)
{
  struct port *p;

  p = slab_alloc (&ports);
  spinlock_init (&p->lock);
  p->type = PORT_QUEUE;
  portqueue_init (&p->queue, 2 * cpu_num ());
  portref->obj = p;
  p->_ref_count = 1;
  return KERN_SUCCESS;
}

void
port_unlink_queue (struct portref *portref)
{
  struct port *p = portref_unsafe_get(portref);

  spinlock (&p->lock);
  assert (p->type == PORT_QUEUE);

  while (thread_wakeone (&p->queue.send_waitq));
  while (thread_wakeone (&p->queue.recv_waitq));

  msgq_discard(&p->queue.msgq);

  p->type = PORT_DEAD;
  spinunlock (&p->lock);

  portref_consume (portref);
}

void
port_zeroref (struct port *port)
{
  PORT_PRINT ("PORT ZERO REF %p\n", port);
  assert (port->type == PORT_DEAD);
  slab_free (port);
}

void
port_init (void)
{
  slab_register (&msgqs, "MSGQS", sizeof (struct msgq_entry), NULL, 0);
  slab_register (&ports, "PORTS", sizeof (struct port), NULL, 0);

}
