/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <nux/slab.h>
#include <machina/error.h>

#include "internal.h"
#include "vm.h"

#ifdef TASK_DEBUG
#define TASK_PRINT printf
#else
#define TASK_PRINT(...)
#endif

struct slab tasks;
struct slab portcaps;

unsigned long *
task_refcnt(struct task *t)
{
  return &t->_ref_count;
}

struct portref
task_getport (struct task *t)
{
  struct portref ret;

  spinlock (&t->lock);
  ret = portref_dup (&t->self);
  spinunlock (&t->lock);
  return ret;
}

mcn_portid_t
task_self (void)
{
  mcn_portid_t ret;
  struct ipcspace *ps;
  struct portright pr;
  struct task *t = cur_task ();

  pr.type = RIGHT_SEND;
  pr.portref = portref_dup (&t->self);

  ps = task_getipcspace (t);
  ipcspace_insertright (ps, &pr, &ret);
  task_putipcspace (t, ps);

  return ret;
}

struct ipcspace *
task_getipcspace (struct task *t)
{
  spinlock (&t->lock);
  return &t->ipcspace;
}

void
task_putipcspace (struct task *t, struct ipcspace *ps)
{
  assert (&t->ipcspace == ps);
  spinunlock (&t->lock);
}

mcn_return_t
task_addportright (struct task *t, struct portright *pr, mcn_portid_t * idout)
{
  mcn_portid_t id;
  mcn_return_t rc;
  struct ipcspace *ps;

  ps = task_getipcspace (t);
  rc = ipcspace_insertright (ps, pr, &id);
  task_putipcspace (t, ps);
  TASK_PRINT ("TASK: Allocated id %d\n", id);
  if (rc == KERN_SUCCESS)
    *idout = id;
  return rc;
}

mcn_return_t
task_allocate_port (struct task *t, mcn_portid_t * newid)
{
  struct portref portref;
  struct portright pr;
  mcn_return_t rc;

  rc = port_alloc_queue (&portref);
  if (rc)
    return rc;

  pr = portright_from_portref (RIGHT_RECV, portref);
  rc = task_addportright (t, &pr, newid);
  return rc;
}

mcn_return_t
task_vm_map (struct task *t, vaddr_t * addr, size_t size, unsigned long mask,
	     bool anywhere, struct vmobjref ref, mcn_vmoff_t off, bool copy,
	     mcn_vmprot_t curprot, mcn_vmprot_t maxprot,
	     mcn_vminherit_t inherit)
{
  mcn_return_t rc = KERN_SUCCESS;

  if (copy)
    {
      TASK_PRINT ("TASK: Making copy of vmobj %p\n", vmobjref_unsafe_get (&ref));
      ref = vmobj_shadowcopy (&ref);
      TASK_PRINT ("TASK: New shadow copy is %p\n", vmobjref_unsafe_get (&ref));
    }

  spinlock (&t->lock);
  if (anywhere)
    {
      rc = vmmap_alloc (&t->vmmap, ref, off, size, curprot, maxprot, addr);
    }
  else
    {
      vmmap_map (&t->vmmap, *addr, ref, off, size, curprot, maxprot);
      rc = KERN_SUCCESS;
    }

  spinunlock (&t->lock);
  return rc;
}

mcn_return_t
task_vm_region (struct task *t, vaddr_t * addr, size_t *size,
		mcn_vmprot_t * curprot, mcn_vmprot_t * maxprot,
		mcn_vminherit_t * inherit, bool *shared,
		struct portref *portref, mcn_vmoff_t * off)
{
  mcn_return_t rc;

  spinlock (&t->lock);
  rc =
    vmmap_region (&t->vmmap, addr, size, curprot, maxprot, inherit, shared,
		  portref, off);
  TASK_PRINT ("TASK: size is %lx\n", *size);
  spinunlock (&t->lock);
  return rc;
}

mcn_return_t
task_vm_allocate (struct task *t, vaddr_t * addr, size_t size, bool anywhere)
{
  struct vmobjref ref;

  TASK_PRINT ("TASK: allocating task %p size %lx anywhere %d\n", t, size,
	  anywhere);
  ref = vmobj_new (true, size);

  return task_vm_map (t, addr, size, 0, anywhere, ref, 0, 0,
		      MCN_VMPROT_DEFAULT, MCN_VMPROT_ALL,
		      MCN_VMINHERIT_DEFAULT);
}

