/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef MACHINA_INTERNAL_H
#define MACHINA_INTERNAL_H

#include <assert.h>
#include <rbtree.h>
#include <string.h>
#include <nux/hal.h>
#include <nux/nux.h>
#include <nux/cpumask.h>
#include <nux/slab.h>
#include <machina/kernparam.h>
#include <machina/message.h>

#include "ref.h"

/*
  RAM reserved for kernel allocation, when memory is low.

  This is the maximum amount. The real amount will be the minimum
  between `RESERVED_MEMORY` and 6.25% of total RAM.
*/
#define RESERVED_MEMORY (16*1024*1024)

/*
  Machina Physical Memory Handling.

  A page in machina can be unused, allocated by the kernel, or used by
  the physical memory cache.
*/
void physmem_init (void);

/*
  Allocate a page for kernel use, if `mayfail` is false then allocate
  from the reserve memory.

  Note: NUX libraries will always allocate with `mayfail` false.
*/
pfn_t pfn_alloc_kernel (bool mayfail);
void pfn_free_kernel (pfn_t pfn);


/*
  KVA Share: Shared area between kernel and userspace.

*/
bool share_kva (vaddr_t va, size_t size, struct umap *umap, uaddr_t uaddr,
		bool uwr);
void unshare_kva (struct umap *umap, uaddr_t uaddr, size_t size);


/*
  Message Buffers.

  Each thread has a message buffer, a shared area between user and
  kernel used for IPC messages.
*/
struct msgbuf
{
  vaddr_t kaddr;
  uaddr_t uaddr;
};

struct msgbuf_zone;
void msgbuf_init (void);
bool msgbuf_alloc (struct umap *umap, struct msgbuf_zone *z,
		   struct msgbuf *mb);
void msgbuf_free (struct umap *umap, struct msgbuf_zone *z,
		  struct msgbuf *mb);


/*
  Timers.
*/
/**INDENT-OFF**/
struct thread;
struct timer
{
  int valid;
  uint64_t time;
  void *opq;
  void (*handler) (void *opq);
  LIST_ENTRY (timer) list;
};
/**INDENT-ON**/

static inline void
timer_init (struct timer *t)
{
  t->valid = 0;
}

void timer_register (struct timer *t, uint64_t nsecs);
void timer_remove (struct timer *timer);
void timer_run (void);


/*

  Scheduler.

*/

extern cpumask_t idlemap;

void cpu_kick (void);
void ipc_kern_exec (void);
uctxt_t *kern_return (void);

/**INDENT-OFF**/
struct waitq
{
  lock_t lock;
  TAILQ_HEAD (, thread) queue;
};
/**INDENT-ON**/

static inline void
waitq_init (struct waitq *wq)
{
  spinlock_init (&wq->lock);
  TAILQ_INIT (&wq->queue);
}

static inline bool
waitq_empty (struct waitq *wq)
{
  bool empty;

  spinlock (&wq->lock);
  empty = TAILQ_EMPTY (&wq->queue);
  spinunlock (&wq->lock);

  return empty;
}

enum sched
{
  SCHED_RUNNING = 0,
  SCHED_RUNNABLE = 1,
  SCHED_STOPPED = 3,
  SCHED_REMOVED = 4,
};

void sched_add (struct thread *th);
uctxt_t *sched_next (void);
void sched_destroy (struct thread *th);
void sched_wait (struct waitq *waitq, unsigned long timeout);
void sched_wakeone (struct waitq *wq);


/*
  Machina Thread.

*/
/**INDENT-OFF**/
struct thread
{
  lock_t lock;			/* PRIO: task > thread. */
  uctxt_t *uctxt;
  LIST_ENTRY (thread) list_entry;
  struct task *task;
  struct msgbuf msgbuf;

  int64_t vtt_almdiff;
  uint64_t vtt_offset;
  uint64_t vtt_rttbase;
  struct timer vtt_alarm;
  unsigned cpu;

  uaddr_t tls;

  struct
  {
    uint8_t op_yield:1;
    uint8_t op_suspend:1;
    uint8_t op_destroy:1;
  } sched_op;
  struct waitq *waitq;
  struct timer timeout;
  enum sched status;
  TAILQ_ENTRY (thread) sched_list;
};
/**INDENT-ON**/

struct thread *thread_new (struct task *t, long ip, long sp, long gp);
struct thread *thread_idle (void);
struct thread *thread_bootstrap (struct task *t);
void thread_enter (struct thread *th);
void thread_init (void);
void thread_vtalrm (int64_t diff);

