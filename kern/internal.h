/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef MACHINA_INTERNAL_H
#define MACHINA_INTERNAL_H

#include <assert.h>
#include <rbtree.h>
#include <nux/hal.h>
#include <nux/nux.h>
#include <machina/message.h>


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
  KVA Share: Shared area between kernel and userspace.

*/
bool share_kva (vaddr_t va, size_t size, struct umap *umap, uaddr_t uaddr, bool uwr);
void unshare_kva(struct umap *umap, uaddr_t uaddr, size_t size);


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


/* Message Buffers.

  Each thread has a message buffer, a shared area between user and
  kernel used for IPC messages.
*/
struct msgbuf {
  vaddr_t kaddr;
  uaddr_t uaddr;
};

#define MSGBUF_PAGE_SHIFT 0
#define MSGBUF_PAGES (1 << MSGBUF_PAGE_SHIFT)
#define MSGBUF_SHIFT (MSGBUF_PAGE_SHIFT + PAGE_SHIFT)
#define MSGBUF_SIZE (1L << MSGBUF_SHIFT)
#define MSGBUF_ORDMAX 1
struct msgbuf_zentry;
LIST_HEAD (msgbuflist, msgbuf_zentry);
struct msgbuf_zone {
  rb_tree_t rbtree;
  uintptr_t opq;
  unsigned long bmap;
  struct msgbuflist zlist[MSGBUF_ORDMAX];
  unsigned nfree;
};


void msgbuf_init(void);
void msgbuf_new(struct msgbuf_zone *z, vaddr_t vastart, vaddr_t vaend);
bool msgbuf_alloc(struct umap *umap, struct msgbuf_zone *z, struct msgbuf *mb);
void msgbuf_free(struct umap *umap, struct msgbuf_zone *z, struct msgbuf *mb);


/*
  Machina Thread.

*/
struct thread {
  lock_t lock; /* PRIO: task > thread. */
  uctxt_t uctxt;
  LIST_ENTRY(thread) list_entry;
  struct task *task;
  struct msgbuf msgbuf;
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
  struct msgbuf_zone msgbuf_zone;
};

bool vmmap_allocmsgbuf (struct vmmap *map, struct msgbuf *msgbuf);
void vmmap_enter(struct vmmap *map);
void vmmap_bootstrap(struct vmmap *map);
void vmmap_setup(struct vmmap *map);


/*
  Port Space: a collection of port rights.

*/
struct portspace {
  lock_t lock;
  struct rb_tree idsearch_rb_tree;
  struct rb_tree portsearch_rb_tree;
};

struct portref;
struct portright;

void portspace_init(void);
void portspace_setup(struct portspace *ps);
void portspace_lock (struct portspace *ps);
void portspace_unlock (struct portspace *ps);
mcn_return_t portspace_insertright(struct portspace *ps, struct portright *pr, mcn_portid_t *idout);
mcn_return_t portspace_resolve(struct portspace *ps, uint8_t bits, mcn_portid_t id, struct portright *right);
mcn_msgioret_t portspace_resolve_sendmsg(struct portspace *ps,
					 uint8_t rembits, mcn_portid_t remid, struct portright *remright,
					 uint8_t locbits, mcn_portid_t locid, struct portright *locright);
mcn_return_t portspace_resolve_receive(struct portspace *ps, mcn_portid_t id, struct portref *portref);

void portspace_print(struct portspace *ps);


/*
  Ports.

*/

enum port_type {
  PORT_KERNEL,
  PORT_QUEUE,
  PORT_DEAD,
};


struct msgq_entry {
  TAILQ_ENTRY(msgq_entry) queue;
};

struct port_queue {
  struct portspace portspace;
  TAILQ_HEAD(, msgq_entry) msgq;
};

struct port {
  unsigned long _ref_count; /* Handled by portref. */

  lock_t lock;
  enum port_type type;
  union {
    struct {} kernel;
    struct {} dead;
    struct port_queue queue;
  };
};

