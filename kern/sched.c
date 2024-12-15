/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

static lock_t sched_lock = 0;
/**INDENT-OFF**/
static TAILQ_HEAD (thread_list, thread) runnable_threads = TAILQ_HEAD_INITIALIZER (runnable_threads);
/**INDENT-ON**/

void
_sched_add (struct thread *th)
{
  th->suspend = 1;
  th->status = SCHED_STOPPED;
}

void
_sched_suspend (struct thread *th)
{
  switch (th->status)
    {

    case SCHED_RUNNING:
      th->sched_op.op_suspend = true;
      break;

    case SCHED_RUNNABLE:
      assert (th->suspend == 0);
      spinlock (&sched_lock);
      TAILQ_REMOVE (&runnable_threads, th, sched_list);
      spinunlock (&sched_lock);
      th->status = SCHED_STOPPED;
      th->suspend = 1;
      break;

    case SCHED_STOPPED:
      assert (th->suspend != 0);
      th->suspend++;
      break;

    case SCHED_REMOVED:
      break;

    default:
      fatal ("Invalid thread status: %d (thread: %p)",
	     th->status, th);
      break;
    }
}

void
_sched_resume (struct thread *th)
{
  bool resumed = false;

  switch (th->status)
    {

    case SCHED_RUNNING:
      assert (th->sched_op.op_suspend == true);
      /*
         Hasn't been suspended yet.
      */
      th->sched_op.op_suspend = false;
      break;

    case SCHED_RUNNABLE:
      /*
	Something has gone wrong with the suspend count.
      */
      fatal ("Resuming runnable thread %p\n", th);
      break;

    case SCHED_STOPPED:
      assert(th->suspend != 0);
      if (--th->suspend == 0)
	{
	  th->status = SCHED_RUNNABLE;
	  spinlock (&sched_lock);
	  TAILQ_INSERT_TAIL (&runnable_threads, th, sched_list);
	  spinunlock (&sched_lock);
	  resumed = true;
	}
      break;

    case SCHED_REMOVED:
      /*
	NOT resuming a thread about to be destroyed.
      */
      break;

    default:
      fatal ("Invalid thread status: %d (thread: %p)",
	     th->status, th);
      break;
    }

  if (resumed)
    cpu_kick ();
}

void
_sched_abort (struct thread *th)
{
  assert (th->waitq == NULL);

  switch (th->status)
    {

    case SCHED_STOPPED:
      _sched_resume (th);
      break;

    case SCHED_RUNNING:
    case SCHED_RUNNABLE:
    case SCHED_REMOVED:
      break;

    default:
      fatal ("Invalid thread status: %d (thread: %p)",
	     th->status, th);
      break;
    }
}

void
_sched_destroy (struct thread *th)
{
  switch (th->status)
    {
    case SCHED_RUNNING:
      th->sched_op.op_destroy = true;
      cpu_ipi(th->cpu);
      break;

    case SCHED_RUNNABLE:
      spinlock(&sched_lock);
      TAILQ_REMOVE (&runnable_threads, th, sched_list);
      spinunlock(&sched_lock);
      th->status = SCHED_REMOVED;
      TAILQ_INSERT_TAIL (&cur_cpu ()->dead_threads, th, sched_list);
      break;

    case SCHED_STOPPED:

      th->status = SCHED_REMOVED;
      TAILQ_INSERT_TAIL (&cur_cpu ()->dead_threads, th, sched_list);
      break;

    case SCHED_REMOVED:
      /* Is this a NOP? */
      fatal ("Thread %p already removed", th);
      break;

    default:
      fatal ("Invalid thread status: %d (thread: %p)",
	     th->status, th);
      break;
    }
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
      if (curth->waitq != NULL)
	{
	  spinlock (&curth->waitq->lock);
	  TAILQ_REMOVE (&curth->waitq->queue, curth, sched_list);
	  spinunlock (&curth->waitq->lock);

	  timer_remove (&curth->timeout);
	  curth->waitq = NULL;
	}
      curth->status = SCHED_REMOVED;
      TAILQ_INSERT_TAIL (&cur_cpu ()->dead_threads, curth, sched_list);
      curth->sched_op.op_destroy = false;
    }
  else if (curth->sched_op.op_suspend)
    {
      curth->status = SCHED_STOPPED;
      curth->suspend += 1;
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
  spinunlock (&curth->lock);

  /*
    Actually switch threads here.
  */
  if (thread_isidle (curth))
    atomic_cpumask_clear (&idlemap, cpu_id ());

  spinlock (&newth->lock);

  if (thread_isidle (newth))
    {
      cpu_umap_exit ();
      cur_cpu ()->task = NULL;
    }
  else
    {
      vmmap_enter (&newth->task->vmmap);
      cur_cpu ()->task = newth->task;
    }
  cur_cpu ()->thread = newth;

  if (thread_isidle (newth))
    atomic_cpumask_set (&idlemap, cpu_id ());

  newth->status = SCHED_RUNNING;
  newth->cpu = cpu_id ();
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
  return cur_thread ()->uctxt;
}

