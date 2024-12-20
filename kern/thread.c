/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <nux/slab.h>
#include <machina/error.h>
#include "internal.h"

#ifndef THREAD_DEBUG
#define THREAD_PRINT(...)
#else
#define THREAD_PRINT printf
#endif

#if THREAD_LOCK_MEASURE
DEFINE_LOCK_MEASURE(thread_lock_msr);
#endif


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

  _sched_add (th);

  THREAD_PRINT ("Allocated thread %p (%lx %lx)\n", th);

  return th;
}

void
thread_zeroref (struct thread *th)
{
  struct task *task = th->task;
  struct vmmap *vmmap = &task->vmmap;

  /*
    Zero reference remaining. We can free the structure.
  */
  vmmap_freemsgbuf (vmmap, &th->msgbuf);
  vmmap_freetls (vmmap, th->tls);

  slab_free(th);
  slab_printstats();
}

void
thread_bootstrap (struct thread *th)
{
  THREAD_PRINT ("BOOTSTRAPPING THREAD %p\n", th);
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

  thread_lock (th);
  switch (th->status)
    {
    case SCHED_RUNNING:
    case SCHED_RUNNABLE:
    case SCHED_STOPPED:
      ret = portref_dup (&th->self);
      break;
    case SCHED_REMOVED:
      ret = PORTREF_NULL;
      break;
    default:
      fatal ("Invalid thread status: %d (thread: %p)",
	     th->status, th);
      break;
    }
  thread_unlock (th);

  return ret;
}

static void
thread_abort (struct thread *th, bool intimer, bool setret)
{
  thread_lock (th);
  if (th->waitq != NULL)
    {
      waitq_lock (th->waitq);
      TAILQ_REMOVE (&th->waitq->queue, th, sched_list);
      waitq_unlock (th->waitq);

      if (!intimer)
	timer_remove (&th->timeout);
      th->waitq = NULL;

      if (setret)
	uctxt_setret (th->uctxt, KERN_THREAD_TIMEDOUT);
    }
  _sched_abort (th);
  thread_unlock (th);
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

  thread_lock (curth);
  assert (curth->status == SCHED_RUNNING);
  if (timeout != 0)
    {
      struct timer *t = &curth->timeout;
      t->valid = 1;
      t->opq = curth;
      t->handler = __waitq_timeout_handler;
      timer_register (t, timeout * 1000 * 1000);
    }
  curth->waitq = wq;
  waitq_lock (wq);
  TAILQ_INSERT_TAIL (&wq->queue, curth, sched_list);
  waitq_unlock (wq);

  _sched_suspend(curth);
  thread_unlock (curth);
}

bool
thread_wakeone (struct waitq *wq)
{
  struct thread *th = NULL;

  waitq_lock (wq);
  if (!TAILQ_EMPTY (&wq->queue))
    {
      th = TAILQ_FIRST (&wq->queue);
      TAILQ_REMOVE (&wq->queue, th, sched_list);
    }
  waitq_unlock (wq);

  if (th == NULL)
    return false;

  thread_lock (th);
  assert ((th->status == SCHED_STOPPED) || th->sched_op.op_suspend);
  assert (th->waitq == wq);
  timer_remove (&th->timeout);
  th->waitq = NULL;

  _sched_resume(th);
  thread_unlock (th);
  return true;
}

void
thread_destroy (struct thread *th)
{
  thread_lock (th);
  if (th->waitq != NULL)
    {
      waitq_lock (th->waitq);
      TAILQ_REMOVE (&th->waitq->queue, th, sched_list);
      waitq_lock (th->waitq);

      timer_remove (&th->timeout);
      th->waitq = NULL;
    }

  _sched_destroy (th);

  /*
    Unlink the kernel port now, so that we have no more new
    references.
  */
  port_unlink_kernel(&th->self);

  thread_unlock (th);
}

void
thread_suspend (struct thread *th)
{
  thread_lock (th);
  _sched_suspend (th);
  thread_unlock (th);
}

void
thread_resume (struct thread *th)
{
  thread_lock (th);
  _sched_resume (th);
  thread_unlock (th);
}

#if 0
void
thread_vtalrm (int64_t diff)
{
  struct thread *th = cur_thread ();
  struct timer *t = &th->vtt_alarm;

  thread_lock (th);
  timer_remove (t);
  t->time = timer_gettime () + diff;
  t->opq = (void *) th;
  t->handler = NULL;
  t->valid = 1;
  debug ("Setting vtalm at %" PRIx64 "\n", t->time);
  timer_register (t);
  thread_unlock (th);
}
#endif

void
thread_init (void)
{

  slab_register (&threads, "THREADS",
		 sizeof (struct thread) + sizeof (uctxt_t), NULL, 1);
}
