/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_TYPES_H_
#define _MACHINA_TYPES_H_

#include <stdint.h>

typedef unsigned long ulong_t;

typedef unsigned mcn_bool_t;
#define TRUE 1
#define FALSE 0

typedef unsigned long mcn_portid_t;
#define MCN_PORTID_NULL 0
#define MCN_PORTID_DEAD -1

typedef mcn_portid_t *mcn_portid_array_t;

typedef uint8_t mcn_portright_t;
#define MCN_PORTRIGHT_SEND 0
#define MCN_PORTRIGHT_RECV 1
#define MCN_PORTRIGHT_ONCE 2
#define MCN_PORTRIGHT_PSET 3
#define MCN_PORTRIGHT_DEAD 4

typedef uint8_t mcn_porttype_t;
#define MCN_PORTTYPE(right) ((mcn_porttype_t)(1 << ((right)+16)))
#define MCN_PORTTYPE_SEND   MCN_PORTTYPE(MCN_PORTRIGHT_SEND)
#define MCN_PORTTYPE_RECV   MCN_PORTTYPE(MCN_PORTRIGHT_RECV)
#define MCN_PORTTYPE_ONCE   MCN_PORTTYPE(MCN_PORTRIGHT_ONCE)
#define MCN_PORTTYPE_PSET   MCN_PORTTYPE(MCN_PORTRIGHT_PSET)
#define MCN_PORTTYPE_DEAD   MCN_PORTTYPE(MCN_PORTRIGHT_DEAD)

#define MCN_PORTTYPE_DNREQUEST (1L << 31)
#define MCN_PORTTYPE_MAREQUEST (1L << 30)

typedef uint32_t mcn_seqno_t;
typedef unsigned mcn_msgcount_t;

#define MCN_QLIMIT_DEFAULT ((mcn_msgcount_t) 5)
#define MCN_QLIMIT_MAX ((mcn_msgcount_t) 16)

typedef int mcn_return_t;

typedef unsigned mcn_msgopt_t;
#define MCN_MSGOPT_NONE			0x000
#define MCN_MSGOPT_SEND_TIMEOUT		0x010
#define MCN_MSGOPT_SEND_NOTIFY		0x020
#define MCN_MSGOPT_SEND_CANCEL		0x080
#define MCN_MSGOPT_RECV_TIMEOUT		0x100
#define MCN_MSGOPT_RECV_NOTIFY		0x200
#define MCN_MSGOPT_RECV_LARGE		0x400

#define MCN_MSGTIMEOUT_NONE 0

typedef unsigned long mcn_vmoff_t;
typedef unsigned long mcn_vmaddr_t;
typedef unsigned long mcn_vmsize_t;

typedef unsigned mcn_vmprot_t;
#define MCN_VMPROT_NONE		0
#define MCN_VMPROT_READ		1
#define MCN_VMPROT_WRITE	2
#define MCN_VMPROT_EXECUTE	4

#define MCN_VMPROT_DEFAULT	(MCN_VMPROT_READ|MCN_VMPROT_WRITE)
#define MCN_VMPROT_ALL		(MCN_VMPROT_READ|MCN_VMPROT_WRITE|MCN_VMPROT_EXECUTE)
#define MCN_VMPROT_NO_CHANGE 8	/* Used by lock_request */

typedef unsigned mcn_vminherit_t;
#define MCN_VMINHERIT_SHARE ((mcn_vminherit_t) 0)
#define MCN_VMINHERIT_COPY  ((mcn_vminherit_t) 1)
#define MCN_VMINHERIT_NONE  ((mcn_vminherit_t) 2)

#define MCN_VMINHERIT_DEFAULT MCN_VMINHERIT_COPY

typedef mcn_portid_t mcn_memobj_t;
#define MCN_MEMOBJ_NULL MCN_PORTID_NULL

typedef mcn_portid_t mcn_memobj_ctrl_t;
typedef mcn_portid_t mcn_memobj_name_t;

typedef int mcn_memobj_copy_strategy_t;
#define MCN_MEMOBJ_COPY_NONE  0
#define MCN_MEMOBJ_COPY_CALL  1
#define MCN_MEMOBJ_COPY_DELAY 2
#define MCN_MEMOBJ_COPY_TEMP  3

typedef int mcn_memobj_return_t;
#define MCN_MEMOBJ_RETURN_NONE  0
#define MCN_MEMOBJ_RETURN_DIRTY 1
#define MCN_MEMOBJ_RETURN_ALL   2


#endif
