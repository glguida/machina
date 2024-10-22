/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <machina/vm_param.h>

bool
vmmap_allocmsgbuf (struct vmmap *map, struct msgbuf *msgbuf)
{

  return msgbuf_alloc (&map->umap, &map->msgbuf_zone, msgbuf);
}

void
vmmap_enter(struct vmmap *map)
{

  cpu_umap_enter (&map->umap);
}


void
vmmap_bootstrap(struct vmmap *map)
{

  umap_bootstrap (&map->umap);
  msgbuf_new(&map->msgbuf_zone, VM_MAP_MSGBUF_START, VM_MAP_MSGBUF_END);
}

void
vmmap_setup(struct vmmap *map)
{

  umap_init(&map->umap);
  msgbuf_new(&map->msgbuf_zone, VM_MAP_MSGBUF_START, VM_MAP_MSGBUF_END);
}
