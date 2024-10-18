/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

void
vmmap_enter(struct vmmap *map)
{

  cpu_umap_enter (&map->umap);
}


void
vmmap_bootstrap(struct vmmap *map)
{

  umap_bootstrap (&map->umap);
}

void
vmmap_setup(struct vmmap *map)
{

  umap_init(&map->umap);
}
