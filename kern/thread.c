/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <nux/slab.h>
#include "internal.h"

struct slab threads;

struct thread *
thread_new(struct task *t)
{
  struct thread *th;

  th = slab_alloc(&threads);
  spinlock_init(&th->lock);

  spinlock(&t->lock);
  th->task = t;
  LIST_INSERT_HEAD(&t->threads, th, list_entry);
  uctxt_init(&th->uctxt, 0, 0);
  spinunlock(&t->lock);

  return th;
}

struct thread *
thread_bootstrap(struct task *t)
{
  struct thread *th;

  th = thread_new(t);
  if (th == NULL)
    {
      fatal("Cannot allocate boot thread.");
    }
  if (!uctxt_bootstrap (&th->uctxt))
    {
      fatal("No bootstrap process.");
    }
  return th;
}

void
thread_enter(struct thread *th)
{
  cur_cpu()->thread = th;

  spinlock(&th->lock);
  task_enter(th->task);
  spinunlock(&th->lock);
}

void thread_init (void)
{

  slab_register(&threads, "THREADS", sizeof(struct thread), NULL, 1);
}
