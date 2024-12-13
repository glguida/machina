/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <nux/slab.h>
#include <machina/error.h>
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
thread_new (struct task *task)
{
  struct vmmap *vmmap = &task->vmmap;
  struct thread *th = slab_alloc (&threads);

  th->uctxt = (uctxt_t *) (th + 1);
  uctxt_init (th->uctxt, 0, 0, 0);
  timer_init (&th->timeout);
  spinlock_init (&th->lock);
  port_alloc_kernel ((void *) th, KOT_THREAD, &th->self);

  if (!vmmap_allocmsgbuf (vmmap, &th->msgbuf))
    {
      slab_free (th);
      return NULL;
    }

  if (!vmmap_alloctls (vmmap, &th->tls))
    {
      slab_free (th);
      return NULL;
    }
  uctxt_settls (th->uctxt, th->tls);

  th->task = task;
  th->_ref_count = 0;

  printf ("Allocated thread %p (%lx %lx)\n", th);

  return th;
}

void
thread_bootstrap (struct thread *th)
{
  printf("BOOTSTRAPPING THREAD %p\n", th);
  if (!uctxt_bootstrap (th->uctxt))
    {
      fatal ("No bootstrap process.");
    }
  uctxt_settls (th->uctxt, th->tls);
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

bool
thread_abort (struct thread *th, bool intimer, bool setret)
{
  spinlock (&th->lock);
  if (th->waitq == NULL)
    {
      spinunlock (&th->lock);
      return false;
    }

  spinlock (&th->waitq->lock);
  TAILQ_REMOVE (&th->waitq->queue, th, sched_list);
  spinunlock (&th->waitq->lock);

  if (!intimer)
    timer_remove (&th->timeout);
  th->waitq = NULL;

  if (setret)
    uctxt_setret (th->uctxt, KERN_THREAD_TIMEDOUT);
  spinunlock (&th->lock);

  printf("Resuming thread!\n");
  sched_resume (th);
  return true;
}

static void
__waitq_timeout_handler (void *opq)
{
  struct thread *th = (void *) opq;

  /* 
     A wakeone might have happened. So in case there was no wait
     queue, we return.
   */
  thread_abort (th, true, true);
}

void
thread_wait (struct waitq *wq, unsigned long timeout)
{
  struct thread *curth = cur_thread ();

  spinlock (&curth->lock);
  if (timeout != 0)
    {
      struct timer *t = &curth->timeout;
      t->valid = 1;
      t->opq = curth;
      t->handler = __waitq_timeout_handler;
      timer_register (t, timeout * 1000 * 1000);
    }
  curth->waitq = wq;
  spinlock (&wq->lock);
  TAILQ_INSERT_TAIL (&wq->queue, curth, sched_list);
  spinunlock (&wq->lock);
  spinunlock (&curth->lock);

  sched_suspend(curth);
}

void
thread_wakeone (struct waitq *wq)
{
  struct thread *th = NULL;

  spinlock (&wq->lock);
  if (!TAILQ_EMPTY (&wq->queue))
    {
      th = TAILQ_FIRST (&wq->queue);
      TAILQ_REMOVE (&wq->queue, th, sched_list);
    }
  spinunlock (&wq->lock);

  if (th == NULL)
    return;

  spinlock(&th->lock);
  assert (th->waitq == wq);
  timer_remove (&th->timeout);
  th->waitq = NULL;
  spinunlock(&th->lock);

  sched_resume(th);
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
