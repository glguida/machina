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

bool
vmmap_alloctls (struct vmmap *map, uaddr_t *tls)
{
  /*
    XXX: TLS SUPPORT.

    This will have to be a memory region allocated in this VM-map.

    For now, alloc a message buffer. :-(
  */
  struct msgbuf tlsmb;
  assert(msgbuf_alloc(&map->umap, &map->msgbuf_zone, &tlsmb));
  *(unsigned long *)(tlsmb.kaddr + MSGBUF_SIZE - sizeof(long)) = tlsmb.uaddr + MSGBUF_SIZE - sizeof(long);
  printf("tls is %lx\n", (long)tlsmb.uaddr + MSGBUF_SIZE - sizeof(long));
  *tls = (long)tlsmb.uaddr + MSGBUF_SIZE - sizeof(long);
  return true;
}

void
vmmap_enter(struct vmmap *map)
{

  cpu_umap_enter (&map->umap);
}


void
vmmap_bootstrap(struct vmmap *map)
{

  vmreg_setup (map);
  umap_bootstrap (&map->umap);
  msgbuf_new(&map->msgbuf_zone, VM_MAP_MSGBUF_START, VM_MAP_MSGBUF_END);
}

void
vmmap_setup(struct vmmap *map)
{

  umap_init(&map->umap);
  msgbuf_new(&map->msgbuf_zone, VM_MAP_MSGBUF_START, VM_MAP_MSGBUF_END);
}
