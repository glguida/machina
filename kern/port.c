/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <nux/slab.h>
#include <machina/error.h>

struct slab ports;

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

mcn_return_t port_enqueue(struct port *port, struct msgq_entry *msg, size_t size)
{
  assert (size >= (sizeof(struct msgq_entry) + sizeof(mcn_msgrecv_t)));

  spinlock(&port->lock);
  assert(port->type == PORT_QUEUE);
  LIST_INSERT_HEAD(&port->queue.msgq, msg, list);
  spinunlock(&port->lock);
  return KERN_SUCCESS;
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
port_alloc_kernel(fn_msgsend_t send, void *ctx, struct portref *portref)
{
  struct port *p;

  p = slab_alloc(&ports);
  spinlock_init(&p->lock);
  p->type = PORT_KERNEL;
  p->kernel.msgsend = send;
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

  LIST_INIT(&p->queue.msgq);
  portspace_setup(&p->queue.portspace);
  portref->obj = p;
  p->_ref_count = 1;
  return KERN_SUCCESS;
}

void
port_init(void)
{
  slab_register(&ports, "PORTS", sizeof(struct port), NULL, 0);

}
