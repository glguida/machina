/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef MACHINA_INTERNAL_H
#define MACHINA_INTERNAL_H

#include <nux/hal.h>
#include <nux/nux.h>

/*
  RAM reserved for kernel allocation, when memory is low.

  This is the maximum amount. The real amount will be the minimum
  between `RESERVED_MEMORY` and 6.25% of total RAM.
*/
#define RESERVED_MEMORY (16*1024*1024)

/*
  Machina Physical Memory Handling.

  At boot, machina creates a `struct physpage` for each RAM page, and
  switches to a list-based allocator (as opposed to NUX bitmap-tree based one.

  Each page is typed, and each type contain associated data.
*/


void physmem_init(void);

#define TYPE_UNKNOWN 0 /* Page type is unknown. Either non-ram or early nux-allocated. */
#define TYPE_RESERVED 2 /* Reserved for system use in emergency settings. */
#define TYPE_FREE 3 /* Page available immediately for allocation. */
#define TYPE_STANDBY 4 /* Page available for allocation but still containing WSET data. */
#define TYPE_MODIFIED 5 /* Page contents need to return to pager before being reused. */
#define TYPE_WSET 6 /* Page being actively used by a working set. */
#define TYPE_SYSTEM 7 /* Page is allocated by system. */
#define TYPE_NONRAM 8 /* Page is not RAM or Firmware-allocated. */

struct physpage {
  LIST_ENTRY(physpage) list_entry;
  pfn_t pfn;
  uint8_t type;
  union {
    struct {
      uint8_t dirty :1;
      uint8_t accessed :1;
    } wset;
  } u;
};

#include "pglist.h"

/*
  Get the `struct physpage` associated with a PFN.
*/
struct physpage * physpage_get(pfn_t pfn);

/*
  Allocate a page for kernel use, if `mayfail` is false then allocate
  from the reserve memory.

  Note: NUX libraries will always allocate with `mayfail` false.
*/
pfn_t pfn_alloc_kernel(bool mayfail);
void pfn_free_kernel(pfn_t pfn);


/*
  Machina Thread.

*/
struct thread {
  lock_t lock; /* PRIO: task > thread. */
  uctxt_t uctxt;
  LIST_ENTRY(thread) list_entry;
  struct task *task;
};

struct thread * thread_new(struct task *t);
struct thread * thread_bootstrap(struct task *t);
void thread_enter(struct thread *th);
void thread_init (void);

static inline uctxt_t *
thread_uctxt(struct thread *th)
{
  return &th->uctxt;
}


/*
  Machina VM Map.

*/
struct vmmap {
  struct umap umap;
};

void vmmap_enter(struct vmmap *map);
void vmmap_bootstrap(struct vmmap *map);
void vmmap_setup(struct vmmap *map);


/*
  Machina Task.

*/
struct task {
  lock_t lock; /* PRIO: task > thread. */
  struct vmmap vmmap;
  unsigned refcount;
  LIST_HEAD(,thread) threads;
};

void task_init(void);
struct task *task_bootstrap(void);
void task_enter(struct task *t);


/*
  Per-CPU Data.
*/
struct mcncpu {
  struct thread *thread;
};

static inline struct mcncpu *
cur_cpu(void)
{
  return (struct mcncpu *)cpu_getdata();
}


#endif
