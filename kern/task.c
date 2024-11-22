/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <nux/slab.h>
#include <machina/error.h>

#include "internal.h"
#include "vm.h"

struct slab tasks;
struct slab portcaps;

void
task_bootstrap(struct taskref *taskref)
{
  struct task *t;

  t = slab_alloc(&tasks);
  vmmap_bootstrap (&t->vmmap);
  spinlock_init (&t->lock);
  ipcspace_setup(&t->ipcspace);
  port_alloc_kernel((void *)t, KOT_TASK, &t->self);
  taskref->obj = t;
  t->_ref_count = 1;


  vmmap_printregions (&t->vmmap);
  struct vmobjref ref = vmobj_new(true, 3*4096);
  //  struct vmobjref ref2 = vmobjref_clone(&ref);
  vmmap_map (&t->vmmap, 0x1000, ref, 0, 4*4096, MCN_VMPROT_ALL, MCN_VMPROT_ALL);
  vmmap_printregions (&t->vmmap);
  vmmap_free (&t->vmmap, 0x3000, 3*4096);
  vmmap_printregions (&t->vmmap);
  vmmap_free (&t->vmmap, 0x1000, 1*4096);
  vmmap_printregions (&t->vmmap);
}

mcn_portid_t
task_self(void)
{
  mcn_portid_t ret;
  struct ipcspace *ps;
  struct portright pr;
  struct task *t = cur_task();

  pr.type = RIGHT_SEND;
  pr.portref = portref_dup(&t->self);

  ps = task_getipcspace(t);
  ipcspace_insertright(ps, &pr, &ret);
  task_putipcspace(t, ps);

  return ret;
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

mcn_return_t
task_vm_allocate(struct task *t, vaddr_t *addr, size_t size, bool anywhere)
{
  mcn_return_t rc;
  struct vmobjref ref;

  printf("TASK: allocating task %p size %lx anywhere %d\n", t, size, anywhere);
  ref = vmobj_new(true, size);

  spinlock(&t->lock);
  if (anywhere)
    {
      rc = vmmap_alloc(&t->vmmap, ref, size, MCN_VMPROT_DEFAULT, MCN_VMPROT_ALL, addr);
    }
  else
    {
      vmmap_map (&t->vmmap, *addr, ref, 0, size, MCN_VMPROT_DEFAULT, MCN_VMPROT_ALL);
      rc = KERN_SUCCESS;
    }
  if (rc)
    {
      spinunlock(&t->lock);
      return rc;
    }

  spinunlock(&t->lock);
  return rc;
}

void
task_init(void)
{

  slab_register(&tasks, "TASKS", sizeof(struct task), NULL, 0);
}
