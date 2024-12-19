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

#if PORT_LOCK_MEASURE

DEFINE_LOCK_MEASURE(port_lock_msr);

#define port_lock(_p) spinlock_measured (&(_p)->lock, &port_lock_msr)
#define port_unlock(_p) spinunlock_measured (&(_p)->lock, &port_lock_msr)

#else

#define port_lock(_p) spinlock (&(_p)->lock)
#define port_unlock(_p) spinunlock (&(_p)->lock)

#endif

struct slab ports;
struct slab msgqs;

unsigned long *
port_refcnt (struct port *p)
{
  return &p->_ref_count;
}

bool
port_dead (struct port *port)
{
  bool r;

  port_lock (port);
  r = port->type == PORT_DEAD;
  port_unlock (port);
  return r;
}

bool
port_kernel (struct port *port)
{
  bool r;

  port_lock (port);
  r = port->type == PORT_KERNEL;
  port_unlock (port);
  return r;
}

enum port_type
port_type (struct port *port)
{
  enum port_type ty;

  port_lock (port);
  ty = port->type;
  port_unlock (port);
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

  port_lock (port);
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
  port_unlock (port);
  return rc;
}

mcn_return_t
port_dequeue (struct port *port, unsigned long timeout,
	      mcn_msgheader_t ** msghp)
{
  mcn_return_t rc;

  port_lock (port);
  switch (port->type)
    {
    default:
      port_unlock (port);
      rc = MSGIO_RCV_INVALID_NAME;
      break;

    case PORT_DEAD:
      /* Messages to dead ports are ignored. */
      port_unlock (port);
      rc = MSGIO_RCV_PORT_DIED;
      break;

    case PORT_QUEUE:
      {
	rc = portqueue_deq (&port->queue, timeout, msghp);
	port_unlock (port);
	break;
      }
    }
  return rc;
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

  port_lock (port);
  t = port_getkobj (port, KOT_TASK);
  if (t != NULL)
    ret = taskref_fromraw (t);
  else
    ret = TASKREF_NULL;
  port_unlock (port);
  return ret;
}

struct threadref
port_get_threadref (struct port *port)
{
  struct thread *th;
  struct threadref ret;

  port_lock (port);
  th = port_getkobj (port, KOT_THREAD);
  if (th != NULL)
    ret = threadref_fromraw (th);
  else
    ret = THREADREF_NULL;
  port_unlock (port);
  return ret;
}

struct vmobjref
port_get_vmobjref (struct port *port)
{
  struct vmobj *vmobj;
  struct vmobjref ret;

  port_lock (port);
  vmobj = port_getkobj (port, KOT_VMOBJ);
  if (vmobj != NULL)
    {
      ret = vmobjref_fromraw (vmobj);
    }
  else
    ret = VMOBJREF_NULL;
  port_unlock (port);
  return ret;
}

struct vmobjref
port_get_vmobjref_from_name (struct port *port)
{
  struct vmobj *vmobj;
  struct vmobjref ret;

  port_lock (port);
  vmobj = port_getkobj (port, KOT_VMOBJ_NAME);
  if (vmobj != NULL)
    {
      ret = vmobjref_fromraw (vmobj);
    }
  else
    ret = VMOBJREF_NULL;
  port_unlock (port);
  return ret;
}

struct host *
port_get_host_from_name (struct port *port)
{
  struct host *host;

  port_lock (port);
  host = port_getkobj (port, KOT_HOST_NAME);
  port_unlock (port);

  return host;
}

struct host *
port_get_host_from_ctrl (struct port *port)
{
  struct host *host;

  port_lock (port);
  host = port_getkobj (port, KOT_HOST_CTRL);
  port_unlock (port);

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

  port_lock (p);
  assert (p->type == PORT_KERNEL);
  p->kernel.obj = NULL;
  p->kernel.kot = 0;
  p->type = PORT_DEAD;
  port_unlock (p);

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

  port_lock (p);
  spinlock (&p->lock);
  assert (p->type == PORT_QUEUE);

  while (thread_wakeone (&p->queue.send_waitq));
  while (thread_wakeone (&p->queue.recv_waitq));

  msgq_discard(&p->queue.msgq);

  p->type = PORT_DEAD;
  port_unlock (p);

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
