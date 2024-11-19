/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <assert.h>
#include <stdint.h>
#include <nux/nux.h>

/*
  Indirect Map.

  A pagetable-like structure that mimics inode maps in old UNIX
  filesystems. Allows to keep a single page for indexing small
  objects, and gradually deeper indirect tables for larger objects.
*/

/*
  Structure of a ipte:

  - V (valid) bit: implies that the entry is valid: the map has an
    entry in this.

  - PFN: If zero, the map has an entry but it's not available. In the
    case of VM objects, it means the entry has to be searched in the
    pager associated with this object.
    If non-zero, the page number the data is currently stored in.
*/
typedef struct ipte {
  uint64_t pfn : 63;
  uint64_t v : 1;
} ipte_t;

#define IPTE_EMPTY ((ipte_t){ .v = 0, .pfn = 0 })
#define IPTE_NOPFN ((ipte_t){ .v = 1, .pfn = 0 })

struct imap {
  ipte_t l1;
  ipte_t l2;
  ipte_t l3;
};

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
  if (ipte->v)
    return pfn_get(ipte->pfn);

  if (!alloc)
    return NULL;
  
  pfn_t pfn = pfn_alloc(0);
  assert (pfn != PFN_INVALID);
  ipte->v = 1;
  ipte->pfn = pfn;
  return pfn_get(pfn);
}

static inline void
_puttable(ipte_t ipte, void *ptr)
{
  assert(ipte.v);
  pfn_put(ipte.pfn, ptr);
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
imap_map(struct imap *im, unsigned long off, pfn_t pfn)
{
  ipte_t new;

  new.v = 1;
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
	 

void imap_test(void)
{
  struct imap im;
  ipte_t ipte;

  imap_init(&im);

  printf("ISHIFT: %d IMASK: %x\n", ISHIFT, IMASK);
  
  ipte = imap_lookup(&im, 0);
  printf("lookup 0 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_lookup(&im, ((1L << 9) + 3L) << PAGE_SHIFT);
  printf("lookup (1 << 9) + 3 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_lookup(&im, ((5L << 18) + (1L << 9) + 3L) << PAGE_SHIFT);
  printf("lookup (5 << 18) + (1 << 9) + 3 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_map(&im, 0, 0x101010);
  printf("map 0 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_map(&im, 1 << PAGE_SHIFT, 0x101010);
  printf("map 1 << PAGE_SHIFT = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_map(&im, ((1L << 9) + 3L) << PAGE_SHIFT, 0x202020);
  printf("map (1 << 9) + 3 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_map(&im, ((1L << 9) + 4L) << PAGE_SHIFT, 0x202020);
  printf("map (1 << 9) + 4 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_map(&im, ((5L << 18) + (1L << 9) + 3L) << PAGE_SHIFT, 0x303030);
  printf("map (5 << 18) + (1 << 9) + 3 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_map(&im, ((5L << 18) + (2L << 9) + 3L) << PAGE_SHIFT, 0x303030);
  printf("map (5 << 18) + (2 << 9) + 3 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_map(&im, ((5L << 18) + (2L << 9) + 4L) << PAGE_SHIFT, 0x303030);
  printf("map (5 << 18) + (2 << 9) + 3 = %d %"PRIx64"\n", ipte.v, ipte.pfn);


  ipte = imap_lookup(&im, 0);
  printf("lookup 0 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_lookup(&im, ((1L << 9) + 3L) << PAGE_SHIFT);
  printf("lookup (1 << 9) + 3 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

  ipte = imap_lookup(&im, ((5L << 18) + (1L << 9) + 3L) << PAGE_SHIFT);
  printf("lookup (5 << 18) + (1 << 9) + 3 = %d %"PRIx64"\n", ipte.v, ipte.pfn);

}
