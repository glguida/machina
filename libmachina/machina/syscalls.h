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
mcn_return_t syscall_port_allocate (mcn_portid_t task, mcn_portright_t right,
				    mcn_portid_t * name);
mcn_return_t syscall_vm_allocate (mcn_portid_t task, mcn_vmaddr_t * addr,
				  unsigned long size, int anywhere);

mcn_return_t syscall_vm_region (mcn_portid_t task, mcn_vmaddr_t * addr,
				unsigned long *size, mcn_vmprot_t * curprot,
				mcn_vmprot_t * maxprot,
				mcn_vminherit_t * inherit, unsigned *shared,
				mcn_portid_t * nameid, mcn_vmoff_t * off);

mcn_return_t
syscall_vm_map (mcn_portid_t task, mcn_vmaddr_t * addr,
		unsigned long size, mcn_vmaddr_t mask,
		unsigned anywhere, mcn_portid_t objname,
		mcn_vmoff_t off, unsigned copy,
		mcn_vmprot_t curprot, mcn_vmprot_t maxprot,
		mcn_vminherit_t inherit);

#endif
