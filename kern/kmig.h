/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _KMIG_H
#define _KMIG_H

/*
  MIG dependencies for MIG kernel code.
*/

#include <machina/types.h>

#include "internal.h"

/*
  A Kernel MIG Server Demux pointer lives in a special section.
*/
#define __kernel_server __attribute__((section(".data_ext0"),used))
#define KERNEL_SERVER_DEMUX(_fn)		\
  uintptr_t __kernel_server  _fn##_ptr = (uintptr_t)(_fn)

/*
  Task References.
*/

typedef struct taskref taskref_t;

static inline struct taskref
taskport_to_taskref (ipc_port_t port)
{
  return port_get_taskref (ipcport_unsafe_get (port));
}

static inline ipc_port_t
taskref_to_taskport (struct taskref task)
{
  struct portref pr;
  pr = task_getport (taskref_unsafe_get (&task));
  return portref_to_ipcport (&pr);
}

static inline void
taskref_deallocate (struct taskref task)
{
  taskref_consume (&task);
}


typedef struct threadref threadref_t;

static inline struct threadref
threadport_to_threadref (ipc_port_t port)
{
  return port_get_threadref (ipcport_unsafe_get (port));
}

static inline ipc_port_t
threadref_to_threadport (struct threadref thread)
{
  struct portref pr;
  pr = thread_getport (threadref_unsafe_get (&thread));
  return portref_to_ipcport (&pr);
}

typedef struct vmobjref vmobjref_t;

static inline struct vmobjref
vmobjctrlport_to_vmobjref (ipc_port_t port)
{
  return port_get_vmobjref (ipcport_unsafe_get (port));
}

static inline ipc_port_t
vmobjref_to_vmobjctrlport (struct vmobjref vmobj)
{
  struct portref pr;
  pr = vmobj_getctrlport (vmobjref_unsafe_get (&vmobj));
  return portref_to_ipcport (&pr);
}

static inline struct vmobjref
vmobjnameport_to_vmobjref (ipc_port_t port)
{
  return port_get_vmobjref_from_name (ipcport_unsafe_get (port));
}

static inline ipc_port_t
vmobjref_to_vmobjnameport (struct vmobjref vmobj)
{
  struct portref pr;
  pr = vmobj_getnameport (vmobjref_unsafe_get (&vmobj));
  return portref_to_ipcport (&pr);
}

typedef struct host *hostptr_t;

static inline struct host *
ctrlport_to_host (ipc_port_t port)
{
  return port_get_host_from_ctrl (ipcport_unsafe_get (port));
}

static inline ipc_port_t
host_to_ctrlport (struct host *host)
{
  struct portref pr;
  pr = host_getctrlport (host);
  return portref_to_ipcport (&pr);
}

static inline struct host *
nameport_to_host (ipc_port_t port)
{
  return port_get_host_from_name (ipcport_unsafe_get (port));
}

static inline ipc_port_t
host_to_nameport (struct host *host)
{
  struct portref pr;
  pr = host_getnameport (host);
  return portref_to_ipcport (&pr);
}

#endif
