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
  info ("SIMPLE MESSAGE RECEIVED (test: %ld)\n", (long) test);
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

  th = thread_new (cur_task (), pc, sp, uctxt_getgp (cur_thread ()->uctxt));
  printf ("thread is %p\n", th);
  if (th == NULL)
    return KERN_RESOURCE_SHORTAGE;

  uctxt_print (cur_thread ()->uctxt);
  sched_add (th);
  uctxt_print (cur_thread ()->uctxt);

  return KERN_SUCCESS;
}

mcn_return_t
create_thread2 (struct taskref tr, long pc, long sp)
{
  struct thread *th;

  if (taskref_isnull (&tr))
    {
      printf ("CREATE_THREAD2: TASK REF IS NULL\n");
      return KERN_INVALID_ARGUMENT;
    }

  struct task *t = taskref_unsafe_get (&tr);
  printf ("task = %p (cur_task = %p)\n", t, cur_task ());

  th = thread_new (t, pc, sp, uctxt_getgp (cur_thread ()->uctxt));
  printf ("thread is %p\n", th);
  if (th == NULL)
    return KERN_RESOURCE_SHORTAGE;

  uctxt_print (cur_thread ()->uctxt);
  sched_add (th);
  uctxt_print (cur_thread ()->uctxt);

  return KERN_SUCCESS;
}

#if 0
struct taskref
convert_port_to_task (ipc_port_t port)
{
  struct task *t = port_getkobj (ipcport_unsafe_get (port), KOT_TASK);
  return taskref_fromraw (t);
}
#endif

void
ipc_kern_exec (void)
{
  mcn_return_t rc;
  mcn_msgheader_t *msgh;

  while (msgq_deq (&cur_cpu ()->kernel_msgq, &msgh))
    {
      kstest_replies_t kr;

      info ("KERNEL SERVER INPUT:");
      message_debug (msgh);
      /*
         MIG-generated code will make a copy of the remote port when
         generating a reply. Create here a reference that will be
         contained in the reply message here, manually.
       */
      if (!ipcport_isnull (msgh->msgh_remote))
	ipcport_forceref (msgh->msgh_remote);

      kstest_server (msgh, (mcn_msgheader_t *) & kr);
      /* Done with the request. Consume and free it. */
      ipc_intmsg_consume (msgh);
      message_debug (msgh);
      kmem_free (0, (vaddr_t) msgh, msgh->msgh_size);

      mcn_msgsize_t size = ((mcn_msgheader_t *) & kr)->msgh_size;
      assert (size <= sizeof (kr));
      mcn_msgheader_t *reply = (mcn_msgheader_t *) kmem_alloc (0, size);
      assert (reply != NULL);
      memcpy (reply, &kr, size);

      info ("KERNEL SERVER OUTPUT");
      message_debug (reply);
      rc = port_enqueue (reply, 0, true);
      printf ("KERNEL SERVER ENQUEUE: %d\n", rc);
      if (rc)
	{
	  ipc_intmsg_consume (reply);
	  kmem_free (0, (vaddr_t) reply, size);
	}
    }
}
