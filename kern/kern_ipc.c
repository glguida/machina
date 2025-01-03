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

extern uintptr_t _data_ext0_start[];
extern uintptr_t _data_ext0_end[];

void
ipc_kern_exec (void)
{
  mcn_return_t rc;
  mcn_msgheader_t *msgh;

  while (msgq_deq (&cur_cpu ()->kernel_msgq, &msgh))
    {
      char buf [MSGBUF_SIZE];
      bool (**demux)(mcn_msgheader_t *, mcn_msgheader_t *);

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

      /*
	Iterate through all the server demux linked to the kernel,
	until we find one that handles the message.
	Otherwhise we return the last unhandled demux message, which
	will contain an error.
      */
      for (demux = (void *)_data_ext0_start; (void *)demux < (void *)_data_ext0_end; demux++)
	if ((*demux)(msgh, (mcn_msgheader_t *)buf))
	  break;

      /* Done with the request. Consume and free it. */
      ipc_intmsg_consume (msgh);

#ifdef KIPC_DEBUG
      message_debug (msgh);
#endif

      kmem_free (0, (vaddr_t) msgh, msgh->msgh_size);

      mcn_msgsize_t size = ((mcn_msgheader_t *) buf)->msgh_size;
      assert (size <= sizeof(buf));
      mcn_msgheader_t *reply = (mcn_msgheader_t *) kmem_alloc (0, size);
      assert (reply != NULL);
      memcpy (reply, buf, size);

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
