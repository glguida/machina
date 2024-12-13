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
  bool ret;

  spinlock(&map->lock);
  ret = msgbuf_alloc (&map->umap, &map->msgbuf_zone, msgbuf);
  spinunlock(&map->lock);
  return ret;
}

bool
vmmap_alloctls (struct vmmap *map, uaddr_t * tls)
{
  enum tlsvariant
  {
    TLS_VARIANT_I = 1,
    TLS_VARIANT_II = 2,
  } tlsv;

#if MCN_MACHINE_RISCV64
  tlsv = TLS_VARIANT_I;
#endif
#if MCN_MACHINE_AMD64
  tlsv = TLS_VARIANT_II;
#endif
#if MCN_MACHINE_I386
  tlsv = TLS_VARIANT_II;
#endif

  spinlock(&map->lock);
  /*
     XXX: TLS SUPPORT.

     This will have to be a memory region allocated in this VM-map.

     For now, alloc a message buffer. :-(
   */
  struct msgbuf tlsmb;
  assert (msgbuf_alloc (&map->umap, &map->msgbuf_zone, &tlsmb));
  switch (tlsv)
    {
    case TLS_VARIANT_I:
      *(unsigned long *) tlsmb.kaddr = tlsmb.uaddr;
      *tls = (long) tlsmb.uaddr;
      break;
    default:
    case TLS_VARIANT_II:
      *(unsigned long *) (tlsmb.kaddr + MSGBUF_SIZE - sizeof (long)) =
	tlsmb.uaddr + MSGBUF_SIZE - sizeof (long);
      *tls = (long) tlsmb.uaddr + MSGBUF_SIZE - sizeof (long);
      break;
    }
  spinunlock(&map->lock);
  printf ("tls [%s] is %lx\n",
	  tlsv == TLS_VARIANT_I ? "variant I" : "variant II",
	  (long) tlsmb.uaddr + MSGBUF_SIZE - sizeof (long));

  return true;
}

void
vmmap_enter (struct vmmap *map)
{
  spinlock(&map->lock);
  cpu_umap_enter (&map->umap);
  spinunlock(&map->lock);
}

void
vmmap_bootstrap (struct vmmap *map)
{

  vmmap_setupregions (map);
  umap_bootstrap (&map->umap);
  msgbuf_new (&map->msgbuf_zone, VM_MAP_MSGBUF_START, VM_MAP_MSGBUF_END);
}

void
vmmap_setup (struct vmmap *map)
{

  umap_init (&map->umap);
  msgbuf_new (&map->msgbuf_zone, VM_MAP_MSGBUF_START, VM_MAP_MSGBUF_END);
}
