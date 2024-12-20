/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_SYSCALLS_H_
#define _MACHINA_SYSCALLS_H_

#include <machina/types.h>

extern __thread void *__local_msgbuf;

void *syscall_msgbuf (void);

mcn_return_t syscall_msgsend (mcn_msgopt_t option, unsigned long timeout,
			      mcn_portid_t notify);
mcn_return_t syscall_msgrecv (mcn_portid_t recv, mcn_msgopt_t option,
			      unsigned long timeout, mcn_portid_t notify);
mcn_return_t syscall_reply_port (void);

mcn_portid_t syscall_task_self (void);

#endif
