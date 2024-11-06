/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

static lock_t sched_lock = 0;
static TAILQ_HEAD(thread_list, thread) running_threads = TAILQ_HEAD_INITIALIZER (running_threads);
static TAILQ_HEAD (, thread) stopped_threads = TAILQ_HEAD_INITIALIZER (stopped_threads);

void
sched_add(struct thread *th)
{
  th->status = SCHED_RUNNABLE;
  spinlock (&sched_lock);
  TAILQ_INSERT_TAIL (&running_threads, th, sched_list);
  spinunlock (&sched_lock);

  cpu_kick ();
}

uctxt_t *
sched_next (void)
{
  struct thread *oldth = cur_thread();
  struct thread *newth = NULL;
  enum sched newst = cur_thread()->status;

  if (newst == SCHED_RUNNING)
    return oldth->uctxt;

  printf("Old thread = %p (%d)\n", oldth, thread_isidle(oldth));

  if (newst == SCHED_RUNNABLE)
    newth = oldth;
  else
    newth = cur_cpu()->idle;

  spinlock (&sched_lock);
  if (!TAILQ_EMPTY (&running_threads))
    {
      newth = TAILQ_FIRST (&running_threads);
      TAILQ_REMOVE (&running_threads, newth, sched_list);
      newth->status = SCHED_RUNNING;
      newth->cpu = cpu_num();
    }
  spinunlock (&sched_lock);

  if (newth == oldth)
    goto _skip_resched;

  oldth->vtt_offset = cur_vtt ();
  if (oldth->vtt_alarm.valid)
    {
      uint64_t ctime = timer_gettime();
      uint64_t ttime = oldth->vtt_alarm.time;
      oldth->vtt_almdiff = ttime > ctime ? ttime - ctime : 1;
    }
  timer_remove (&oldth->vtt_alarm);

  thread_enter (newth);

  newth->vtt_rttbase = timer_gettime ();
  if (newth->vtt_almdiff)
    {
      thread_vtalrm (newth->vtt_almdiff);
    }

  if (thread_isidle (oldth))
    goto _skip_resched;

  switch (oldth->status)
    {
    case SCHED_RUNNABLE:
      spinlock (&sched_lock);
      TAILQ_INSERT_TAIL (&running_threads, oldth, sched_list);
      spinunlock (&sched_lock);
      break;
    case SCHED_STOPPED:
      spinlock (&sched_lock);
      TAILQ_INSERT_TAIL (&stopped_threads, oldth, sched_list);
      spinunlock (&sched_lock);
      break;
    default:
      fatal ("Unknown schedule state %d\n", oldth->status);
    }

 _skip_resched:
  printf("cpu %d: New thread: %p New uctxt: %p\n", cpu_id(), cur_thread(), cur_thread()->uctxt);
  return cur_thread()->uctxt;
}

void
sched_wake (struct thread *th)
{
  spinlock (&sched_lock);
  switch (th->status)
    {
    case SCHED_RUNNABLE:
      cpu_kick();
      break;
    case SCHED_RUNNING:
      cpu_ipi(th->cpu);
      break;
    case SCHED_STOPPED:
      TAILQ_REMOVE (&stopped_threads, th, sched_list);
      th->status = SCHED_RUNNABLE;
      TAILQ_INSERT_TAIL (&running_threads, th, sched_list);
      cpu_kick();
      break;
    default:
      fatal ("Unknown schedule state %d\n", th->status);
    }
  spinunlock (&sched_lock);
}
