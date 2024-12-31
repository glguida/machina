/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

/*
  The Memory Controller implements the page reclamation algorithm for
  the memcache.

  It is currently Clock Algorithm, storing pages in a queue.
*/

#include "internal.h"

static lock_t clock_lock;
/**INDENT-OFF**/
static TAILQ_HEAD (, physmem_page) clock_queue = TAILQ_HEAD_INITIALIZER(clock_queue);
/**INDENT-ON**/


void
memctrl_newpage (struct physmem_page *page)
{
  nuxperf_inc (&pmachina_clock_newpage);
  spinlock (&clock_lock);
  spinlock (&page->lock);
  TAILQ_INSERT_TAIL (&clock_queue, page, pageq);
  spinunlock (&page->lock);
  spinunlock (&clock_lock);
}

void
memctrl_delpage (struct physmem_page *page)
{
  nuxperf_inc (&pmachina_clock_delpage);
  spinlock (&clock_lock);
  spinlock (&page->lock);
  TAILQ_REMOVE (&clock_queue, page, pageq);
  spinunlock (&page->lock);
  spinunlock (&clock_lock);
}

/*
  A Clock Tick.
*/
void
memctrl_do_tick (void)
{
  struct physmem_page *page;

  nuxperf_inc (&pmachina_clock_tick);
  spinlock (&clock_lock);
  page = TAILQ_FIRST(&clock_queue);
  TAILQ_REMOVE (&clock_queue, page, pageq);
  TAILQ_INSERT_TAIL(&clock_queue, page, pageq);
  memcache_tick (page);
  spinunlock (&clock_lock);    
}


static unsigned long _memctrl_pending_ticks;

/*
  Enable a single tick on the clock.
*/
void
memctrl_tick_one (void)
{
  __atomic_fetch_add (&_memctrl_pending_ticks, 1, __ATOMIC_RELAXED);
}

void
memctrl_run_clock (void)
{
  unsigned long pending;

  pending = __atomic_exchange_n (&_memctrl_pending_ticks, 0, __ATOMIC_RELAXED);

  for (unsigned long i = 0; i < pending; i++)
    memctrl_do_tick ();
}