void port_init(void);
bool port_dead(struct port *);
bool port_kernel(struct port *);
enum port_type port_type(struct port *);
//mcn_return_t port_enqueue(struct port *p, struct msgq_entry *, size_t size);
mcn_return_t port_alloc_kernel(void *ctx, struct portref *portref);
mcn_return_t port_alloc_queue(struct portref *portref);
struct portspace * port_getportspace(struct port *port);
void port_putportspace(struct port *port, struct portspace *ps);
mcn_return_t port_enqueue(struct port *port, mcn_msgheader_t *inmsgh,
			  struct portright *local_right, struct portright *remote_right,
			  volatile void *body, size_t bodysize);
mcn_return_t port_dequeue(mcn_portid_t recvid, struct port *port, struct portspace *outps, mcn_msgheader_t *outmsgh, size_t outsize);

struct port;
struct portref {
  struct port *obj;
};

static inline struct portspace *
portref_get_portspace(struct portref *pr)
{
  return port_getportspace(REF_GET(*pr));
}


/*
  Port Rights.

*/
enum portright_type {
  RIGHT_INVALID = 0,
  RIGHT_SEND,
  RIGHT_RECV,
  RIGHT_ONCE,
};

struct portright {
  enum portright_type type;
  struct portref portref;
};


#define portright_from_portref(_type, _portref)	\
  ({						\
    struct portright pr;			\
    pr.type = (_type);				\
    pr.portref = REF_MOVE(_portref);		\
    pr;						\
  })


static inline enum port_type
portright_porttype(struct portright *pr)
{
  return port_type(REF_GET(pr->portref));
}

static inline struct portref
portright_movetoportref(struct portright *pr)
{
  pr->type = RIGHT_INVALID;
  return REF_MOVE(pr->portref);
}

static inline void
portright_consume(struct portright *pr)
{
  pr->type = RIGHT_INVALID;
  /* XXX: DELETE IF */REF_DESTROY(pr->portref);
}

static inline struct port *
portright_unsafe_get(struct portright *pr)
{
  return REF_GET(pr->portref);
}

static inline void
portright_unsafe_put(struct port **port)
{
  *port = NULL;
}


/*
  Machina Task.

*/
struct task {
  lock_t lock; /* PRIO: task > thread. */
  struct vmmap vmmap;
  unsigned refcount;
  LIST_HEAD(,thread) threads;
  struct portspace portspace;
  mcn_portid_t task_self;
};

void task_init(void);
struct task *task_bootstrap(void);
void task_enter(struct task *t);
struct portspace * task_getportspace(struct task *t);
void task_putportspace(struct task *t, struct portspace *ps);
mcn_return_t task_addportright(struct task *t, struct portright *pr, mcn_portid_t *id);
mcn_return_t task_allocate_port(struct task *t, mcn_portid_t *newid);


/*
  Machina IPC.
*/
mcn_return_t ipc_msg(mcn_msgopt_t opt, mcn_portid_t recv, unsigned long timeout, mcn_portid_t notify);

/*
  Per-CPU Data.
*/
struct mcncpu {
  struct thread *thread;
  struct task *task;
  struct port_queue kernel_queue;
};

static inline struct mcncpu *
cur_cpu(void)
{
  return (struct mcncpu *)cpu_getdata();
}

static inline struct thread *
cur_thread(void)
{
  return cur_cpu()->thread;
}

static inline void *
cur_kmsgbuf(void)
{
  struct thread *t = cur_thread();

  return t == NULL ? NULL : (void *)t->msgbuf.kaddr;
}

static inline uaddr_t
cur_umsgbuf(void)
{
  struct thread *t = cur_thread();

  return t == NULL ? UADDR_INVALID : (uaddr_t)t->msgbuf.uaddr;
}

static inline struct task *
cur_task(void)
{
  return cur_cpu()->task;
}


/*
  Dual locks.

  Lock ordering here is implied by the pointer value.

*/
#define MIN(_a,_b) ((_a) < (_b) ? (_a) : (_b))
#define MAX(_a,_b) ((_a) > (_b) ? (_a) : (_b))

static inline void spinlock_dual(lock_t *a, lock_t *b)
{
  if (a == b)
    spinlock(a);
  else
    {
      /* Pointer value defines lock ordering. */
      spinlock(MIN(a,b));
      spinlock(MAX(a,b));
    }
}

static inline void spinunlock_dual(lock_t *a, lock_t *b)
{
  if (a == b)
    spinunlock(a);
  else
    {
      spinunlock(MAX(a,b));
      spinunlock(MIN(a,b));
    }
}

#endif
