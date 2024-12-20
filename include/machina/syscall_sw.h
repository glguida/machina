/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <stdbool.h>
#include <machina/types.h>

#define __syscall_msgbuf -1L

#define __syscall_msgsend -20L
#define __syscall_msgrecv -21L
#define __syscall_reply_port -26L
#define __syscall_task_self -27L


