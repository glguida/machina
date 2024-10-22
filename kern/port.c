/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <nux/slab.h>

struct slab ports;

struct portref
port_alloc_kernel(fn_msgsend_t send, void *ctx)
{
  struct portref portref;
  struct port *p;

  p = slab_alloc(&ports);
  p->type = PORT_KERNEL;
  p->kernel.msgsend = send;
  p->kernel.ctx = ctx;

  portref.obj = p;
  p->_ref_count = 1;

  return portref;
}

void
port_init(void)
{
  slab_register(&ports, "PORTS", sizeof(struct port), NULL, 0);

}
