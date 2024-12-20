#ifndef KS_H
#define KS_H

#include <ks_user.h>

mcn_return_t syscall_port_allocate (mcn_portid_t task, mcn_portright_t right,
				    mcn_portid_t * name);

mcn_return_t syscall_vm_region (mcn_portid_t task, mcn_vmaddr_t * addr,
				unsigned long *size, mcn_vmprot_t * curprot,
				mcn_vmprot_t * maxprot,
				mcn_vminherit_t * inherit,
				unsigned *shared, mcn_portid_t * nameid,
				mcn_vmoff_t * off);

mcn_return_t syscall_vm_map (mcn_portid_t task, mcn_vmaddr_t * addr,
			     unsigned long size, mcn_vmaddr_t mask,
			     unsigned anywhere, mcn_portid_t objname,
			     mcn_vmoff_t off, unsigned copy,
			     mcn_vmprot_t curprot, mcn_vmprot_t maxprot,
			     mcn_vminherit_t inherit);

mcn_return_t syscall_vm_allocate (mcn_portid_t task, mcn_vmaddr_t * addr,
				  unsigned long size, int anywhere);

#endif
