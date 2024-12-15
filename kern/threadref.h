/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __THREADREF_H__
#define __THREADREF_H__

struct thread;
typedef struct threadref {
       struct thread *obj;
} threadref_t;

#define THREADREF_NULL ((struct threadref){.obj = NULL})

unsigned long *thread_refcnt(struct thread *obj);
void thread_zeroref(struct thread *obj);

static inline void _thread_inc(struct thread *obj)
{
  if (obj != NULL)
    {
      unsigned long cnt, *ptr;

      ptr = thread_refcnt(obj);
      cnt = __atomic_add_fetch (ptr, 1, __ATOMIC_ACQUIRE);
      assert (cnt != 0);
    }
}

static inline unsigned long _thread_dec(struct thread *obj)
{
  unsigned long cnt = 0;

  if (obj != NULL)
    {
      unsigned long *ptr = thread_refcnt(obj);
      cnt = __atomic_fetch_sub (ptr, 1, __ATOMIC_RELEASE);
      assert (cnt != 0);
    }
  return cnt - 1;
}

static inline bool
threadref_isnull (struct threadref *ref)
{
  return ref->obj == NULL;
}

static inline struct threadref
threadref_dup (struct threadref *ref)
{
  _thread_inc(ref->obj);
  return *ref;
}

static inline struct threadref
threadref_fromraw (struct thread *ptr)
{
  if (ptr == NULL)
    return THREADREF_NULL;

  _thread_inc(ptr);
  return (struct threadref){.obj = ptr};
}

static inline struct thread *
threadref_unsafe_get (struct threadref *ref)
{
  return ref->obj;
}

static inline void
threadref_consume (struct threadref *ref)
{
  if (_thread_dec(ref->obj) == 0)
    thread_zeroref(ref->obj);
  ref->obj = NULL;
}

static inline void
threadref_move (struct threadref *to, struct threadref *from)
{
  *to = *from;
  from->obj = NULL;
}


#endif

