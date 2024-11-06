/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <assert.h>
#include <nux/nux.h>
#include <nux/hal.h>
#include <machina/syscall_sw.h>

#include "internal.h"

void
ipc_kern_exec(void);


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
      ret = cur_umsgbuf();
      break;
    case __syscall_msgrecv:
      ret = ipc_msgrecv((mcn_portid_t)a2, (mcn_msgopt_t)a3, a4, (mcn_portid_t)a5);
      break;
    case __syscall_msgsend:
      ret = ipc_msgsend((mcn_msgopt_t)a2, a3, (mcn_portid_t)a4);
      break;
    case __syscall_reply_port:
      {
	mcn_return_t rc;
	mcn_portid_t id;

	rc = task_allocate_port(cur_task(), &id);
	printf("Allocated port %d [%ld]\n", rc, id);
	if (rc)
	  ret = MCN_PORTID_NULL;
	else
	  ret = id;
      }
      break;
    case 0:
      info("SYSC%ld test passed.", a1);
      break;
      ret = 0;
    case 1:
      assert(a2 == 1);
      info("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 2:
      assert(a2 == 1);
      assert(a3 == 2);
      info("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 3:
      assert(a2 == 1);
      assert(a3 == 2);
      assert(a4 == 3);
      info("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 4:
      assert(a2 == 1);
      assert(a3 == 2);
      assert(a4 == 3);
      assert(a5 == 4);
      info("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 5:
      assert(a2 == 1);
      assert(a3 == 2);
      assert(a4 == 3);
      assert(a5 == 4);
      assert(a6 == 5);
      info("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 6:
      assert(a2 == 1);
      assert(a3 == 2);
      assert(a4 == 3);
      assert(a5 == 4);
      assert(a6 == 5);
      assert(a7 == 6);
      info("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 4096:
      putchar (a2);
      ret = 0;
      break;
      
    default:
      info ("Received unknown syscall %ld %ld %ld %ld %ld %ld %ld\n",
	    a1, a2, a3, a4, a5, a6, a7);
      ret = -1;
      break;
    }

  uctxt_setret(cur_thread()->uctxt, ret);
  return kern_return();
}
