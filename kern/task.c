/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <nux/slab.h>

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
  //  rb_tree_init(&t->portright, &portright_ops);
  t->refcount = 0;
  hal_umap_load(NULL);

  return t;
}

void
task_enter(struct task *t)
{

  spinlock(&t->lock);
  t->refcount++;
  vmmap_enter (&t->vmmap);
  spinunlock(&t->lock);
}

/*
static mcn_portid_t
_alloc_id(struct task *t)
{
  struct portcap *last;

  last = (struct portcap *)RB_TREE_MAX(&t->portright);

  return last ? last->id + 1 : 0;
}

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
