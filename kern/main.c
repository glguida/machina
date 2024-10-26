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

__thread char test_2[40];

struct thread *bootstrap;

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
  portspace_init();

  /* Initialise per-CPU data. */
  cpu_setdata((void *)kmem_alloc(0, sizeof(struct mcncpu)));

  struct task *t = task_bootstrap();
  printf("task = %p\n", t);
  struct thread *th = thread_bootstrap(t);
  printf("thread = %p\n", th);
  rc = port_alloc_kernel(testport_send, NULL, &portref);
  assert(rc == KERN_SUCCESS);
  struct portright testpr = portright_from_portref(RIGHT_SEND, portref);
  mcn_portid_t id;
  rc = task_addportright(t, &testpr, &id);
  printf("Test id is %ld\n", id);
  assert(rc == KERN_SUCCESS);

  bootstrap = th;
  cpu_ipi (cpu_id ());

  printf("cpu id %d\n", cpu_id());

  return EXIT_IDLE;
}

int
main_ap (void)
{
  printf ("%d: %" PRIx64 "\n", cpu_id (), timer_gettime ());
  return EXIT_IDLE;
}

uctxt_t *
entry_ipi (uctxt_t * uctxt)
{
  info ("IPI!");
  thread_enter(bootstrap);
  return thread_uctxt(bootstrap);
}

uctxt_t *
entry_alarm (uctxt_t * uctxt)
{
  timer_alarm (1 * 1000 * 1000 * 1000);
  info ("TMR: %" PRIu64 " us", timer_gettime ());
  uctxt_print (uctxt);
  return uctxt;
}

uctxt_t *
entry_ex (uctxt_t * uctxt, unsigned ex)
{
  info ("Exception %d", ex);
  uctxt_print (uctxt);
  return UCTXT_IDLE;
}

uctxt_t *
entry_pf (uctxt_t * uctxt, vaddr_t va, hal_pfinfo_t pfi)
{
  info ("CPU #%d Pagefault at %08lx (%d)", cpu_id (), va, pfi);
  uctxt_print (uctxt);
  return UCTXT_IDLE;
}

uctxt_t *
entry_irq (uctxt_t * uctxt, unsigned irq, bool lvl)
{
  info ("IRQ %d", irq);
  return uctxt;

}
