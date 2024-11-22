/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <nux/slab.h>
#include <machina/error.h>
#include <machina/message.h>

#include <ks.h>

mcn_return_t
simple (mcn_portid_t test)
{
  info ("SIMPLE MESSAGE RECEIVED (test: %ld)\n", test);
  return KERN_SUCCESS;
}

static int counter = 0;
mcn_return_t
inc (mcn_portid_t test, long *new)
{
  *new = ++counter;
  return KERN_SUCCESS;
}

mcn_return_t
add (mcn_portid_t test, long b, long *c)
{
  counter += b;
  *c = counter;
  return KERN_SUCCESS;
}

mcn_return_t
mul (mcn_portid_t test, int b, long *c)
{
  counter *= b;
  *c = counter;
  return KERN_SUCCESS;
}

mcn_return_t
create_thread (mcn_portid_t test, long pc, long sp)
{
  struct thread *th;

  printf ("\n\tcreate thread called (%lx %lx)\n", pc, sp);

  th = thread_new (cur_task (), pc, sp);
  if (th == NULL)
    return KERN_RESOURCE_SHORTAGE;

  uctxt_print (cur_thread ()->uctxt);
  sched_add (th);
  uctxt_print (cur_thread ()->uctxt);

  return KERN_SUCCESS;
}

struct task *
convert_port_to_taskptr (ipc_port_t port)
{
  struct portref task_pr = ipcport_to_portref (&port);
  return port_getkobj (portref_unsafe_get (&task_pr), KOT_TASK);
}

void
ipc_kern_exec (void)
{
  mcn_msgheader_t *msgh;

  while (msgq_deq (&cur_cpu ()->kernel_msgq, &msgh))
    {
      kstest_replies_t kr;

      info ("KERNEL SERVER INPUT:");
      //      message_debug(msgh);
      kstest_server (msgh, (mcn_msgheader_t *) & kr);

      mcn_msgsize_t size = ((mcn_msgheader_t *) & kr)->msgh_size;
      assert (size <= sizeof (kr));
      mcn_msgheader_t *reply = (mcn_msgheader_t *) kmem_alloc (0, size);
      assert (reply != NULL);
      memcpy (reply, &kr, size);

      info ("KERNEL SERVER OUTPUT");
      //      message_debug(reply);
      port_enqueue (reply, 0, true);
    }
}
