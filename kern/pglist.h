/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef MACHINA_PGLIST_H
#define MACHINA_PGLIST_H

#include <nux/types.h>
#include <string.h>

struct pglist
{
  lock_t lock;
    LIST_HEAD (, physpage) list;
  unsigned long pages;
};

static inline void
pglist_init (struct pglist *pl)
{
  memset (pl, 0, sizeof (*pl));
}

static inline struct physpage *
pglist_rem (struct pglist *pl)
{
  struct physpage *pg;

  spinlock (&pl->lock);
  pg = LIST_FIRST (&pl->list);
  if (pg)
    {
      LIST_REMOVE (pg, list_entry);
      pl->pages--;
    }
  spinunlock (&pl->lock);

  return pg;
}

static inline void
pglist_add (struct pglist *pl, struct physpage *pg)
{
  spinlock (&pl->lock);
  LIST_INSERT_HEAD (&pl->list, pg, list_entry);
  pl->pages++;
  spinunlock (&pl->lock);
}

static inline unsigned long
pglist_pages (struct pglist *pl)
{
  unsigned long pages;
  spinlock (&pl->lock);
  pages = pl->pages;
  spinunlock (&pl->lock);
  return pages;
}

#endif
