/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <stdio.h>
#include <nux/nux.h>
#include <nux/hal.h>
#include <nux/nuxperf.h>
#include <machina/error.h>

#include "internal.h"
#include "vm.h"

#include <ku.h>

cpumask_t idlemap;

void
cpu_kick (void)
{
  nuxperf_inc (&pmachina_cpu_kick);
  once_cpumask (idlemap, cpu_ipi(i));
}

struct taskref bootstrap_taskref;
static int smp_sync = 0;

static struct timer timer;

static void
_print_perfctr (void *unused, nuxperf_t *ctr)
{
  printf ("ctr: %-20s\t%20ld\n", ctr->name, ctr->val);
}

static void _timer_handler (void *opq)
{
  struct task *t = taskref_unsafe_get (&bootstrap_taskref);

  mcn_return_t rc;
  struct portref pr;
  struct ipcspace *ps;
  ps = task_getipcspace (t);
  rc = ipcspace_resolve (ps, MCN_MSGTYPE_COPYSEND, 3, &pr);
  task_putipcspace (t, ps);
  if (!rc)
    user_simple (portref_to_ipcport (&pr));

  nuxperf_foreach (_print_perfctr, NULL);
  nuxperf_reset();
  nuxmeasure_print ();
  nuxmeasure_reset();
}

void
start_timer (void)
{
  timer.valid = 1;
  timer.opq = NULL;
  timer.handler = _timer_handler;
  timer_register (&timer, 1L * 1000 * 1000 * 1000);
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
  TAILQ_INIT (&cur_cpu ()->dead_tasks);
  atomic_cpumask_set (&idlemap, cpu_id ());

  task_bootstrap (&bootstrap_taskref);

  port_alloc_kernel (NULL, KOT_THREAD, &portref);
  struct portref send = portref_dup(&portref);
  struct portright testpr = portright_from_portref (RIGHT_SEND, send);
  mcn_portid_t id;
  rc =
    task_addportright (taskref_unsafe_get (&bootstrap_taskref), &testpr, &id);
  printf ("Test id is %ld\n", id);
  assert (rc == KERN_SUCCESS);

  start_timer();
  
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
  TAILQ_INIT (&cur_cpu ()->dead_tasks);
  atomic_cpumask_set (&idlemap, cpu_id ());

  return EXIT_IDLE;
}

uctxt_t *
kern_return (void)
{
  uctxt_t *uctxt;

  ipc_kern_exec ();

  void memctrl_run_clock ();
  memctrl_run_clock ();

  uctxt = sched_next ();

  {
    struct thread *th, *tmp;
    TAILQ_FOREACH_SAFE(th, &cur_cpu ()->dead_threads, sched_list, tmp)
      {
	TAILQ_REMOVE(&cur_cpu ()->dead_threads, th, sched_list);
	_task_destroy_thread(th);
      }
  }

  {
    struct task *t, *tmp;
    TAILQ_FOREACH_SAFE(t, &cur_cpu ()->dead_tasks, task_list, tmp)
      {
	TAILQ_REMOVE(&cur_cpu ()->dead_tasks, t, task_list);
	_task_cleanup(t);
      }
  }

  return uctxt;
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

  timer_run ();

  if (!timer.valid)
    {
      timer.valid = 1;
      timer.opq = NULL;
      timer.handler = _timer_handler;
      timer_register (&timer, 1L * 1000 * 1000 * 1000);
    }

  return kern_return ();
}

uctxt_t *
entry_ex (uctxt_t * uctxt, unsigned ex)
{
  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *uctxt;

  info ("Exception %d", ex);
  uctxt_print (uctxt);
  thread_destroy (cur_thread ());
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
      thread_destroy (cur_thread ());
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

#undef NUXPERF
#undef NUXPERF_DECLARE
#define NUXPERF_DEFINE
#include "perf.h"
