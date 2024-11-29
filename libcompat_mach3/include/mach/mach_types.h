#ifndef MACH_TYPES_H
#define MACH_TYPES_H

#include <machina/machina_types.h>

typedef mcn_task_t task_t;
typedef mcn_task_array_t task_array_t;
typedef mcn_vmtask_t mcn_vm_task_t;
typedef mcn_ipcspace_t ipc_space_t;
typedef mcn_thread_t thread_t;
typedef mcn_thread_array_t thread_array_t;
typedef mcn_host_t host_t;
typedef mcn_host_priv_t host_priv_t;
typedef mcn_processor_t processor_t;
typedef mcn_processor_array_t processor_array_t;
//typedef mcn_processor_set_t procesor_set_t;
//typedef mcn_processor_set_name_t processor_set_name;
//typedef mcn_processor_set_array_t processor_set_array_t;
//typedef mcn_processor_set_name_array_t processor_set_name_array_t;
typedef mcn_emulvec_t emul_vector_t;

#include <mach/std_types.h>

#endif
