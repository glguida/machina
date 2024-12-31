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
  spinlock (&clock_lock);
  spinlock (&page->lock);
  TAILQ_REMOVE (&clock_queue, page, pageq);
  spinunlock (&page->lock);
  spinunlock (&clock_lock);
}
