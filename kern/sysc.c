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

uctxt_t *
entry_sysc (uctxt_t * u,
	    unsigned long a1, unsigned long a2, unsigned long a3,
	    unsigned long a4, unsigned long a5, unsigned long a6,
	    unsigned long a7)
{
  switch (a1)
    {
    case __syscall_msgbuf:
      uctxt_setret(u, cur_umsgbuf());
      break;
    case __syscall_msgio:
      uctxt_setret(u, ipc_msgio((mcn_msgopt_t)a2, (mcn_portid_t)a3, a4, (mcn_portid_t)a5));
      break;
    case 0:
      info("SYSC%ld test passed.", a1);
      break;
    case 1:
      assert(a2 == 1);
      info("SYSC%ld test passed.", a1);
      break;
    case 2:
      assert(a2 == 1);
      assert(a3 == 2);
      info("SYSC%ld test passed.", a1);
      break;
    case 3:
      assert(a2 == 1);
      assert(a3 == 2);
      assert(a4 == 3);
      info("SYSC%ld test passed.", a1);
      break;
    case 4:
      assert(a2 == 1);
      assert(a3 == 2);
      assert(a4 == 3);
      assert(a5 == 4);
      info("SYSC%ld test passed.", a1);
      break;
    case 5:
      assert(a2 == 1);
      assert(a3 == 2);
      assert(a4 == 3);
      assert(a5 == 4);
      assert(a6 == 5);
      info("SYSC%ld test passed.", a1);
      break;
    case 6:
      assert(a2 == 1);
      assert(a3 == 2);
      assert(a4 == 3);
      assert(a5 == 4);
      assert(a6 == 5);
      assert(a7 == 6);
      info("SYSC%ld test passed.", a1);
      break;
    case 4096:
      putchar (a2);
      break;

    default:
      info ("Received unknown syscall %ld %ld %ld %ld %ld %ld %ld\n",
	    a1, a2, a3, a4, a5, a6, a7);
      break;
    }
  return u;
}
