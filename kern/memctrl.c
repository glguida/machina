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
static TAILQ_HEAD(, physmem_page) clock_queue;


void
memctrl_newpage(struct physmem_page *page)
{
  spinlock (&clock_lock);
  TAILQ_INSERT_HEAD(&clock_queue, page, pageq);
  spinunlock(&clock_lock);
}
