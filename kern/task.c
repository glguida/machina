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
  ipcspace_setup(&t->ipcspace);
  t->refcount = 0;

  return t;
}

void
task_enter(struct task *t)
{
  cur_cpu()->task = t;
  vmmap_enter (&t->vmmap);
}

struct ipcspace *
task_getipcspace(struct task *t)
{
  spinlock(&t->lock);
  return &t->ipcspace;
}

void
task_putipcspace(struct task *t, struct ipcspace *ps)
{
  assert(&t->ipcspace == ps);
  spinunlock(&t->lock);
}

mcn_return_t
task_addportright(struct task *t, struct portright *pr, mcn_portid_t *idout)
{
  mcn_portid_t id;
  mcn_return_t rc;
  struct ipcspace *ps;

  ps = task_getipcspace(t);
  rc = ipcspace_insertright(ps, pr, &id);
  task_putipcspace(t, ps);
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

void
task_init(void)
{

  slab_register(&tasks, "TASKS", sizeof(struct task), NULL, 0);
}
