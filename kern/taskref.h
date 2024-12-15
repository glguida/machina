/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __TASKREF_H__
#define __TASKREF_H__

struct task;
typedef struct taskref {
       struct task *obj;
} taskref_t;

#define TASKREF_NULL ((struct taskref){.obj = NULL})

unsigned long *task_refcnt(struct task *obj);
void task_zeroref(struct task *obj);

static inline void _task_inc(struct task *obj)
{
  if (obj != NULL)
    {
      unsigned long cnt, *ptr;

      ptr = task_refcnt(obj);
      cnt = __atomic_add_fetch (ptr, 1, __ATOMIC_ACQUIRE);
      assert (cnt != 0);
    }
}

static inline unsigned long _task_dec(struct task *obj)
{
  unsigned long cnt = 0;

  if (obj != NULL)
    {
      unsigned long *ptr = task_refcnt(obj);
      cnt = __atomic_fetch_sub (ptr, 1, __ATOMIC_RELEASE);
      assert (cnt != 0);
    }
  return cnt - 1;
}

static inline bool
taskref_isnull (struct taskref *ref)
{
  return ref->obj == NULL;
}

static inline struct taskref
taskref_dup (struct taskref *ref)
{
  _task_inc(ref->obj);
  return *ref;
}

static inline struct taskref
taskref_fromraw (struct task *ptr)
{
  if (ptr == NULL)
    return TASKREF_NULL;

  _task_inc(ptr);
  return (struct taskref){.obj = ptr};
}

static inline struct task *
taskref_unsafe_get (struct taskref *ref)
{
  return ref->obj;
}

static inline void
taskref_consume (struct taskref *ref)
{
  if (_task_dec(ref->obj) == 0)
    task_zeroref(ref->obj);
  ref->obj = NULL;
}

static inline void
taskref_move (struct taskref *to, struct taskref *from)
{
  *to = *from;
  from->obj = NULL;
}


#endif

