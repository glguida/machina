/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __PORTREF_H__
#define __PORTREF_H__

struct port;
typedef struct portref {
       struct port *obj;
} portref_t;

#define PORTREF_NULL ((struct portref){.obj = NULL})

unsigned long *port_refcnt(struct port *obj);
void port_zeroref(struct port *obj);

static inline void _port_inc(struct port *obj)
{
  if (obj != NULL)
    {
      unsigned long cnt, *ptr;

      ptr = port_refcnt(obj);
      cnt = __atomic_add_fetch (ptr, 1, __ATOMIC_ACQUIRE);
      assert (cnt != 0);
    }
}

static inline unsigned long _port_dec(struct port *obj)
{
  unsigned long cnt = 0;

  if (obj != NULL)
    {
      unsigned long *ptr = port_refcnt(obj);
      cnt = __atomic_fetch_sub (ptr, 1, __ATOMIC_RELEASE);
      assert (cnt != 0);
    }
  return cnt - 1;
}

static inline bool
portref_isnull (struct portref *ref)
{
  return ref->obj == NULL;
}

static inline struct portref
portref_dup (struct portref *ref)
{
  _port_inc(ref->obj);
  return *ref;
}

static inline struct portref
portref_fromraw (struct port *ptr)
{
  if (ptr == NULL)
    return PORTREF_NULL;

  _port_inc(ptr);
  return (struct portref){.obj = ptr};
}

static inline struct port *
portref_unsafe_get (struct portref *ref)
{
  return ref->obj;
}

static inline void
portref_consume (struct portref *ref)
{
  if (_port_dec(ref->obj) == 0)
    port_zeroref(ref->obj);
  ref->obj = NULL;
}

static inline void
portref_move (struct portref *to, struct portref *from)
{
  *to = *from;
  from->obj = NULL;
}


#endif

