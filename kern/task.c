/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <nux/slab.h>
#include <machina/error.h>

#include "internal.h"

struct slab tasks;
struct slab portcaps;

struct task *
task_bootstrap(void)
{
  struct task *t;

  t = slab_alloc(&tasks);
  
  vmmap_bootstrap (&t->vmmap);
  spinlock_init (&t->lock);
  portspace_setup(&t->portspace);
  t->refcount = 0;
  hal_umap_load(NULL);

  return t;
}

void
task_enter(struct task *t)
{
  cur_cpu()->task = t;

  spinlock(&t->lock);
  vmmap_enter (&t->vmmap);
  spinunlock(&t->lock);
}

struct portspace *
task_getportspace(struct task *t)
{
  spinlock(&t->lock);
  portspace_lock(&t->portspace);
  spinunlock(&t->lock);

  return &t->portspace;
}

void
task_putportspace(struct task *t, struct portspace *ps)
{
  spinlock(&t->lock);
  assert(&t->portspace == ps);
  portspace_unlock(&t->portspace);
  spinunlock(&t->lock);
}

mcn_return_t
task_addportright(struct task *t, struct portright *pr, mcn_portid_t *idout)
{
  mcn_portid_t id;
  mcn_return_t rc;
  struct portspace *ps;

  ps = task_getportspace(t);
  rc = portspace_insertright(ps, pr, &id);
  task_putportspace(t, ps);
  printf("TASK: Allocated id %d\n", id);
  if (rc == KERN_SUCCESS)
    *idout = id;
  return rc;
}

mcn_return_t
task_allocate_port(struct task *t, mcn_portid_t *newid)
{
  struct portref portref;
  struct portright pr;
  mcn_return_t rc;

  rc = port_alloc_queue(&portref);
  if (rc)
    return rc;

  pr = portright_from_portref(RIGHT_RECV, portref);
  rc = task_addportright(t, &pr, newid);
  return rc;
}

/*
mcn_return_t
task_allocate_port(struct task *t, mcn_portid_t *newid)
{
  struct port *port;
  struct portcap *pcap;
  uaddr_t uaddr;

  port = port_create();
  pcap = slab_alloc(&portright);

  spinlock(&t->lock);
  if (!vmmap_addport (&t->vmmap, port, &uaddr))
    {
      spinunlock(&t->lock);
      return KERN_NO_SPACE;
    }
  pcap->id = _alloc_id(t);
  pcap->type = PORTCAP_RECEIVE;
  pcap->rcv.mscount = 0;
  pcap->rcv.port = port;
  pcap->rcv.uportbuf = uaddr;
  spinunlock(&t->lock);

  printf("Created port id %ld, uaddr: %lx\n", pcap->id, uaddr);

  *newid = pcap->id;

  return KERN_SUCCESS;
}
*/

void
task_init(void)
{

  slab_register(&tasks, "TASKS", sizeof(struct task), NULL, 0);
  //  slab_register(&portright, "PORTRIGHT", sizeof(struct portcap), NULL, 0);
}
