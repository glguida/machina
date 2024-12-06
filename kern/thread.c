/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <nux/slab.h>
#include "internal.h"

struct slab threads;

unsigned long *
thread_refcnt(struct thread *th)
{
  return &th->_ref_count;
}

struct thread *
thread_idle (void)
{
  struct thread *th;

  /*
     A minimal thread structure.
     The only important bit is the UCTXT.
   */
  th = (struct thread *) kmem_alloc (0, sizeof (struct thread));
  memset (th, 0, sizeof (struct thread));
  th->uctxt = UCTXT_IDLE;
  th->status = SCHED_RUNNABLE;
  return th;
}

struct thread *
thread_new (struct task *t, long ip, long sp, long gp)
{
  struct thread *th;

  th = slab_alloc (&threads);
  th->uctxt = (uctxt_t *) (th + 1);
  spinlock_init (&th->lock);

  printf ("Allocated thread %p (%lx %lx)\n", th, ip, sp);

  spinlock (&t->lock);
  if (!vmmap_allocmsgbuf (&t->vmmap, &th->msgbuf))
    {
      spinunlock (&t->lock);
      slab_free (th);
      return NULL;
    }
  if (!vmmap_alloctls (&t->vmmap, &th->tls))
    {
      spinunlock (&t->lock);
      slab_free (th);
      return NULL;
    }
  th->task = t;
  port_alloc_kernel ((void *) th, KOT_THREAD, &th->self);

  LIST_INSERT_HEAD (&t->threads, th, list_entry);
  uctxt_init (th->uctxt, ip, sp, gp);
  printf ("Thread: %p\n", th);
  uctxt_print (th->uctxt);
  printf ("---------------\n");
  uctxt_settls (th->uctxt, th->tls);
  timer_init (&th->timeout);
  uctxt_print (th->uctxt);
  printf ("---------------\n");
  printf ("---------------\n");
  spinunlock (&t->lock);


  return th;
}

struct thread *
thread_bootstrap (struct task *t)
{
  struct thread *th;

  th = thread_new (t, 0, 0, 0);
  if (th == NULL)
    {
      fatal ("Cannot allocate boot thread.");
    }
  if (!uctxt_bootstrap (th->uctxt))
    {
      fatal ("No bootstrap process.");
    }
  uctxt_settls (th->uctxt, th->tls);

  return th;
}

struct portref
thread_getport (struct thread *th)
{
  struct portref ret;

  spinlock (&th->lock);
  ret = portref_dup (&th->self);
  spinunlock (&th->lock);
  return ret;
}

void
thread_enter (struct thread *th)
{
  if (thread_isidle (cur_thread ()))
    atomic_cpumask_clear (&idlemap, cpu_id ());

  if (thread_isidle (th))
    {
      cpu_umap_exit ();
    }
  else
    {
      spinlock_dual (&th->lock, &th->task->lock);
      task_enter (th->task);
      spinunlock_dual (&th->lock, &th->task->lock);
    }
  cur_cpu ()->thread = th;


  if (thread_isidle (th))
    atomic_cpumask_set (&idlemap, cpu_id ());
}

#if 0
void
thread_vtalrm (int64_t diff)
{
  struct thread *th = cur_thread ();
  struct timer *t = &th->vtt_alarm;

  spinlock (&th->lock);
  timer_remove (t);
  t->time = timer_gettime () + diff;
  t->opq = (void *) th;
  t->handler = NULL;
  t->valid = 1;
  debug ("Setting vtalm at %" PRIx64 "\n", t->time);
  timer_register (t);
  spinunlock (&th->lock);
}
#endif

void
thread_init (void)
{

  slab_register (&threads, "THREADS",
		 sizeof (struct thread) + sizeof (uctxt_t), NULL, 1);
}
