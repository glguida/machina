/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef MACHINA_REF_H
#define MACHINA_REF_H

/*
  Machina Object Refcounting. Or Poor Man's ARC.

  Shared pointers are stored in Machina through ref type, that are
  pointers embedded in a structure.

  The type reference must defined an unsigned long '_ref_count' field.
*/
#define REF_SWAP(_ptr, _new)			\
  ({						\
    typeof((_r)) old = (_r);			\
    *(_ptr) = (_new);				\
    old;					\
  })

#define REF_DUP(_r)							\
  ({									\
    unsigned long cnt;							\
    cnt = __atomic_add_fetch (&(_r).obj->_ref_count, 1, __ATOMIC_ACQUIRE); \
    assert (cnt != 0);							\
    (_r);								\
  })

#define REF_MOVE(_r)				\
  ({						\
    typeof((_r)) new;				\
    new = (_r);					\
    (_r).obj = NULL;				\
    new;					\
  })

#define REF_DESTROY(_r)							\
  ({									\
    unsigned long cnt = 0;						\
    if ((_r).obj != NULL)						\
      {									\
	cnt = __atomic_fetch_sub (&(_r).obj->_ref_count, 1, __ATOMIC_RELEASE); \
	assert (cnt != 0);						\
	(_r).obj = NULL;						\
    }									\
    cnt - 1;								\
  })

/* Get the pointer of the reference.

   Note: This pointer is only meant to be used locally, and not stored
   in a shared variable.
*/
#define REF_GET(_r) ((_r).obj)


#endif
