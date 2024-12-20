/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <stddef.h>
#include <nux/syscalls.h>
#include <machina/types.h>
#include <machina/error.h>
#include <machina/syscall_sw.h>
#include <machina/syscalls.h>

#include <stdio.h>

__thread void *__local_msgbuf = NULL;

void *
syscall_msgbuf (void)
{
  if (__local_msgbuf == NULL)
    {
      __local_msgbuf = (void *) syscall0 (__syscall_msgbuf);
    }
  return __local_msgbuf;
}

mcn_return_t
syscall_msgsend (mcn_msgopt_t option, unsigned long timeout,
		 mcn_portid_t notify)
{
  return syscall3 (__syscall_msgsend, option, timeout, notify);
}

mcn_return_t
syscall_msgrecv (mcn_portid_t port, mcn_msgopt_t option,
		 unsigned long timeout, mcn_portid_t notify)
{
  return syscall4 (__syscall_msgrecv, port, option, timeout, notify);
}

mcn_return_t
syscall_reply_port (void)
{
  return syscall0 (__syscall_reply_port);
}

mcn_portid_t
syscall_task_self (void)
{
  mcn_return_t r;

  r = syscall0 (__syscall_task_self);
  return r;
}

