/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <machina/types.h>
#include <machina/message.h>
#include <machina/syscalls.h>

mcn_msgioret_t
mcn_msgrecv(mcn_portid_t recv, mcn_msgopt_t option, unsigned long timeout, mcn_portid_t notify)
{
  return (mcn_msgioret_t)syscall_msgrecv(recv, option, timeout, notify);
}

mcn_msgioret_t
mcn_msgsend(mcn_msgopt_t option, unsigned long timeout, mcn_portid_t notify)
{
  return (mcn_msgioret_t)syscall_msgsend(option, timeout, notify);
}

mcn_portid_t
mcn_reply_port(void)
{
  return (mcn_portid_t)syscall_reply_port();
}
