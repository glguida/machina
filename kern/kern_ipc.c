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

mcn_return_t simple
(
	mcn_portid_t test
)
{
  info("SIMPLE MESSAGE RECEIVED (test: %ld)\n", test);
  return KERN_SUCCESS;
}

static int counter = 0;
mcn_return_t inc(mcn_portid_t test, long *new)
{
  *new = ++counter;
  return KERN_SUCCESS;
}

mcn_return_t add(mcn_portid_t test, long b, long *c)
{
  counter += b;
  *c = counter;
  return KERN_SUCCESS;
}

mcn_return_t mul(mcn_portid_t test, int b, long *c)
{
  counter *= b;
  *c = counter;
  return KERN_SUCCESS;
}


void
ipc_kern_exec(void)
{
  struct port_queue *queue = &cur_cpu()->kernel_queue;
  if (!TAILQ_EMPTY(&queue->msgq))
    {
      mcn_msgheader_t *reply = (mcn_msgheader_t *)kmem_alloc(0, sizeof(kstest_replies_t));
      if (reply == NULL)
	return;

      struct msgq_entry *msgq = TAILQ_FIRST(&queue->msgq);
      assert(msgq != NULL);
      TAILQ_REMOVE(&queue->msgq, msgq, queue);
      mcn_msgheader_t *msgh = msgq->msgh;
      slab_free(msgq);

      info("KERNEL SERVER INPUT:");
      message_debug(msgh);
      kstest_server(msgh, reply);
      info("KERNEL SERVER OUTPUT");
      message_debug(reply);
      port_enqueue(reply);
    }
}
