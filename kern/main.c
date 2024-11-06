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

cpumask_t idlemap;

void
cpu_kick (void)
{
  cpu_ipi_mask (idlemap);
}

__thread char test_2[40];

struct thread *bootstrap;
static int smp_sync = 0;

mcn_return_t testport_send(void *ctx, mcn_msgid_t id, void *data, size_t size, struct portref reply)
{
  printf("Received message: context %p id %d, data is at %p with size %ld\n",
	 ctx, id, data, size);

  return 0;
}

int
main (int argc, char *argv[])
{
  mcn_return_t rc;
  struct portref portref;

  info ("MACHINA Started.");
  physmem_init();
  msgbuf_init();
  task_init();
  thread_init();
  port_init();
  ipcspace_init();

  /* Initialise per-CPU data. */
  cpu_setdata((void *)kmem_alloc(0, sizeof(struct mcncpu)));
  TAILQ_INIT(&cur_cpu()->kernel_queue.msgq);
  cur_cpu()->idle = thread_idle();
  cur_cpu()->thread = cur_cpu()->idle;
  atomic_cpumask_set (&idlemap, cpu_id());

  struct task *t = task_bootstrap();
  struct thread *th = thread_bootstrap(t);
  hal_umap_load(NULL);
  sched_add(th);
  printf("here");

  rc = port_alloc_kernel(NULL, &portref);
  assert(rc == KERN_SUCCESS);
  struct portright testpr = portright_from_portref(RIGHT_SEND, portref);
  mcn_portid_t id;
  rc = task_addportright(t, &testpr, &id);
  printf("Test id is %ld\n", id);
  assert(rc == KERN_SUCCESS);


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
  cpu_setdata((void *)kmem_alloc(0, sizeof(struct mcncpu)));
  TAILQ_INIT(&cur_cpu()->kernel_queue.msgq);
  cur_cpu()->idle = thread_idle();
  cur_cpu()->thread = cur_cpu()->idle;
  atomic_cpumask_set (&idlemap, cpu_id());

  return EXIT_IDLE;
}

uctxt_t *
kern_return (void)
{
  ipc_kern_exec();

  return sched_next ();
}

uctxt_t *
entry_ipi (uctxt_t * uctxt)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;
  
  info ("IPI!");
  return kern_return();
}

uctxt_t *
entry_alarm (uctxt_t * uctxt)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;

  timer_alarm (1 * 1000 * 1000 * 1000);
  info ("TMR: %" PRIu64 " us", timer_gettime ());
  uctxt_print (uctxt);
  return kern_return();
}

uctxt_t *
entry_ex (uctxt_t * uctxt, unsigned ex)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;
  
  info ("Exception %d", ex);
  uctxt_print (uctxt);
  cur_thread()->status = SCHED_STOPPED;
  return kern_return();
}

uctxt_t *
entry_pf (uctxt_t * uctxt, vaddr_t va, hal_pfinfo_t pfi)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;

  printf("cur_thread(): %p\n", cur_thread());
  info ("CPU #%d Pagefault at %08lx (%d)", cpu_id (), va, pfi);
  uctxt_print (uctxt);
  cur_thread()->status = SCHED_STOPPED;
  return kern_return();
}

uctxt_t *
entry_irq (uctxt_t * uctxt, unsigned irq, bool lvl)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;
  
  info ("IRQ %d", irq);
  return uctxt;
}
