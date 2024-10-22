/*
  NUX: A kernel Library.
  Copyright (C) 2019-2024 Gianluca Guida <glguida@tlbflush.org>
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _ALLOC_H
#define _ALLOC_H

#include <limits.h>
#include <stdint.h>
#include <assert.h>
#include <queue.h>
#include <string.h>

#ifndef ALLOCFUNC
#define ALLOCFUNC(...) zone_##__VA_ARGS__
#endif

#ifdef ALLOCDEBUG
#define dbgprintf(...) printf (__VA_ARGS__)
#else
#define dbgprintf(...)
#endif

#ifndef __ZONE_T
#error __ZADDR_T not defined
#else
typedef __ZONE_T zone_t;
#endif

#ifndef __ZADDR_T
#error __ZADDR_T not defined
#else
typedef __ZADDR_T zaddr_t;
#endif

#ifndef __ZORDMAX
#define __ZORDMAX LONG_BIT
#endif

#define ORDMAX __ZORDMAX

/* Basic zentry structure must contain these fields:

   struct zentry {
       LIST_ENTRY(zentry) list;
       zaddr_t addr;
       size_t size;
   };

zone_t should be instead:

  struct __ZENTRY;
  LIST_HEAD (zlist, __ZENTRY);
  struct zone
  {
    uintptr_t opq;
    unsigned long bmap;
    struct zlist zlist[ORDMAX];
    unsigned nfree;
  };
   
*/

static inline unsigned
lsbit (unsigned long x)
{
  assert (x != 0);
  return __builtin_ffsl (x) - 1;
}

static inline unsigned
msbit (unsigned long x)
{
  assert (x != 0);
  return LONG_BIT - __builtin_clzl (x) - 1;
}

static inline void
_zone_detachentry (__ZONE_T *z, struct __ZENTRY *ze)
{
  uint32_t msb;

  assert (ze->size != 0);
  msb = MIN(ORDMAX - 1, msbit (ze->size));
  assert (msb < ORDMAX);

  LIST_REMOVE (ze, list);
  dbgprintf ("LIST_REMOVE: %p (%lx ->", ze, z->bmap);
  if (LIST_EMPTY (z->zlist + msb))
    z->bmap &= ~(1UL << msb);
  dbgprintf (" %lx)", z->bmap);
  z->nfree -= ze->size;
  dbgprintf ("D<%p>(%lx,%lx)", ze, ze->addr, ze->size);
}

static inline void
_zone_attachentry (__ZONE_T *z, struct __ZENTRY *ze)
{
  uint32_t msb;

  assert (ze->size != 0);
  msb = MIN(ORDMAX - 1, msbit (ze->size));
  assert (msb < ORDMAX);

  dbgprintf ("LIST_INSERT(%p + %d, %p), bmap (%lx ->", z->zlist, msb,
	     ze, z->bmap);
  z->bmap |= (1UL << msb);
  dbgprintf (" %lx", z->bmap);


  LIST_INSERT_HEAD (z->zlist + msb, ze, list);
  z->nfree += ze->size;
  dbgprintf ("A<%p>(%lx,%lx)", ze, ze->addr, ze->size);
}

static inline struct __ZENTRY *
_zone_findfree (__ZONE_T *zn, size_t size)
{
  unsigned long tmp;
  unsigned int minbit;
  struct __ZENTRY *ze = NULL;

  minbit = msbit (size);

  if (size != (1 << minbit))
    minbit += 1;

  if (minbit >= ORDMAX)
    {
      /* Wrong size */
      return NULL;
    }

  tmp = zn->bmap >> minbit;
  if (tmp)
    {
      tmp = lsbit (tmp);
      ze = LIST_FIRST (zn->zlist + minbit + tmp);
      dbgprintf ("LIST_FIRST(%p + %d + %d) = %p", zn->zlist, minbit, tmp, ze);
    }
  return ze;
}

static inline void
zone_remove (__ZONE_T *z, struct __ZENTRY *ze)
{
  _zone_detachentry (z, ze);
  printf("FREEPTR\n");
  ___freeptr (ze, z->opq);
}

static inline void
zone_create (__ZONE_T *z, zaddr_t zaddr, size_t size)
{
  struct __ZENTRY *ze, *pze = NULL, *nze = NULL;
  zaddr_t fprev = zaddr, lnext = zaddr + size;
  dbgprintf ("HHH");
  ___get_neighbors (zaddr, size, &pze, &nze, z->opq);
  dbgprintf ("HHH");
  if (pze)
    {
      fprev = pze->addr;
      zone_remove (z, pze);
    }
  dbgprintf ("HHH");
  if (nze)
    {
      lnext = nze->addr + nze->size;
      zone_remove (z, nze);
    }
  dbgprintf ("HHH");
  ze = ___mkptr (fprev, lnext - fprev, z->opq);
  dbgprintf ("MKPTR(%p): %lx,%lx", ze, ze->addr, ze->size);
  _zone_attachentry (z, ze);
}


static inline void
zone_free (__ZONE_T *z, zaddr_t zaddr, size_t size)
{

  assert (size != 0);
  dbgprintf ("Freeing %lx", zaddr);
  zone_create (z, zaddr, size);
}

static inline zaddr_t
zone_alloc (__ZONE_T *z, size_t size)
{
  struct __ZENTRY *ze;
  zaddr_t addr = (zaddr_t) - 1;
  long diff;

  assert (size != 0);

  ze = _zone_findfree (z, size);
  if (ze == NULL)
    goto out;

  addr = ze->addr;
  diff = ze->size - size;
  assert (diff >= 0);
  zone_remove (z, ze);
  if (diff > 0)
    zone_create (z, addr + size, diff);

out:
  dbgprintf ("Allocating %lx", addr);
  return addr;
}

static inline void
zone_init (__ZONE_T *z, uintptr_t opq)
{
  int i;

  z->bmap = 0;
  z->nfree = 0;
  z->opq = opq;
  for (i = 0; i < ORDMAX; i++)
    LIST_INIT (z->zlist + i);
}

#endif