mcn_return_t
task_create_thread(struct task *t, struct threadref *ref)
{

  struct thread *th = thread_new (t);
  if (th == NULL)
    {
      *ref = THREADREF_NULL;
      return KERN_RESOURCE_SHORTAGE;
    }

  spinlock (&t->lock);
  LIST_INSERT_HEAD (&t->threads, th, list_entry);
  spinunlock (&t->lock);

  /*
    The thread saved both in the task's thread list and the scheduler
    list is an _implicit_ reference. We don't keep the threadref
    around but we know that one of the refcount is ours.  When the
    thread is destroyed, we remove it from both the scheduler list and
    the task's list, and we consume this reference.
  */
  struct threadref implicit_threadref = threadref_fromraw(th);

  *ref = threadref_dup (&implicit_threadref);
  return KERN_SUCCESS;
}

static void
threadref_consume_implicit(struct thread *th)
{
  struct threadref ref = { .obj = th };

  threadref_consume(&ref);
}

void
_task_destroy_thread(struct thread *th)
{
  struct task *t = th->task;

  spinlock(&th->lock);
  assert(th->status == SCHED_REMOVED);
  spinunlock(&th->lock);

  spinlock(&t->lock);
  LIST_REMOVE (th, list_entry);
  TASK_PRINT("TASK STATUS IS %s\n", t->status == TASK_DESTROYING ? "DESTROYING" : "ACTIVE");
  if ((t->status == TASK_DESTROYING) && LIST_EMPTY(&t->threads))
    TAILQ_INSERT_TAIL (&cur_cpu ()->dead_tasks, t, task_list);
  spinunlock(&t->lock);

  /*
    Thread is removed, so there's no pointers in the scheduler. It has
    also been removed from the task's thread list, so we can remove
    the thread reference.

    If the thread reference reaches zero, the thread will be
    freed. Otherwhise will be freed when all the messages queued in
    the system (and other references if exist) will be consumed.
   */
  threadref_consume_implicit(th);
}

void
task_destroy (struct task *t)
{
  struct thread *th;

  spinlock (&t->lock);
  TASK_PRINT("TASK %p: DESTROYING TASK (STATUS: %s)\n",
	     t, t->status == TASK_ACTIVE ? "ACTIVE" : "DESTROYING");


  if (t->status == TASK_DESTROYING)
    {
      spinunlock (&t->lock);
      return;
    }
  t->status = TASK_DESTROYING;
  LIST_FOREACH(th, &t->threads, list_entry)
    {
      thread_destroy(th);
    }
  spinunlock (&t->lock);
}

void
task_bootstrap (struct taskref *taskref)
{
  mcn_return_t rc;
  struct task *t;
  struct threadref threadref;

  t = slab_alloc (&tasks);
  vmmap_bootstrap (&t->vmmap);
  spinlock_init (&t->lock);
  ipcspace_setup (&t->ipcspace);
  port_alloc_kernel ((void *) t, KOT_TASK, &t->self);
  t->_ref_count = 0;
  t->status = TASK_ACTIVE;

  /*
    Allocate Implicit reference to task.
  */
  struct taskref implicit_taskref = taskref_fromraw(t);
  (void)implicit_taskref;

  /*
    Create the bootstrap thread and execute.
  */
  rc = task_create_thread(t, &threadref);
  assert (rc == KERN_SUCCESS);

  thread_bootstrap(threadref_unsafe_get(&threadref));
  thread_resume(threadref_unsafe_get(&threadref));
  threadref_consume(&threadref);

  struct vmobjref ref = vmobj_new (true, 3 * 4096);
  //  struct vmobjref ref2 = vmobjref_clone(&ref);
  vmmap_map (&t->vmmap, 0x1000, ref, 0, 4 * 4096, MCN_VMPROT_ALL,
	     MCN_VMPROT_ALL);
  vmmap_printregions (&t->vmmap);
  vmmap_free (&t->vmmap, 0x3000, 3 * 4096);
  vmmap_printregions (&t->vmmap);
  vmmap_free (&t->vmmap, 0x1000, 1 * 4096);
  vmmap_printregions (&t->vmmap);

  *taskref = taskref_fromraw(t);
}

void
_task_cleanup (struct task *t)
{
  TASK_PRINT("TASK %p cleanup!\n", t);

  /*
    Make self port dead. This will disallow new references.
  */
  TASK_PRINT("TASK %p: Unlinking task port\n", t);
  port_unlink_kernel(&t->self);

  /*
    Consume implicit task.
  */
  TASK_PRINT("TASK %p: Consuming self implicit reference.\n", t);
  struct taskref ref = { .obj = t };
  taskref_consume(&ref);
}

void
task_zeroref (struct task *task)
{
  TASK_PRINT ("TASK ZERO REF");
  vmmap_destroy (&task->vmmap);
  ipcspace_destroy(&task->ipcspace);
  slab_free (task);
  slab_printstats();
}

void
task_init (void)
{

  slab_register (&tasks, "TASKS", sizeof (struct task), NULL, 0);
}
