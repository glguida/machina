/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_MESSAGE_H_
#define _MACHINA_MESSAGE_H_

#include <stdint.h>
#include <machina/types.h>

typedef struct
{
  uint32_t msgt_name:8,
    msgt_size:8,
    msgt_number:12,
    msgt_inline:1, msgt_longform:1, msgt_deallocate:1, msgt_unused:1;
} mcn_msgtype_t;

typedef struct
{
  mcn_msgtype_t msgtl_header;
  unsigned short msgtl_name;
  unsigned short msgtl_size;
  unsigned msgtl_number;
} mcn_msgtype_long_t;

typedef uint8_t mcn_msgtype_name_t;
#define MCN_MSGTYPE_UNSTRUCTURED	0x00
#define MCN_MSGTYPE_BIT			0x00
#define MCN_MSGTYPE_BOOLEAN		0x00
#define MCN_MSGTYPE_INT16		0x01
#define MCN_MSGTYPE_INT32		0x02
#define MCN_MSGTYPE_CHAR		0x08
#define MCN_MSGTYPE_BYTE		0x09
#define MCN_MSGTYPE_INT8		0x09
#define MCN_MSGTYPE_REAL		0x0a
#define MCN_MSGTYPE_INT64		0x0b
#define MCN_MSGTYPE_STRING		0x0c
#define MCN_MSGTYPE_CSTRING		0x0c

#define MCN_MSGTYPE_PORTNAME		0x0f
#define MCN_MSGTYPE_MOVERECV		0x10
#define MCN_MSGTYPE_MOVESEND		0x11
#define MCN_MSGTYPE_MOVEONCE		0x12
#define MCN_MSGTYPE_COPYSEND		0x13
#define MCN_MSGTYPE_MAKESEND		0x14
#define MCN_MSGTYPE_MAKEONCE		0x15
#define MCN_MSGTYPE_LAST		0x17

#define MCN_MSGTYPE_PORTRECV		0x10
#define MCN_MSGTYPE_PORTONCE		0x12
#define MCN_MSGTYPE_PORTSEND		0x11

#define MCN_MSGTYPE_POLYMORPHIC		((mcn_msgtype_name_t)-1)

#define MCN_MSGTYPE_IS_PORT(_x)			\
  (((_x) >= MCN_MSGTYPE_MOVERECV) &&		\
   ((_x) <= MCN_MSGTYPE_MAKEONCE))

#define MCN_MSGTYPE_IS_SEND(_x)			\
  (((_x) >= MCN_MSGTYPE_MOVESEND) &&		\
   ((_x) <= MCN_MSGTYPE_MAKEONCE))


typedef uint32_t mcn_msgbits_t;
#define MCN_MSGBITS_NONE	0x00000
#define MCN_MSGBITS_REMOTE_MASK	0x000ff
#define MCN_MSGBITS_LOCAL_MASK	0x0ff00
#define MCN_MSGBITS_COMPLEX	0x80000

#define MCN_MSGBITS_REMOTE(_b) ((_b) & MCN_MSGBITS_REMOTE_MASK)
#define MCN_MSGBITS_LOCAL(_b) (((_b) & MCN_MSGBITS_LOCAL_MASK) >> 8)
#define MCN_MSGBITS(_remote,_local) ((_remote)|((_local) << 8))

typedef uint32_t mcn_msgsize_t;

typedef unsigned long mcn_msgid_t;

typedef unsigned long mcn_seqno_t;

typedef mcn_return_t mcn_msgioret_t;
#define MSGIO_SUCCESS  0x00000000

#define MSGIO_MSG_MASK   0x00003c00
#define MSGIO_MSG_IPC_SPACE  0x00002000
#define MSGIO_MSG_VM_SPACE  0x00001000
#define MSGIO_MSG_IPC_KERNEL  0x00000800
#define MSGIO_MSG_VM_KERNEL  0x00000400

#define MSGIO_SEND_IN_PROGRESS  0x10000001
#define MSGIO_SEND_INVALID_DATA  0x10000002
#define MSGIO_SEND_INVALID_DEST  0x10000003
#define MSGIO_SEND_TIMED_OUT  0x10000004
#define MSGIO_SEND_WILL_NOTIFY  0x10000005
#define MSGIO_SEND_NOTIFY_IN_PROGRESS 0x10000006
#define MSGIO_SEND_INTERRUPTED  0x10000007
#define MSGIO_SEND_MSG_TOO_SMALL  0x10000008
#define MSGIO_SEND_INVALID_REPLY  0x10000009
#define MSGIO_SEND_INVALID_RIGHT  0x1000000a
#define MSGIO_SEND_INVALID_NOTIFY 0x1000000b
#define MSGIO_SEND_INVALID_MEMORY 0x1000000c
#define MSGIO_SEND_NO_BUFFER  0x1000000d
#define MSGIO_SEND_NO_NOTIFY  0x1000000e
#define MSGIO_SEND_INVALID_TYPE  0x1000000f
#define MSGIO_SEND_INVALID_HEADER 0x10000010

#define MSGIO_RCV_IN_PROGRESS  0x10004001
#define MSGIO_RCV_INVALID_NAME  0x10004002
#define MSGIO_RCV_TIMED_OUT  0x10004003
#define MSGIO_RCV_TOO_LARGE  0x10004004
#define MSGIO_RCV_INTERRUPTED  0x10004005
#define MSGIO_RCV_PORT_CHANGED  0x10004006
#define MSGIO_RCV_INVALID_NOTIFY  0x10004007
#define MSGIO_RCV_INVALID_DATA  0x10004008
#define MSGIO_RCV_PORT_DIED  0x10004009
#define MSGIO_RCV_IN_SET   0x1000400a
#define MSGIO_RCV_HEADER_ERROR  0x1000400b
#define MSGIO_RCV_BODY_ERROR  0x1000400c


typedef struct mcn_msgheader
{
  mcn_msgbits_t msgh_bits;
  mcn_msgsize_t msgh_size;
  mcn_portid_t msgh_remote;
  mcn_portid_t msgh_local;
  mcn_seqno_t msgh_seqno;
  mcn_msgid_t msgh_msgid;
} mcn_msgheader_t;

#endif
