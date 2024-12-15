/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __VMOBJREF_H__
#define __VMOBJREF_H__

struct vmobj;
typedef struct vmobjref {
       struct vmobj *obj;
} vmobjref_t;

#define VMOBJREF_NULL ((struct vmobjref){.obj = NULL})

unsigned long *vmobj_refcnt(struct vmobj *obj);
void vmobj_zeroref(struct vmobj *obj);

static inline void _vmobj_inc(struct vmobj *obj)
{
  if (obj != NULL)
    {
      unsigned long cnt, *ptr;

      ptr = vmobj_refcnt(obj);
      cnt = __atomic_add_fetch (ptr, 1, __ATOMIC_ACQUIRE);
      assert (cnt != 0);
    }
}

static inline unsigned long _vmobj_dec(struct vmobj *obj)
{
  unsigned long cnt = 0;

  if (obj != NULL)
    {
      unsigned long *ptr = vmobj_refcnt(obj);
      cnt = __atomic_fetch_sub (ptr, 1, __ATOMIC_RELEASE);
      assert (cnt != 0);
    }
  return cnt - 1;
}

static inline bool
vmobjref_isnull (struct vmobjref *ref)
{
  return ref->obj == NULL;
}

static inline struct vmobjref
vmobjref_dup (struct vmobjref *ref)
{
  _vmobj_inc(ref->obj);
  return *ref;
}

static inline struct vmobjref
vmobjref_fromraw (struct vmobj *ptr)
{
  if (ptr == NULL)
    return VMOBJREF_NULL;

  _vmobj_inc(ptr);
  return (struct vmobjref){.obj = ptr};
}

static inline struct vmobj *
vmobjref_unsafe_get (struct vmobjref *ref)
{
  return ref->obj;
}

static inline void
vmobjref_consume (struct vmobjref *ref)
{
  if (_vmobj_dec(ref->obj) == 0)
    vmobj_zeroref(ref->obj);
  ref->obj = NULL;
}

static inline void
vmobjref_move (struct vmobjref *to, struct vmobjref *from)
{
  *to = *from;
  from->obj = NULL;
}


#endif

