/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <assert.h>
#include <stdint.h>
#include <nux/nux.h>

#include "vm.h"

/*
  Indirect Map.

  A pagetable-like structure that mimics inode maps in old UNIX
  filesystems. Allows to keep a single page for indexing small
  objects, and gradually deeper indirect tables for larger objects.
*/

void
imap_init(struct imap *im)
{
  im->l1 = IPTE_EMPTY;
  im->l2 = IPTE_EMPTY;
  im->l3 = IPTE_EMPTY;
}

static inline ipte_t *
_gettable(ipte_t *ipte, const bool alloc)
{
  if (ipte->p)
    return pfn_get(ipte_pfn(ipte));

  if (!alloc)
    return NULL;
  
  pfn_t pfn = pfn_alloc(0);
  assert (pfn != PFN_INVALID);
  ipte->p = 1;
  ipte->pfn = pfn;
  return pfn_get(pfn);
}

static inline void
_puttable(ipte_t ipte, void *ptr)
{
  pfn_put(ipte_pfn(&ipte), ptr);
}

#define ISHIFT (PAGE_SHIFT - 3) /* 3 = LOG2(sizeof(ipte)) */
#define IMASK ((1 << ISHIFT) - 1)
#define L1IDX(off) ((off >> PAGE_SHIFT) & IMASK)
#define L2IDX(off) ((off >> (PAGE_SHIFT + ISHIFT)) & IMASK)
#define L3IDX(off) ((off >> (PAGE_SHIFT + 2 * ISHIFT)) & IMASK)


static inline ipte_t
_get_entry(struct imap *im, unsigned long off, const bool set, ipte_t newval)
{
  unsigned l1idx = L1IDX(off);
  unsigned l2idx = L2IDX(off);
  unsigned l3idx = L3IDX(off);
  ipte_t ret;

  if ((l3idx == 0) && (l2idx == 0))
    {
      ipte_t *l1ptr;
      debug("%s L1: %d-%d-%d", set ? "SET" : "GET", l3idx, l2idx, l1idx);
      l1ptr = _gettable(&im->l1, set);
      if (l1ptr == NULL)
	return IPTE_EMPTY;
      ret = l1ptr[l1idx];
      if (set)
	l1ptr[l1idx] = newval;
      _puttable(im->l1, l1ptr);
    }
  else if (l3idx == 0)
    {
      debug("%s L2: %d-%d-%d", set ? "SET" : "GET", l3idx, l2idx, l1idx);
      ipte_t *l2ptr, *l1ptr;
      l2ptr = _gettable(&im->l2, set);
      if (l2ptr == NULL)
	return IPTE_EMPTY;
      l1ptr = _gettable(l2ptr + l2idx, set);
      if (l1ptr == NULL)
	{
	  _puttable(im->l2, l2ptr);
	  return IPTE_EMPTY;
	}
      ret = l1ptr[l1idx];
      if (set)
	l1ptr[l1idx] = newval;
      _puttable(l2ptr[l2idx], l1ptr);
      _puttable(im->l2, l2ptr);
    }
  else
    {
      debug("%s L3: %d-%d-%d", set ? "SET" : "GET", l3idx, l2idx, l1idx);
      ipte_t *l3ptr, *l2ptr, *l1ptr;
      l3ptr = _gettable(&im->l3, set);
      if (l3ptr == NULL)
	return IPTE_EMPTY;
      l2ptr = _gettable(l3ptr + l3idx, set);
      if (l2ptr == NULL)
	{
	  _puttable(im->l3, l3ptr);
	  return IPTE_EMPTY;
	}
      l1ptr = _gettable(l2ptr + l2idx, set);
      if (l1ptr == NULL)
	{
	  _puttable(l3ptr[l3idx], l2ptr);
	  _puttable(im->l3, l3ptr);
	  return IPTE_EMPTY;
	}
      ret = l1ptr[l1idx];
      if (set)
	l1ptr[l1idx] = newval;
      _puttable(l2ptr[l2idx], l1ptr);
      _puttable(l3ptr[l3idx], l2ptr);
      _puttable(im->l3, l3ptr);
    }
  return ret;
}

/*
  Map the pte at offset off. Returns the old pfn.
*/
ipte_t
imap_map(struct imap *im, unsigned long off, pfn_t pfn, bool roshared, vm_prot_t protmask)
{
  ipte_t new;

  new.p = 1;
  new.roshared = roshared;
  new.protmask = protmask;
  new.pfn = pfn;
  return _get_entry(im, off, true, new);
}


/*
  Get the pte at offset off.
*/
ipte_t
imap_lookup(struct imap *im, unsigned long off)
{
  return _get_entry(im, off, false, IPTE_EMPTY);
}
