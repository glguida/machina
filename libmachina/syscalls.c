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

/* XXX: Make this per-thread. */
void *__local_msgbuf = NULL;

void __attribute__((constructor (0))) __machina_sysinit (void)
{
  __local_msgbuf = (void *)syscall0(__syscall_msgbuf);
  printf("constructor! %p\n", __local_msgbuf);
}

void *
syscall_msgbuf(void)
{
  return __local_msgbuf;
}

mcn_return_t
syscall_msg(mcn_msgopt_t option, mcn_portid_t recv, unsigned long timeout, mcn_portid_t notify)
{
  return syscall4(__syscall_msg, option, recv, timeout, notify);
}

mcn_return_t
syscall_reply_port(void)
{
  return syscall0(__syscall_reply_port);
}

mcn_return_t
syscall_mach_port_allocate(mcn_portid_t task, mcn_portright_t right, mcn_portid_t *name)
{
  mcn_return_t r;

  r = syscall2(__syscall_mach_port_allocate, task, right);
  if (r == KERN_SUCCESS)
    *name = *(mcn_portid_t *)__local_msgbuf;
  return r;
}
