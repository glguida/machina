/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <assert.h>
#include <nux/nux.h>
#include <nux/hal.h>
#include <machina/syscall_sw.h>
#include <machina/error.h>

#include "internal.h"

extern uintptr_t _data_ext1_start[];
extern uintptr_t _data_ext1_end[];

uctxt_t *
entry_sysc (uctxt_t * u,
	    unsigned long a1, unsigned long a2, unsigned long a3,
	    unsigned long a4, unsigned long a5, unsigned long a6,
	    unsigned long a7)
{
  long ret = 0;

  if (cur_thread ()->uctxt != UCTXT_IDLE)
    *cur_thread ()->uctxt = *u;


  switch (a1)
    {
    case __syscall_msgbuf:
      nuxperf_inc (&pmachina_sysc_msgbuf);
      ret = cur_umsgbuf ();
      break;
    case __syscall_msgrecv:
      nuxperf_inc (&pmachina_sysc_msgrecv);
      ret =
	ipc_msgrecv ((mcn_portid_t) a2, (mcn_msgopt_t) a3, a4,
		     (mcn_portid_t) a5);
      break;
    case __syscall_msgsend:
      nuxperf_inc (&pmachina_sysc_msgsend);
      ret = ipc_msgsend ((mcn_msgopt_t) a2, a3, (mcn_portid_t) a4);
      break;
    case __syscall_reply_port:
      {
	mcn_return_t rc;
	mcn_portid_t id;

	nuxperf_inc (&pmachina_sysc_reply_port);
	rc = task_allocate_port (cur_task (), &id);
	if (rc)
	  ret = MCN_PORTID_NULL;
	else
	  ret = id;
      }
      break;
    case __syscall_task_self:
      nuxperf_inc (&pmachina_sysc_task_self);
      ret = task_self ();
      break;

    default:
      {
	bool found;
	bool (**demux) (uctxt_t * u,
		       unsigned long a1, unsigned long a2, unsigned long a3,
		       unsigned long a4, unsigned long a5, unsigned long a6,
		       unsigned long a7, long *ret);

	/*
	  Iterate through all the syscall demux linked to the kernel,
	  until we find one that handles the message.
	  Return an error otherwhise.
	*/
	for (demux = (void *)_data_ext1_start;
	     (void *)demux < (void *)_data_ext1_end;
	     demux++)
	  if ((*demux)(u, a1, a2, a3, a4, a5, a6, a7, &ret))
	    {
	      found = true;
	      break;
	    }

	if (!found)
	  {
	    info ("Received unknown syscall %ld %ld %ld %ld %ld %ld %ld\n",
		  a1, a2, a3, a4, a5, a6, a7);
	    ret = -1;
	  }
      }
      break;
    }

  uctxt_setret (cur_thread ()->uctxt, ret);
  return kern_return ();
}
