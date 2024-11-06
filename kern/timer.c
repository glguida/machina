#include "internal.h"

lock_t timers_lock = 0;
static LIST_HEAD (, timer) timers = LIST_HEAD_INITIALIZER (timers);

void
timer_run (void)
{
  int updated;
  uint64_t cnt = timer_gettime ();
  struct timer *c, *t;

  spinlock (&timers_lock);
  updated = 0;
  LIST_FOREACH_SAFE (c, &timers, list, t)
  {
    if (c->time > cnt)
      break;
    if (c->handler != NULL)
      {
	c->handler (cnt);
      }
    c->valid = 0;
    LIST_REMOVE (c, list);
    updated = 1;
  }
  if (updated)
    {
      /* Update hw timer. */
      c = LIST_FIRST (&timers);
      if (c == NULL)
	timer_clear ();
      else
	timer_alarm (c->time);
    }
  spinunlock (&timers_lock);
}

void
timer_register (struct timer *t)
{
  struct timer *c;

  if (!t->valid)
    return;

  spinlock (&timers_lock);
  c = LIST_FIRST (&timers);
  if (c == NULL)
    {
      LIST_INSERT_HEAD (&timers, t, list);
      goto out;
    }
  /* XXX: rollover */
  while (t->time > c->time)
    {
      if (LIST_NEXT (c, list) == NULL)
	{
	  LIST_INSERT_AFTER (c, t, list);
	  goto out;
	}
      c = LIST_NEXT (c, list);
    }
  LIST_INSERT_AFTER (c, t, list);
out:
  /* Update hw timer. */
  c = LIST_FIRST (&timers);
  timer_alarm (c->time);
  spinunlock (&timers_lock);
}

void
timer_remove (struct timer *timer)
{
  struct timer *c;

  if (!timer->valid)
    return;
  spinlock (&timers_lock);
  LIST_REMOVE (timer, list);
  /* Update hw timer. */
  c = LIST_FIRST (&timers);
  if (c == NULL)
    timer_clear ();
  else
    timer_alarm (c->time);
  spinunlock (&timers_lock);
  timer->valid = 0;
}

