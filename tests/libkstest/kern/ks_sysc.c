/*
  Simple syscalls defined in a kernel module.
*/

#include "internal.h"
#include <machina/error.h>
#include <ks_syscalls.h>

mcn_return_t
syscall_getport (mcn_portid_t port, struct portref *portref)
{
  mcn_return_t rc;

  struct ipcspace *ps = task_getipcspace (cur_task ());
  rc = ipcspace_resolve (ps, MCN_MSGTYPE_COPYSEND, port, portref);
  task_putipcspace (cur_task (), ps);
  return rc;
}

mcn_return_t
syscall_setport (struct portref portref, mcn_portid_t * portid)
{
  mcn_return_t rc;
  struct portright right = portright_from_portref (RIGHT_SEND, portref);

  struct ipcspace *ps = task_getipcspace (cur_task ());
  rc = ipcspace_insertright (ps, &right, portid);
  task_putipcspace (cur_task (), ps);
  return rc;
}

bool
kstest_sysc (uctxt_t * u,
	     unsigned long a1, unsigned long a2, unsigned long a3,
	     unsigned long a4, unsigned long a5, unsigned long a6,
	     unsigned long a7, long *out)
{
  long ret;

  switch (a1)
    {
    case 0:
      info ("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 1:
      assert (a2 == 1);
      info ("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 2:
      assert (a2 == 1);
      assert (a3 == 2);
      info ("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 3:
      assert (a2 == 1);
      assert (a3 == 2);
      assert (a4 == 3);
      info ("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 4:
      assert (a2 == 1);
      assert (a3 == 2);
      assert (a4 == 3);
      assert (a5 == 4);
      info ("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 5:
      assert (a2 == 1);
      assert (a3 == 2);
      assert (a4 == 3);
      assert (a5 == 4);
      assert (a6 == 5);
      info ("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 6:
      assert (a2 == 1);
      assert (a3 == 2);
      assert (a4 == 3);
      assert (a5 == 4);
      assert (a6 == 5);
      assert (a7 == 6);
      info ("SYSC%ld test passed.", a1);
      ret = 0;
      break;
    case 4096:
      putchar (a2);
      ret = 0;
      break;

    case _test_syscall_vm_map:
      {
	volatile struct _test_syscall_vm_map_in in =
	  *(volatile struct _test_syscall_vm_map_in *) cur_kmsgbuf ();
	struct portref task_pr, vmobj_pr;
	struct taskref tref;
	struct vmobjref vmobjref;
	vaddr_t vaddr;

	nuxperf_inc (&pmachina_sysc_vm_map);
	
	ret = syscall_getport (a2, &task_pr);
	if (ret)
	  break;

	/* Valid references: TASK PORTREF */

	tref = port_get_taskref (portref_unsafe_get (&task_pr));
	portref_consume (&task_pr);
	if (taskref_isnull (&tref))
	  {
	    ret = KERN_INVALID_NAME;
	    break;
	  }

	/* Valid references: TASK REF */

	ret = syscall_getport (in.objname, &vmobj_pr);
	if (ret)
	  {
	    taskref_consume (&tref);
	    break;
	  }

	/* Valid references: TASK REF, VMOBJ PORTREF */

	vmobjref =
	  port_get_vmobjref_from_name (portref_unsafe_get (&vmobj_pr));
	portref_consume (&vmobj_pr);
	if (vmobjref_isnull (&vmobjref))
	  {
	    taskref_consume (&tref);
	    ret = KERN_INVALID_NAME;
	    break;
	  }

	/* Valid references: TASK REF, VMOBJ REF */

	printf
	  ("SYSC: map task %p addr %lx size %lx mask %lx anywhere ? %d\n",
	   taskref_unsafe_get (&tref), in.addr, in.size, in.mask, in.anywhere,
	   in.objname, in.off, in.copy, in.curprot, in.maxprot, in.inherit);
	vaddr = in.addr;
	ret =
	  task_vm_map (taskref_unsafe_get (&tref), &vaddr, in.size, in.mask,
		       in.anywhere, vmobjref, in.off, in.copy, in.curprot,
		       in.maxprot, in.inherit);

	taskref_consume (&tref);
	vmobjref_consume (&vmobjref);
	*(mcn_vmaddr_t *) cur_kmsgbuf () = vaddr;
      }
      break;
    case _test_syscall_vm_allocate:
      {
	mcn_vmaddr_t addr = *(mcn_vmaddr_t *) cur_kmsgbuf ();
	struct portref task_pr;
	struct taskref tref;

	nuxperf_inc (&pmachina_sysc_vm_allocate);

	ret = syscall_getport (a2, &task_pr);
	if (ret)
	  break;

	/* Valid references: TASK PORTREF */

	tref = port_get_taskref (portref_unsafe_get (&task_pr));
	portref_consume (&task_pr);
	if (taskref_isnull (&tref))
	  {
	    ret = KERN_INVALID_NAME;
	    break;
	  }

	/* Valid references: TASK REF */

	printf
	  ("SYSC: vmallocate task %p, addr %lx, size %lx, anywhere? %ld\n",
	   taskref_unsafe_get (&tref), addr, a3, a4);
	ret = task_vm_allocate (taskref_unsafe_get (&tref), &addr, a3, a4);
	taskref_consume (&tref);
	*(mcn_vmaddr_t *) cur_kmsgbuf () = addr;
	break;
      }

    case _test_syscall_vm_region:
      {
	struct portref task_pr;
	struct taskref tref;

	nuxperf_inc (&pmachina_sysc_vm_region);

	printf ("SYSC: VMREGION GETPORT %lx\n", a2);
	ret = syscall_getport (a2, &task_pr);
	if (ret)
	  break;

	/* Valid references: TASK PORTREF */

	tref = port_get_taskref (portref_unsafe_get (&task_pr));
	portref_consume (&task_pr);
	if (taskref_isnull (&tref))
	  {
	    ret = KERN_INVALID_NAME;
	    break;
	  }

	/* Valid references: TASK REF */

	mcn_vmaddr_t addr = *(volatile mcn_vmaddr_t *) cur_kmsgbuf ();
	volatile struct _test_syscall_vm_region_out *out = cur_kmsgbuf ();
	size_t size;
	mcn_vmprot_t curprot, maxprot;
	mcn_vminherit_t inherit;
	bool shared;
	struct portref portref;
	mcn_portid_t objname;
	mcn_vmoff_t off;

	printf ("SYSC: vmregion task %p addr %lx\n",
		taskref_unsafe_get (&tref), addr);
	ret =
	  task_vm_region (taskref_unsafe_get (&tref), &addr, &size, &curprot,
			  &maxprot, &inherit, &shared, &portref, &off);
	taskref_consume (&tref);
	if (ret)
	  break;

	/* Valid references: PORTREF */

	ret = syscall_setport (portref, &objname);

	/* Valid references: none */

	if (ret)
	    break;

	out->addr = addr;
	out->size = size;
	out->curprot = curprot;
	out->maxprot = maxprot;
	out->inherit = inherit;
	out->shared = shared;
	out->objname = objname;
	out->off = off;
	ret = KERN_SUCCESS;
	break;
      }
      
    default:
      return false;
    }

  *out = ret;
  return true;
}

KERNEL_SYSCALL_DEMUX(kstest_sysc);
