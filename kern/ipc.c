/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <alloca.h>
#include <machina/error.h>
#include <machina/message.h>

static inline mcn_msgtype_name_t
msgbits_sendrecv_intern(mcn_msgtype_name_t type)
{
  switch (type)
    {
    case 0:
      return 0;

    case MCN_MSGTYPE_MOVESEND:
    case MCN_MSGTYPE_COPYSEND:
    case MCN_MSGTYPE_MAKESEND:
      return MCN_MSGTYPE_PORTSEND;

    case MCN_MSGTYPE_MOVEONCE:
    case MCN_MSGTYPE_MAKEONCE:
      return MCN_MSGTYPE_PORTONCE;

    default:
      fatal("Wrong type %d\n", type);
      return -1;
    }
}

void
intmsg_consume(mcn_msgheader_t *intmsg)
{
  if (intmsg->msgh_remote != 0)
    portref_consume(ipcport_to_portref(&intmsg->msgh_remote));
  if (intmsg->msgh_local != 0)
    portref_consume(ipcport_to_portref(&intmsg->msgh_local));
}

static mcn_msgioret_t
externalize(struct portspace *ps, mcn_msgheader_t *intmsg, volatile mcn_msgheader_t *extmsg, size_t size)
{
  mcn_msgioret_t rc;
  mcn_portid_t local, remote;

  assert (size >= sizeof(mcn_msgheader_t));
  assert (size <= MSGBUF_SIZE);

  struct portref local_pref = ipcport_to_portref(&intmsg->msgh_local);
  local = portspace_lookup(ps, portref_unsafe_get(&local_pref));
  portref_consume(local_pref);

  remote = MCN_PORTID_NULL;
  if (intmsg->msgh_remote != 0)
    {
      const mcn_msgtype_name_t rembits = MCN_MSGBITS_REMOTE(intmsg->msgh_bits);
      struct portref portref = ipcport_to_portref(&intmsg->msgh_remote);
      struct portright pr = portright_from_portref(rembits, portref);
      rc = portspace_insertright(ps, &pr, &remote);
      if (rc)
	{
	  remote = MCN_PORTID_NULL;
	  portright_consume(&pr);
	}
    }

  extmsg->msgh_bits = intmsg->msgh_bits;
  extmsg->msgh_remote = remote;
  extmsg->msgh_local = local;
  extmsg->msgh_size = size;
  extmsg->msgh_seqno = 0; /* XXX: SEQNO */
  extmsg->msgh_msgid = intmsg->msgh_msgid;

  memcpy((void *)(extmsg + 1), (void *)(intmsg + 1), size - sizeof(mcn_msgheader_t));
  return MSGIO_SUCCESS;
}

static mcn_msgioret_t
internalize(struct portspace *ps, volatile mcn_msgheader_t *extmsg, mcn_msgheader_t *intmsg, size_t size)
{
  mcn_msgioret_t rc;

  assert (size >= sizeof(mcn_msgheader_t));
  assert (size <= MSGBUF_SIZE);

  const mcn_msgbits_t ext_bits = extmsg->msgh_bits;
  const mcn_portid_t ext_local = extmsg->msgh_local;
  const mcn_portid_t ext_remote = extmsg->msgh_remote;
  const mcn_msgid_t ext_msgid = extmsg->msgh_msgid;

  const mcn_msgtype_name_t ext_rembits = MCN_MSGBITS_REMOTE(ext_bits);
  const mcn_msgtype_name_t ext_locbits = MCN_MSGBITS_LOCAL(ext_bits);

  struct portref remote_pref, local_pref;
  rc = portspace_resolve_sendmsg(ps, ext_rembits, ext_remote, &remote_pref,
				 ext_locbits, ext_local, &local_pref);
  if (rc)
    return rc;

  /* Swap remote and local. */
  intmsg->msgh_bits = MCN_MSGBITS(msgbits_sendrecv_intern(ext_locbits),
			       msgbits_sendrecv_intern(ext_rembits));
  intmsg->msgh_remote = portref_to_ipcport(&local_pref);
  intmsg->msgh_local = portref_to_ipcport(&remote_pref);
  intmsg->msgh_size = size;
  intmsg->msgh_seqno = 0;
  intmsg->msgh_msgid = ext_msgid;

  memcpy((void *)(intmsg + 1), (void *)(extmsg + 1), size - sizeof(mcn_msgheader_t));
  return MSGIO_SUCCESS;
}

mcn_msgioret_t
ipc_msg(mcn_msgopt_t opt, mcn_portid_t recv_port, unsigned long timeout, mcn_portid_t notify)
{
  mcn_msgioret_t rc;
  struct portspace *ps;

  const bool send = !!(opt & MCN_MSGOPT_SEND);
  const bool recv = !!(opt & MCN_MSGOPT_RECV);

  if (!send)
    goto _do_recv;

  {
    volatile mcn_msgheader_t *ext_msg = (volatile mcn_msgheader_t *)cur_kmsgbuf();
    const mcn_msgsize_t ext_size = ext_msg->msgh_size;

    if ((ext_size < sizeof(mcn_msgheader_t)) || (ext_size > MSGBUF_SIZE))
      return MSGIO_SEND_INVALID_DATA;

    message_debug((mcn_msgheader_t *)ext_msg);

    mcn_msgheader_t *int_msg = (mcn_msgheader_t *)kmem_alloc(0, ext_size);
    ps = task_getportspace(cur_task());
    rc = internalize(ps, ext_msg, int_msg, ext_size);
    task_putportspace(cur_task(), ps);
    if (rc)
      {
	kmem_free(0, (vaddr_t)int_msg, ext_size);
	return rc;
      }

    message_debug(int_msg);

    rc = port_enqueue(int_msg);
    if (rc)
      {
	intmsg_consume(int_msg);
	kmem_free(0, (vaddr_t)int_msg, ext_size);
	return rc;
      }
  }

 _do_recv:
  if (!recv)
    goto _do_exit;

  {
    struct portref recv_pref;
    mcn_msgheader_t *intmsg;

    ps = task_getportspace(cur_task());
    rc = portspace_resolve_receive(ps, recv_port, &recv_pref);
    if (rc)
      {
	task_putportspace(cur_task(), ps);
	return MSGIO_RCV_INVALID_NAME;
      }

    rc = port_dequeue(portref_unsafe_get(&recv_pref), &intmsg);
    if (rc)
      {
	task_putportspace(cur_task(), ps);
	return rc;
      }

    const mcn_msgsize_t size = intmsg->msgh_size;
    printf("Internal received %d bytes", size);
    message_debug(intmsg);
    externalize(ps, intmsg, (volatile mcn_msgheader_t *)cur_kmsgbuf(), size);
    message_debug((mcn_msgheader_t *)cur_kmsgbuf());
    task_putportspace(cur_task(), ps);

    intmsg_consume(intmsg);
    kmem_free(0, (vaddr_t)intmsg, size);

  }

 _do_exit:
  return rc;  
}
