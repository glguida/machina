/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <machina/types.h>
#include <machina/message.h>
#include <machina/syscalls.h>

mcn_msgioret_t
mcn_msgio(mcn_msgopt_t option, mcn_portid_t recv, unsigned long timeout, mcn_portid_t notify)
{
  return (mcn_msgioret_t)syscall_msgio(option, recv, timeout, notify);
}

mcn_portid_t
mcn_reply_port(void)
{
  return (mcn_portid_t)syscall_reply_port();
}
