/*
  This is compiled in kernel.
*/

#include "internal.h"
#include "ks_server.h"
#include <machina/error.h>

mcn_return_t
simple (mcn_portid_t test)
{
  printf ("SIMPLE MESSAGE RECEIVED (test: %ld)\n", (long) test);
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

