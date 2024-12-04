/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <stdbool.h>
#include <machina/types.h>

#define __syscall_msgbuf -1L

#define __syscall_msgsend -20L
#define __syscall_msgrecv -21L
#define __syscall_reply_port -26L
#define __syscall_task_self -27L


#define __syscall_vm_region -63L
struct __syscall_vm_region_out {
  mcn_vmaddr_t addr;
  unsigned long size;
  mcn_vmprot_t curprot;
  mcn_vmprot_t maxprot;
  mcn_vminherit_t inherit;
  bool shared;
  mcn_portid_t objname;
  mcn_vmoff_t off;
};

#define __syscall_vm_map -64L
struct __syscall_vm_map_in {
  mcn_vmaddr_t addr;
  unsigned long size;
  mcn_vmaddr_t mask;
  bool anywhere;
  mcn_portid_t objname;
  mcn_vmoff_t off;
  bool copy;
  mcn_vmprot_t curprot;
  mcn_vmprot_t maxprot;
  mcn_vminherit_t inherit;
};
#define __syscall_vm_allocate -65L
#define __syscall_vm_deallocate -66L
#define __syscall_port_allocate -72L
