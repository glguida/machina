/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

bool
share_kva (vaddr_t va, size_t size, struct umap *umap, uaddr_t uaddr,
	   bool uwr)
{
  unsigned flags = (uwr ? HAL_PTE_W : 0) | HAL_PTE_P | HAL_PTE_U;
  pfn_t pfn;
  unsigned pages;

  va = trunc_page (va);
  uaddr = trunc_page (uaddr);
  pages = (round_page (va + size) - va) >> PAGE_SHIFT;

  for (unsigned i = 0; i < pages; i++)
    {
      pfn = kmap_getpfn (va + i * PAGE_SIZE);
      if (pfn == PFN_INVALID)
	continue;

      if (!umap_map (umap, uaddr + i * PAGE_SIZE, pfn, flags, NULL))
	{
	  for (unsigned j = 0; j < i; j++)
	    (void) umap_unmap (umap, uaddr + j * PAGE_SIZE);
	  umap_commit (umap);
	  return false;
	}
    }
  umap_commit (umap);
  return true;
}

void
unshare_kva (struct umap *umap, uaddr_t uaddr, size_t size)
{
  unsigned pages;

  uaddr = trunc_page (uaddr);
  pages = (round_page (uaddr + size) - uaddr) >> PAGE_SHIFT;

  for (unsigned i = 0; i < pages; i++)
    (void) umap_unmap (umap, uaddr + i * PAGE_SIZE);
  umap_commit (umap);
}
