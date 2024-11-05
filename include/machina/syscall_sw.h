/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#define __syscall_msgbuf -1L

#define __syscall_msgsend -20
#define __syscall_msgrecv -21
#define __syscall_reply_port -26L

#define __syscall_mach_port_allocate -72L
