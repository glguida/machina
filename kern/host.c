/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

struct host host;

void
host_init (void)
{
  port_alloc_kernel (&host, KOT_HOST_NAME, &host.name);
  port_alloc_kernel (&host, KOT_HOST_CTRL, &host.ctrl);
}

struct portref
host_getctrlport(struct host *host)
{
  return portref_dup(&host.ctrl);
}

struct portref
host_getnameport(struct host *host)
{
  return portref_dup(&host.name);
}
