/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <stdio.h>
#include <nux/nux.h>
#include <nux/hal.h>
#include <machina/error.h>

#include "internal.h"
#include "vm.h"

#include <ku.h>

cpumask_t idlemap;

void
cpu_kick (void)
{
  cpu_ipi_mask (idlemap);
}

__thread char test_2[40];

struct taskref bootstrap_taskref;
struct thread *bootstrap;
static int smp_sync = 0;

mcn_return_t
testport_send (void *ctx, mcn_msgid_t id, void *data, size_t size,
	       struct portref reply)
{
  printf ("Received message: context %p id %d, data is at %p with size %ld\n",
	  ctx, id, data, size);

  return 0;
}

int
main (int argc, char *argv[])
{
  mcn_return_t rc;
  struct portref portref;

  info ("MACHINA Started.");
  physmem_init ();
  memcache_init ();
  msgbuf_init ();
  vmreg_init ();
  vmobj_init ();
  task_init ();
  thread_init ();
  port_init ();
  ipcspace_init ();

  /* Initialise per-CPU data. */
  cpu_setdata ((void *) kmem_alloc (0, sizeof (struct mcncpu)));
  msgq_init (&cur_cpu ()->kernel_msgq);
  cur_cpu ()->idle = thread_idle ();
  cur_cpu ()->thread = cur_cpu ()->idle;
  TAILQ_INIT (&cur_cpu ()->dead_threads);
  atomic_cpumask_set (&idlemap, cpu_id ());

  task_bootstrap (&bootstrap_taskref);
  struct thread *th =
    thread_bootstrap (taskref_unsafe_get (&bootstrap_taskref));
  hal_umap_load (NULL);
  sched_add (th);
  printf ("here");

  port_alloc_kernel (NULL, KOT_THREAD, &portref);
  struct portright testpr = portright_from_portref (RIGHT_SEND, portref);
  mcn_portid_t id;
  rc =
    task_addportright (taskref_unsafe_get (&bootstrap_taskref), &testpr, &id);
  printf ("Test id is %ld\n", id);
  assert (rc == KERN_SUCCESS);

  taskref_consume(&bootstrap_taskref);
  bootstrap = th;

  smp_sync = 1;
  __sync_synchronize ();
  cpu_ipi (cpu_id ());

  return EXIT_IDLE;
}

int
main_ap (void)
{
  while (!__sync_bool_compare_and_swap (&smp_sync, 1, 1));

  /* Initialise per-CPU data. */
  cpu_setdata ((void *) kmem_alloc (0, sizeof (struct mcncpu)));
  msgq_init (&cur_cpu ()->kernel_msgq);
  cur_cpu ()->idle = thread_idle ();
  cur_cpu ()->thread = cur_cpu ()->idle;
  TAILQ_INIT (&cur_cpu ()->dead_threads);
  atomic_cpumask_set (&idlemap, cpu_id ());

  return EXIT_IDLE;
}

uctxt_t *
kern_return (void)
{
  ipc_kern_exec ();

  return sched_next ();
}

uctxt_t *
entry_ipi (uctxt_t * uctxt)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;

  return kern_return ();
}

uctxt_t *
entry_alarm (uctxt_t * uctxt)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;

  info ("TMR: %" PRIu64 " us", timer_gettime ());
  uctxt_print (uctxt);
  timer_run ();
  return kern_return ();
}

uctxt_t *
entry_ex (uctxt_t * uctxt, unsigned ex)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;

  info ("Exception %d", ex);
  uctxt_print (uctxt);
  sched_destroy (cur_thread ());
  return kern_return ();
}

uctxt_t *
entry_pf (uctxt_t * uctxt, vaddr_t va, hal_pfinfo_t pfi)
{
  mcn_vmprot_t req;

  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;

  printf ("cur_thread(): %p\n", cur_thread ());
  info ("CPU #%d Pagefault at %08lx (%x)", cpu_id (), va, pfi);
  uctxt_print (uctxt);

  req = MCN_VMPROT_READ;
  req |= pfi & HAL_PF_INFO_WRITE ? MCN_VMPROT_WRITE : 0;
  req |= pfi & HAL_PF_INFO_EXE ? MCN_VMPROT_EXECUTE : 0;

  if (!vmmap_fault (&cur_task ()->vmmap, va, req))
    {
      mcn_return_t rc;
      printf ("Sending simple\n");
      struct portref pr;
      struct ipcspace *ps;
      ps = task_getipcspace (cur_task ());
      ipcspace_debug (ps);
      rc = ipcspace_resolve (ps, MCN_MSGTYPE_COPYSEND, 3, &pr);
      task_putipcspace (cur_task (), ps);
      if (!rc)
	{
	  printf ("user simple\n");
	  user_simple (portref_to_ipcport (&pr));
	  printf ("destroying cur thread\n");
	}
      sched_destroy (cur_thread ());
    }
  return kern_return ();
}

uctxt_t *
entry_irq (uctxt_t * uctxt, unsigned irq, bool lvl)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;

  info ("IRQ %d", irq);
  return uctxt;
}
