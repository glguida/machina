/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_TYPES_H_
#define _MACHINA_TYPES_H_

typedef unsigned long ulong_t;

typedef unsigned long mcn_portid_t;
#define MCN_PORTID_NULL 0
#define MCN_PORTID_DEAD -1

typedef unsigned mcn_portright_t;
#define MCN_PORTRIGHT_SEND 0
#define MCN_PORTRIGHT_RECEIVE 1
#define MCN_PORTRIGHT_SENDONCE 2
#define MCN_PORTRIGHT_PORTSET 3
#define MCN_PORTRIGHT_DEADNAME 4

typedef int mcn_return_t;

typedef unsigned mcn_msgopt_t;
#define MCN_MSGOPT_NONE			0x000
#define MCN_MSGOPT_SEND_TIMEOUT		0x010
#define MCN_MSGOPT_SEND_NOTIFY		0x020
#define MCN_MSGOPT_SEND_CANCEL		0x080
#define MCN_MSGOPT_RECV_TIMEOUT		0x100
#define MCN_MSGOPT_RECV_NOTIFY		0x200

#define MCN_MSGTIMEOUT_NONE 0

typedef unsigned long mcn_vmoff_t;
typedef unsigned long mcn_vmaddr_t;

typedef unsigned mcn_vmprot_t;
#define MCN_VMPROT_NONE		0
#define MCN_VMPROT_READ		1
#define MCN_VMPROT_WRITE	2
#define MCN_VMPROT_EXECUTE	4
#define MCN_VMPROT_DEFAULT	(MCN_VMPROT_READ|MCN_VMPROT_WRITE)
#define MCN_VMPROT_ALL		(MCN_VMPROT_READ|MCN_VMPROT_WRITE|MCN_VMPROT_EXECUTE)

#define MCN_VMPROT_NO_CHANGE 8	/* Used by lock_request */

#endif