static inline uctxt_t *
thread_uctxt (struct thread *th)
{
  return th->uctxt;
}

static inline bool
thread_isidle (struct thread *th)
{
  return thread_uctxt (th) == UCTXT_IDLE;
}


/*
  Port Space: a collection of port rights.

*/
struct ipcspace
{
  struct rb_tree idsearch_rb_tree;
  struct rb_tree portsearch_rb_tree;
};

struct port;
struct portref;
struct portright;

void ipcspace_init (void);
void ipcspace_setup (struct ipcspace *ps);
mcn_portid_t ipcspace_lookup (struct ipcspace *ps, struct port *port);
mcn_return_t ipcspace_insertright (struct ipcspace *ps, struct portright *pr,
				   mcn_portid_t * idout);
mcn_return_t ipcspace_resolve (struct ipcspace *ps, uint8_t bits,
			       mcn_portid_t id, struct portref *pref);
mcn_msgioret_t ipcspace_resolve_sendmsg (struct ipcspace *ps, uint8_t rembits,
					 mcn_portid_t remid,
					 struct portref *rempref,
					 uint8_t locbits, mcn_portid_t locid,
					 struct portref *locpref);
mcn_return_t ipcspace_resolve_receive (struct ipcspace *ps, mcn_portid_t id,
				       struct portref *portref);

void ipcspace_print (struct ipcspace *ps);


/*
  Ports.

*/

enum port_type
{
  PORT_KERNEL,
  PORT_QUEUE,
  PORT_DEAD,
};


struct msgq_entry
{
  TAILQ_ENTRY (msgq_entry) queue;
  mcn_msgheader_t *msgh;
};

/**INDENT-OFF**/
typedef TAILQ_HEAD (msgqueue, msgq_entry) msgqueue_t;
/**INDENT-ON**/

void msgq_init (msgqueue_t * msgq);
mcn_return_t msgq_enq (msgqueue_t * msgq, mcn_msgheader_t * msgh);
bool msgq_deq (msgqueue_t * msgq, mcn_msgheader_t ** msghp);

struct port_queue
{
  struct waitq recv_waitq;
  struct waitq send_waitq;
  unsigned capacity;
  unsigned entries;
  msgqueue_t msgq;
};

void portqueue_init (struct port_queue *pq, unsigned limit);

enum kern_objtype
{
  KOT_TASK,
  KOT_THREAD,
  KOT_VMOBJ,
  KOT_VMOBJ_NAME,
};

struct port
{
  unsigned long _ref_count;	/* Handled by portref. */

  lock_t lock;
  enum port_type type;
  union
  {
    struct
    {
      void *obj;
      enum kern_objtype kot;
    } kernel;
    struct
    {
    } dead;
    struct port_queue queue;

  };
};

void port_init (void);
bool port_dead (struct port *);
bool port_kernel (struct port *);
enum port_type port_type (struct port *);
void port_alloc_kernel (void *obj, enum kern_objtype kot,
			struct portref *portref);
void *port_getkobj (struct port *port, enum kern_objtype kot);
mcn_return_t port_alloc_queue (struct portref *portref);
mcn_return_t port_enqueue (mcn_msgheader_t * msgh, unsigned long timeout,
			   bool force);
mcn_return_t port_dequeue (struct port *port, unsigned long timeout,
			   mcn_msgheader_t ** msghp);

struct portref
{
  struct port *obj;
};

typedef mcn_portid_t ipc_port_t;

static inline ipc_port_t
portref_to_ipcport (struct portref *portref)
{
  ipc_port_t ipcport;

  ipcport = (ipc_port_t) (uintptr_t) portref->obj;
  portref->obj = NULL;
  /* Portref effectively moved to ipcport. */
  return ipcport;
}

static inline struct portref
ipcport_to_portref (ipc_port_t * ipcport)
{
  struct portref portref;

  portref.obj = (struct port *) (uintptr_t) (*ipcport);
  *ipcport = 0;
  /* IPC port effectively moved to portref. */
  return portref;
}

static inline struct portref
portref_dup (struct portref *portref)
{
  return REF_DUP (*portref);
}

static inline struct port *
portref_unsafe_get (struct portref *portref)
{
  return REF_GET (*portref);
}

static inline void
portref_consume (struct portref *portref)
{
  /* XXX: DELETE IF */ REF_DESTROY (*portref);
}

