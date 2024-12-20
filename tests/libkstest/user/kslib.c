#include <stdio.h>
#include <nux/syscalls.h>
#include <machina/types.h>
#include <machina/error.h>
#include <machina/syscalls.h>
#include <ks_syscalls.h>
#include <ks.h>

mcn_return_t
syscall_port_allocate (mcn_portid_t task, mcn_portright_t right,
		       mcn_portid_t * name)
{
  mcn_return_t r;

  r = syscall2 (_test_syscall_port_allocate, task, right);
  if (r == KERN_SUCCESS)
    *name = *(mcn_portid_t *) __local_msgbuf;
  return r;
}

mcn_return_t
syscall_vm_region (mcn_portid_t task, mcn_vmaddr_t * addr,
		   unsigned long *size, mcn_vmprot_t * curprot,
		   mcn_vmprot_t * maxprot, mcn_vminherit_t * inherit,
		   unsigned *shared, mcn_portid_t * nameid, mcn_vmoff_t * off)
{
  mcn_return_t r;
  volatile struct _test_syscall_vm_region_out *out =
    (volatile struct _test_syscall_vm_region_out *) __local_msgbuf;
  *(volatile mcn_vmaddr_t *) __local_msgbuf = *addr;
  r = syscall1 (_test_syscall_vm_region, task);
  if (r == KERN_SUCCESS)
    {
      *addr = out->addr;
      *size = out->size;
      *curprot = out->curprot;
      *maxprot = out->maxprot;
      *inherit = out->inherit;
      *shared = out->shared;
      *nameid = out->objname;
      *off = out->off;
    }
  return r;
}

mcn_return_t
syscall_vm_map (mcn_portid_t task, mcn_vmaddr_t * addr,
		unsigned long size, mcn_vmaddr_t mask,
		unsigned anywhere, mcn_portid_t objname,
		mcn_vmoff_t off, unsigned copy,
		mcn_vmprot_t curprot, mcn_vmprot_t maxprot,
		mcn_vminherit_t inherit)
{
  mcn_return_t r;
  volatile struct _test_syscall_vm_map_in *in =
    (volatile struct _test_syscall_vm_map_in *) __local_msgbuf;

  in->addr = *addr;
  in->size = size;
  in->mask = mask;
  in->anywhere = anywhere;
  in->objname = objname;
  in->off = off;
  in->copy = copy;
  in->curprot = curprot;
  in->maxprot = maxprot;
  in->inherit = inherit;
  r = syscall1 (_test_syscall_vm_map, task);
  *addr = *(mcn_vmaddr_t *) __local_msgbuf;
  return r;
}

mcn_return_t
syscall_vm_allocate (mcn_portid_t task, mcn_vmaddr_t * addr,
		     unsigned long size, int anywhere)
{
  mcn_return_t r;

  *(volatile mcn_vmaddr_t *) __local_msgbuf = *addr;
  r = syscall3 (_test_syscall_vm_allocate, task, size, anywhere);
  *addr = *(mcn_vmaddr_t *) __local_msgbuf;
  return r;
}
