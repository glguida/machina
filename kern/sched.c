/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <machina/error.h>

static lock_t sched_lock = 0;
/**INDENT-OFF**/
static TAILQ_HEAD (thread_list, thread) runnable_threads = TAILQ_HEAD_INITIALIZER (runnable_threads);
static TAILQ_HEAD (, thread) stopped_threads = TAILQ_HEAD_INITIALIZER (stopped_threads);
/**INDENT-ON**/

void
sched_add (struct thread *th)
{

  spinlock (&th->lock);
  th->status = SCHED_RUNNABLE;
  spinlock (&sched_lock);
  TAILQ_INSERT_TAIL (&runnable_threads, th, sched_list);
  spinunlock (&sched_lock);
  spinunlock (&th->lock);
  cpu_kick ();
}

uctxt_t *
sched_next (void)
{
  struct thread *curth = cur_thread ();
  struct thread *newth;

  spinlock (&curth->lock);

  if (thread_isidle (curth))
    goto _skip_sched_ops;

  assert (curth->status == SCHED_RUNNING);
  if (curth->sched_op.op_destroy)
    {
      curth->status = SCHED_REMOVED;
      TAILQ_INSERT_TAIL (&cur_cpu ()->dead_threads, curth, sched_list);
      curth->sched_op.op_destroy = false;
    }
  else if (curth->sched_op.op_suspend)
    {
      curth->status = SCHED_STOPPED;
      curth->sched_op.op_suspend = false;
    }
  else if (curth->sched_op.op_yield)
    {
      curth->status = SCHED_RUNNABLE;
      spinlock (&sched_lock);
      TAILQ_INSERT_TAIL (&runnable_threads, curth, sched_list);
      spinunlock (&sched_lock);
      curth->sched_op.op_yield = false;
    }
  else
    {
      spinunlock (&curth->lock);
      return curth->uctxt;
    }

_skip_sched_ops:

  spinlock (&sched_lock);
  if (!TAILQ_EMPTY (&runnable_threads))
    {
      newth = TAILQ_FIRST (&runnable_threads);
      TAILQ_REMOVE (&runnable_threads, newth, sched_list);
      spinunlock (&sched_lock);
      newth->status = SCHED_RUNNING;
      newth->cpu = cpu_id ();
    }
  else
    {
      spinunlock (&sched_lock);
      newth = cur_cpu ()->idle;
    }

  if (newth == curth)
    {
      spinunlock (&curth->lock);
      goto _skip_resched;
    }

  curth->vtt_offset = cur_vtt ();
  if (curth->vtt_alarm.valid)
    {
      uint64_t ctime = timer_gettime ();
      uint64_t ttime = curth->vtt_alarm.time;
      curth->vtt_almdiff = ttime > ctime ? ttime - ctime : 1;
    }
  timer_remove (&curth->vtt_alarm);
  spinunlock (&curth->lock);

  thread_enter (newth);

  spinlock (&newth->lock);
  newth->vtt_rttbase = timer_gettime ();
#if 0
  if (newth->vtt_almdiff)
    {
      thread_vtalrm (newth->vtt_almdiff);
    }
#endif
  spinunlock (&newth->lock);

_skip_resched:
  assert (cur_thread () == newth);
  //    printf("cpu %d: New thread: %p New uctxt: %p\n", cpu_id(), cur_thread(), cur_thread()->uctxt);
  return cur_thread ()->uctxt;
}


void
sched_destroy (struct thread *th)
{
  spinlock (&th->lock);
  th->sched_op.op_destroy = true;
  spinunlock (&th->lock);
}

bool
sched_abort (struct thread *th)
{
  if (th->waitq == NULL)
    return false;

  spinlock (&th->waitq->lock);
  TAILQ_REMOVE (&th->waitq->queue, th, sched_list);
  spinunlock (&th->waitq->lock);
  if (th->status == SCHED_RUNNING)
    {
      /*
         Hasn't been suspended yet.
       */
      assert (th->sched_op.op_suspend == true);
      th->sched_op.op_suspend = false;
    }
  else
    {
      assert (th->status == SCHED_STOPPED);
      th->status = SCHED_RUNNABLE;
      spinlock (&sched_lock);
      TAILQ_INSERT_TAIL (&runnable_threads, th, sched_list);
      spinunlock (&sched_lock);
    }
  th->waitq = NULL;
  return true;
}

void
__waitq_timeout_handler (void *opq)
{
  struct thread *th = (void *) opq;

  spinlock (&th->lock);
  /* 
     A wakeone might have happened. So in case there was no wait
     queue, we return.
   */
  if (sched_abort (th))
    uctxt_setret (th->uctxt, KERN_THREAD_TIMEDOUT);
  spinunlock (&th->lock);
}

void
sched_wait (struct waitq *wq, unsigned long timeout)
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
  spinlock (&wq->lock);
  assert (curth->status == SCHED_RUNNING);
  assert (curth->waitq == NULL);
  curth->sched_op.op_suspend = true;
  curth->waitq = wq;
  TAILQ_INSERT_TAIL (&wq->queue, curth, sched_list);
  spinunlock (&wq->lock);
  spinunlock (&curth->lock);
}

void
sched_wakeone (struct waitq *wq)
{
  bool kick = false;
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

  spinlock (&th->lock);
  timer_remove (&th->timeout);
  if (th->status == SCHED_RUNNING)
    {
      /*
         Hasn't been suspended yet.
       */
      assert (th->sched_op.op_suspend == true);
      assert (th->waitq == wq);
      th->sched_op.op_suspend = false;
    }
  else
    {
      assert (th->status == SCHED_STOPPED);
      th->status = SCHED_RUNNABLE;
      spinlock (&sched_lock);
      TAILQ_INSERT_TAIL (&runnable_threads, th, sched_list);
      spinunlock (&sched_lock);
      kick = true;
    }
  th->waitq = NULL;
  spinunlock (&th->lock);

  if (kick)
    cpu_kick ();
}
