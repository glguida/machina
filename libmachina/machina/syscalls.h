/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_SYSCALLS_H_
#define _MACHINA_SYSCALLS_H_

#include <machina/types.h>

extern void *__local_msgbuf; /* GIANLUCA: MAKE THIS PER THREAD */

void *syscall_msgbuf(void);

mcn_return_t syscall_msgio(mcn_msgopt_t option, mcn_portid_t recv, unsigned long timeout, mcn_portid_t notify);

mcn_return_t syscall_mach_port_allocate(mcn_portid_t task, mcn_portright_t right, mcn_portid_t *name);


#endif
