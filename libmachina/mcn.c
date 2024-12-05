/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <machina/types.h>
#include <machina/error.h>
#include <machina/message.h>
#include <machina/syscalls.h>

static mcn_portid_t task_self_ = MCN_PORTID_NULL;

void __attribute__((constructor (0))) __machina_libinit (void)
{
  task_self_ = syscall_task_self ();
}

mcn_msgioret_t
mcn_msgrecv (mcn_portid_t recv, mcn_msgopt_t option, unsigned long timeout,
	     mcn_portid_t notify)
{
  mcn_msgioret_t rc;

  do
    {
      rc = (mcn_msgioret_t) syscall_msgrecv (recv, option, timeout, notify);
    }
  while (rc == KERN_RETRY);

  return rc;
}

mcn_msgioret_t
mcn_msgsend (mcn_msgopt_t option, unsigned long timeout, mcn_portid_t notify)
{
  mcn_msgioret_t rc;

  do
    {
      rc = (mcn_msgioret_t) syscall_msgsend (option, timeout, notify);
    }
  while (rc == KERN_RETRY);

  return rc;
}

mcn_portid_t
mcn_reply_port (void)
{
  return (mcn_portid_t) syscall_reply_port ();
}

mcn_portid_t
mcn_task_self (void)
{
  return task_self_;
}
