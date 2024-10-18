/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_MESSAGE_H_
#define _MACHINA_MESSAGE_H_

#include <stdint.h>

#define MCN_MSGTYPE_DATA	0x00
#define MCN_MSGTYPE_INT8	0x01
#define MCN_MSGTYPE_INT16	0x02
#define MCN_MSGTYPE_INT32	0x03
#define MCN_MSGTYPE_INT64	0x04
#define MCN_MSGTYPE_CHAR	0x08
#define MCN_MSGTYPE_REAL	0x0a
#define MCN_MSGTYPE_CSTR	0x0b
#define MCN_MSGTYPE_PORTNAME	0x0f
#define MCN_MSGTYPE_MOVERECV	0x10
#define MCN_MSGTYPE_MOVESEND	0x11
#define MCN_MSGTYPE_MOVEONCE	0x12
#define MCN_MSGTYPE_COPYSEND	0x13
#define MCN_MSGTYPE_MAKESEND	0x14
#define MCN_MSGTYPE_MAKEONCE	0x15
#define MCN_MSGTYPE_LAST	0x17

#define MCN_MSGTYPE_IS_PORT(_x)			\
  (((_x) >= MCN_MSGTYPE_MOVERECV) &&		\
   ((_x) <= MCN_MSGTYPE_MAKEONCE))

#define MCN_MSGTYPE_IS_SEND(_x)			\
  (((_x) >= MCN_MSGTYPE_MOVESEND) &&		\
   ((_x) <= MCN_MSGTYPE_MAKEONCE))


typedef uint32_t mcn_msgflag_t;
#define MCN_MSGFLAG_NONE	0x00000
#define MCN_MSGFLAG_REMOTE_MASK	0x0000f
#define MCN_MSGFLAG_LOCAL_MASK	0x000f0
#define MCN_MSGFLAG_COMPLEX	0x80000

#define MCN_MSGFLAG_LOCAL_MOVERECV	0x10
#define MCN_MSGFLAG_LOCAL_MOVESEND	0x20
#define MCN_MSGFLAG_LOCAL_MOVEONCE	0x30
#define MCN_MSGFLAG_LOCAL_COPYSEND	0x40
#define MCN_MSGFLAG_LOCAL_MAKESEND	0x50
#define MCN_MSGFLAG_LOCAL_MAKEONCE	0x60

#define MCN_MSGFLAG_REMOTE_MOVERECV	0x01
#define MCN_MSGFLAG_REMOTE_MOVESEND	0x02
#define MCN_MSGFLAG_REMOTE_MOVEONCE	0x03
#define MCN_MSGFLAG_REMOTE_COPYSEND	0x04
#define MCN_MSGFLAG_REMOTE_MAKESEND	0x05
#define MCN_MSGFLAG_REMOTE_MAKEONCE	0x06

#define MCN_MSGFLAG_REMOTE(_b) ((_b) & MCN_MSGFLAG_REMOTE_MASK)
#define MCN_MSGFLAG_LOCAL(_b) ((_b) & MCN_MSGFLAG_REMOTE_MASK)

typedef uint32_t mcn_msgsize_t;

typedef unsigned long mcn_msgid_t;

struct mcn_msgsend {
  mcn_msgflag_t msgs_flag;
  mcn_msgsize_t msgs_size;
  mcn_portid_t msgs_remote;
  mcn_portid_t msgs_local;
  mcn_msgid_t msgs_msgid;
};

#endif
