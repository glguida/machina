#include <stdbool.h>
#include <machina/types.h>

#define _test_syscall_vm_region -63L
struct _test_syscall_vm_region_out
{
  mcn_vmaddr_t addr;
  unsigned long size;
  mcn_vmprot_t curprot;
  mcn_vmprot_t maxprot;
  mcn_vminherit_t inherit;
  bool shared;
  mcn_portid_t objname;
  mcn_vmoff_t off;
};

#define _test_syscall_vm_map -64L
struct _test_syscall_vm_map_in
{
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
#define _test_syscall_vm_allocate -65L
#define _test_syscall_vm_deallocate -66L
#define _test_syscall_port_allocate -72L
