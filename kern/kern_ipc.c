/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <nux/slab.h>
#include <machina/error.h>
#include <machina/message.h>

#ifndef KIPC_DEBUG
#define KIPC_PRINT(...)
#else
#define KIPC_PRINT printf
#endif

#include <ks.h>

mcn_return_t
simple (mcn_portid_t test)
{
  KIPC_PRINT ("SIMPLE MESSAGE RECEIVED (test: %ld)\n", (long) test);
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
  mcn_return_t rc;
  struct threadref ref;

  printf ("\n\tcreate thread called (%lx %lx)\n", pc, sp);

  rc = task_create_thread (cur_task (), &ref);
  printf ("thread is %p\n", threadref_unsafe_get(&ref));
  if (rc)
    return rc;

  threadref_consume(&ref);
  return KERN_SUCCESS;
}

mcn_return_t
create_thread2 (struct taskref tr, long pc, long sp)
{
  mcn_return_t rc;
  struct threadref threadref;

  if (taskref_isnull (&tr))
    {
      printf ("CREATE_THREAD2: TASK REF IS NULL\n");
      return KERN_INVALID_ARGUMENT;
    }

  struct task *t = taskref_unsafe_get (&tr);
  printf ("task = %p (cur_task = %p)\n", t, cur_task ());

  rc = task_create_thread (t, &threadref);
  printf ("thread is %p\n", threadref_unsafe_get(&threadref));
  if (rc)
    return rc;

  /* HACK. */
  uctxt_setip (threadref_unsafe_get(&threadref)->uctxt, pc);
  uctxt_setsp (threadref_unsafe_get(&threadref)->uctxt, sp);

  thread_resume(threadref_unsafe_get(&threadref));

  threadref_consume(&threadref);
  return KERN_SUCCESS;
}

void
ipc_kern_exec (void)
{
  mcn_return_t rc;
  mcn_msgheader_t *msgh;

  while (msgq_deq (&cur_cpu ()->kernel_msgq, &msgh))
    {
      kstest_replies_t kr;

#ifdef KIPC_DEBUG
      KIPC_PRINT ("KERNEL SERVER INPUT:\n");
      message_debug (msgh);
#endif
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

#ifdef KIPC_DEBUG
      message_debug (msgh);
#endif

      kmem_free (0, (vaddr_t) msgh, msgh->msgh_size);

      mcn_msgsize_t size = ((mcn_msgheader_t *) & kr)->msgh_size;
      assert (size <= sizeof (kr));
      mcn_msgheader_t *reply = (mcn_msgheader_t *) kmem_alloc (0, size);
      assert (reply != NULL);
      memcpy (reply, &kr, size);

#ifdef KIPC_DEBUG
      KIPC_PRINT ("KERNEL SERVER OUTPUT");
      message_debug (reply);
#endif
      rc = port_enqueue (reply, 0, true);
      KIPC_PRINT ("KERNEL SERVER ENQUEUE: %d\n", rc);
      if (rc)
	{
	  ipc_intmsg_consume (reply);
	  kmem_free (0, (vaddr_t) reply, size);
	}
    }
}