static inline struct port *
ipcport_unsafe_get (ipc_port_t ipcport)
{
  return (struct port *) (uintptr_t) ipcport;
}

static inline bool
ipcport_isnull (ipc_port_t ipcport)
{
  return ipcport_unsafe_get (ipcport) == NULL;
}


/*
  Port Rights.

*/
enum portright_type
{
  RIGHT_INVALID = 0,
  RIGHT_SEND = MCN_MSGTYPE_PORTSEND,
  RIGHT_RECV = MCN_MSGTYPE_PORTRECV,
  RIGHT_ONCE = MCN_MSGTYPE_PORTONCE,
};

struct portright
{
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
portright_porttype (struct portright *pr)
{
  return port_type (REF_GET (pr->portref));
}

static inline struct portref
portright_movetoportref (struct portright *pr)
{
  pr->type = RIGHT_INVALID;
  return REF_MOVE (pr->portref);
}

static inline void
portright_consume (struct portright *pr)
{
  pr->type = RIGHT_INVALID;
  /* XXX: DELETE IF */ REF_DESTROY (pr->portref);
}

static inline struct port *
portright_unsafe_get (struct portright *pr)
{
  return REF_GET (pr->portref);
}

static inline void
portright_unsafe_put (struct port **port)
{
  *port = NULL;
}


#include "vm.h"


/*
  Machina Task.

*/
struct taskref;
/**INDENT-OFF**/
struct task
{
  lock_t lock;			/* PRIO: task > thread. */
  struct vmmap vmmap;
  unsigned _ref_count;
  LIST_HEAD (, thread) threads;
  struct ipcspace ipcspace;
  struct portref self;
};
/**INDENT-ON**/

void task_init (void);
void task_bootstrap (struct taskref *taskref);
void task_enter (struct task *t);
struct ipcspace *task_getipcspace (struct task *t);
void task_putipcspace (struct task *t, struct ipcspace *ps);
mcn_return_t task_addportright (struct task *t, struct portright *pr,
				mcn_portid_t * id);
mcn_return_t task_allocate_port (struct task *t, mcn_portid_t * newid);
mcn_return_t task_vm_map (struct task *t, vaddr_t *addr, size_t size, unsigned long mask, bool anywhere, struct vmobjref objref, mcn_vmoff_t off, bool copy, mcn_vmprot_t curprot, mcn_vmprot_t maxprot, mcn_vminherit_t inherit);
mcn_return_t task_vm_allocate (struct task *t, vaddr_t * addr, size_t size,
			       bool anywhere);
mcn_return_t task_vm_region (struct task *t, vaddr_t *addr, size_t *size, mcn_vmprot_t *curprot, mcn_vmprot_t *maxprot, mcn_vminherit_t *inherit, bool *shared, struct portref *portref, mcn_vmoff_t *off);
mcn_portid_t task_self (void);

struct taskref
{
  struct task *obj;
};

static inline struct taskref
taskref_dup (struct taskref *taskref)
{
  return REF_DUP (*taskref);
}

static inline struct task *
taskref_unsafe_get (struct taskref *taskref)
{
  return REF_GET (*taskref);
}

static inline void
taskref_consume (struct taskref taskref)
{
  /* XXX: DELETE IF */ REF_DESTROY (taskref);
}

/*
  Machina IPC.
*/
mcn_msgioret_t ipc_msgsend (mcn_msgopt_t opt, unsigned long timeout,
			    mcn_portid_t notify);
mcn_msgioret_t ipc_msgrecv (mcn_portid_t recv_port, mcn_msgopt_t opt,
			    unsigned long timeout, mcn_portid_t notify);

/*
  Per-CPU Data.
*/
/**INDENT-OFF**/
struct mcncpu
{
  struct thread *idle;
  struct thread *thread;
  struct task *task;
  struct msgqueue kernel_msgq;
  TAILQ_HEAD (, thread) dead_threads;
};
/**INDENT-ON**/

static inline struct mcncpu *
cur_cpu (void)
{
  return (struct mcncpu *) cpu_getdata ();
}

static inline struct thread *
cur_thread (void)
{
  return cur_cpu ()->thread;
}

static inline uint64_t
cur_vtt (void)
{
  return timer_gettime () - cur_thread ()->vtt_rttbase +
    cur_thread ()->vtt_offset;
}

static inline void *
cur_kmsgbuf (void)
{
  struct thread *t = cur_thread ();

  return t == NULL ? NULL : (void *) t->msgbuf.kaddr;
}

static inline uaddr_t
cur_umsgbuf (void)
{
  struct thread *t = cur_thread ();

  return t == NULL ? UADDR_INVALID : (uaddr_t) t->msgbuf.uaddr;
}

static inline struct task *
cur_task (void)
{
  return cur_cpu ()->task;
}


/*
  Dual locks.

  Lock ordering here is implied by the pointer value.

*/
#define MIN(_a,_b) ((_a) < (_b) ? (_a) : (_b))
#define MAX(_a,_b) ((_a) > (_b) ? (_a) : (_b))

static inline void
spinlock_dual (lock_t * a, lock_t * b)
{
  if (a == b)
    spinlock (a);
  else
    {
      /* Pointer value defines lock ordering. */
      spinlock (MIN (a, b));
      spinlock (MAX (a, b));
    }
}

static inline void
spinunlock_dual (lock_t * a, lock_t * b)
{
  if (a == b)
    spinunlock (a);
  else
    {
      spinunlock (MAX (a, b));
      spinunlock (MIN (a, b));
    }
}

#include <machina/message.h>

static inline const char *
typename_debug (mcn_msgtype_name_t name)
{
  switch (name)
    {
    case MCN_MSGTYPE_BIT:
      return "UNSTR";
    case MCN_MSGTYPE_INT16:
      return "INT16";
    case MCN_MSGTYPE_INT32:
      return "INT32";
    case MCN_MSGTYPE_INT8:
      return "INT8 ";
    case MCN_MSGTYPE_REAL:
      return "REAL ";
    case MCN_MSGTYPE_INT64:
      return "INT64";
    case MCN_MSGTYPE_STRING:
      return "CSTR ";
    case MCN_MSGTYPE_PORTNAME:
      return "PNAME";
    case MCN_MSGTYPE_MOVERECV:
      return "MVRCV";
    case MCN_MSGTYPE_MOVESEND:
      return "MVSND";
    case MCN_MSGTYPE_MOVEONCE:
      return "MVONC";
    case MCN_MSGTYPE_COPYSEND:
      return "CPSND";
    case MCN_MSGTYPE_MAKESEND:
      return "MKSND";
    case MCN_MSGTYPE_MAKEONCE:
      return "MKONC";
    default:
      return "?????";
    }
}

static inline void
message_debug (mcn_msgheader_t * msgh)
{
  void *ptr = (void *)(msgh + 1);
  void *end = (void *)msgh + msgh->msgh_size;

  printf ("===== Message %p =====\n", msgh);
  printf ("  bits:   [ R: %s - L: %s ]\n",
	  typename_debug (MCN_MSGBITS_REMOTE (msgh->msgh_bits)),
	  typename_debug (MCN_MSGBITS_LOCAL (msgh->msgh_bits)));
  printf ("  size:   %ld bytes\n", msgh->msgh_size);
  printf ("  remote: %lx\n", (long) msgh->msgh_remote);
  printf ("  local:  %lx\n", (long) msgh->msgh_local);
  printf ("  seqno:  %lx\n", (long) msgh->msgh_seqno);
  printf ("  msgid:  %ld\n", (long) msgh->msgh_msgid);
  while (ptr < end)
    {
      mcn_msgtype_t *ty = ptr;
      mcn_msgtype_long_t *longty = ptr;
      unsigned name, size, number;

      name = ty->msgt_longform ? longty->msgtl_name : ty->msgt_name;
      size = ty->msgt_longform ? longty->msgtl_size : ty->msgt_size;
      number = ty->msgt_longform ? longty->msgtl_number : ty->msgt_number;

      printf("  - %s (size: %d, number: %d, inline: %d, longform: %d, deallocate: %d)\n",
	     typename_debug(name),
	     size,
	     number,
	     ty->msgt_inline,
	     ty->msgt_longform,
	     ty->msgt_deallocate);

      ptr += ty->msgt_longform ? sizeof(mcn_msgtype_long_t) : sizeof(mcn_msgtype_t);
      if (ty->msgt_inline)
	{
	  ptr += (size >> 3) == 8 ? 4 : 0; /* Align. */
	  printf ("  - bytes: ");
	  for (int i = 0; i < (size >> 3); i++)
	    printf("%02x ", *(char *)ptr++);
	  printf("\n");
	}
    }

  printf ("==================================\n");
}

void ipcspace_debug (struct ipcspace *ps);

#endif
