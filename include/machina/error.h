/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_ERROR_H_
#define _MACHINA_ERROR_H_

/*
  This follow the Mach error system:

	hi		 		       lo
	| system(6) | subsystem(12) | code(14) |
*/

#define err_system(_s) ((_s) & 0x3f) << 26)
#define err_sub(_s)    ((_s) & 0xfff) << 14)

#define err_get_system(_e) (((_e) >> 26) & 0x3f)
#define err_get_sub(_e)    (((_e) >> 14) & 0xfff)
#define err_get_code(_e)   (((_e) & 0x3fff)

#define system_emask  err_system(0x3f)
#define ssub_emask    err_system(0xfff)
#define code_emask    0x3ff

#define err_kern      err_system(0x00)	/* Kernel. */
#define err_us        err_system(0x01)	/* Userspace Library. */
#define err_server    err_system(0x02)	/* Userspace Servers.  */
#define err_ipc       err_system(0x04)	/* Machina IPC Errors. */
#define err_bootstrap err_system(0x05)	/* Bootstrap Errors. */
#define err_local     err_system(0x3e)	/* User-defined Errors. */
#define	err_max_system 0x3f


/*
  mcn_return_t
*/

#define KERN_SUCCESS		0

#define KERN_INVALID_ADDRESS	1	/* Invalid Address. */
#define KERN_PROTECTION_FAILURE	2	/* Protection Fault. */
#define KERN_NO_SPACE		3	/* No space in VM. */
#define KERN_INVALID_ARGUMENT	4	/* Invalid Argument. */
#define KERN_FAILURE		5	/* Generic Failure. */
#define KERN_RESOURCE_SHORTAGE	6	/* Not enough resources. */
#define KERN_NOT_RECEIVER	7	/* Can't receive from port. */
#define KERN_NO_ACCESS		8	/* Generic Access Restriction. */
#define KERN_MEMORY_FAILURE	9	/* Memory Object Destroyed. */
#define KERN_MEMORY_ERROR	10	/* Unable to retrieve Memory Object Data. */
#define KERN_NOT_IN_SET		12	/* Receive right not in Port Set. */
#define KERN_NAME_EXISTS	13	/* Port Name already exists. */
#define KERN_ABORTED		14	/* Operation Aborted. Handled by IPC. */
#define KERN_INVALID_NAME	15	/* Port Name invalid. */
#define	KERN_INVALID_TASK	16	/* Task Invalid. */
#define KERN_INVALID_RIGHT	17	/* Port Right Invalid. */
#define KERN_INVALID_VALUE	18	/* Generic Value Error. */
#define	KERN_UREFS_OVERFLOW     19	/* Operation would overflow user references. */
#define	KERN_INVALID_CAPABILITY	20	/* Port Right doesn't allow operation. */
#define KERN_RIGHT_EXISTS	21	/* Port Right already exists. */
#define	KERN_INVALID_HOST	22	/* Not a Host. */
#define KERN_MEMORY_PRESENT	23	/* Precious Memory Already Present. */


#define KERN_RETRY		100	/* Thread has been queued. Retry operation. */
#define KERN_THREAD_TIMEDOUT	101
#endif
