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
static void *__sys_msgbuf = NULL;

void __attribute__((constructor (0))) __machina_sysinit (void)
{
  __sys_msgbuf = (void *)syscall0(__syscall_msgbuf);
  printf("constructor! %p\n", __sys_msgbuf);
}

void *
syscall_msgbuf(void)
{
  return __sys_msgbuf;
}

mcn_return_t
syscall_msgio(mcn_msgopt_t option, mcn_portid_t recv, unsigned long timeout, mcn_portid_t notify)
{
  return syscall4(__syscall_msgio, option, recv, timeout, notify);
}

mcn_return_t
syscall_mach_port_allocate(mcn_portid_t task, mcn_portright_t right, mcn_portid_t *name)
{
  mcn_return_t r;

  r = syscall2(__syscall_mach_port_allocate, task, right);
  if (r == KERN_SUCCESS)
    *name = *(mcn_portid_t *)__sys_msgbuf;
  return r;
}
