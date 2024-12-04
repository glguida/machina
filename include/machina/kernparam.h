/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_KERNPARAM_H_
#define _MACHINA_KERNPARAM_H_

/*
  MACHINA_MAX_THREADS: Maximum threads per task.
*/

#if __i386__

#define MACHINA_MAX_THREADS 1024
#define MACHINA_MAX_PORTS 128

#elif __amd64__

#define MACHINA_MAX_THREADS (64*1024)
#define MACHINA_MAX_PORTS 1024

#elif defined(__riscv) && __riscv_xlen == 64

#define MACHINA_MAX_THREADS (64*1024)
#define MACHINA_MAX_PORTS 1024

#endif

#define MSGBUF_PAGE_SHIFT 0
#define MSGBUF_PAGES (1 << MSGBUF_PAGE_SHIFT)
#define MSGBUF_SHIFT (MSGBUF_PAGE_SHIFT + PAGE_SHIFT)
#define MSGBUF_SIZE (1L << MSGBUF_SHIFT)

#define MACHINA_MSG_MAXSIZE MSGBUF_SIZE

#endif
