/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_VM_PARAM_H_
#define _MACHINA_VM_PARAM_H_

#include <nux/defs.h>
#include <machina/kernparam.h>

#if __i386__

#define VM_ADDR_MAX 0xbffff000	/* Maximum User Address. */

#elif __amd64__

/* We use 42 bit. See NUX. */
#define VM_ADDR_MAX 0x3fffffff000L

#elif defined(__riscv) && __riscv_xlen == 64

/* We use 42 bit. See NUX. */
#define VM_ADDR_MAX 0x3fffffff000L

#endif

#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_SIZE_FIXED

#define VM_MAP_MSGBUF_END VM_ADDR_MAX
#define VM_MAP_MSGBUF_START (VM_ADDR_MAX - MACHINA_MAX_THREADS * PAGE_SIZE)

#define VM_MAP_PORTS_PAGES 2
#define VM_MAP_PORTS_SIZE (VM_MAP_PORTS_PAGES * PAGE_SIZE)
#define VM_MAP_PORTS_END VM_MAP_MSGBUF_START - PAGE_SIZE
#define VM_MAP_PORTS_START (VM_MAP_PORTS_END - MACHINA_MAX_PORTS * VM_MAP_PORTS_SIZE)

#endif
