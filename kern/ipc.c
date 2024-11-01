/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <alloca.h>
#include <machina/error.h>
#include <machina/message.h>

#include <ks.h>

mcn_return_t simple
(
	mcn_portid_t test
)
{
  info("SIMPLE MESSAGE RECEIVED (test: %ld)\n", test);
  return KERN_SUCCESS;
}

static int counter = 0;
mcn_return_t inc(mcn_portid_t test, long *new)
{
  *new = ++counter;
  return KERN_SUCCESS;
}

mcn_return_t add(mcn_portid_t test, long b, long *c)
{
  counter += b;
  *c = counter;
  return KERN_SUCCESS;
}

mcn_return_t mul(mcn_portid_t test, int b, long *c)
{
  counter *= b;
  *c = counter;
  return KERN_SUCCESS;
}

mcn_msgioret_t
ipc_msg(mcn_msgopt_t opt, mcn_portid_t recv_port, unsigned long timeout, mcn_portid_t notify)
{
  const bool send = !!(opt & MCN_MSGOPT_SEND);
  const bool recv = !!(opt & MCN_MSGOPT_RECV);
  struct portspace *ps;
  mcn_msgioret_t rc;

  if (!send)
    goto _do_recv;

  /* Copy header from user-accessible memory. */
  mcn_msgheader_t send_reqh = *(volatile mcn_msgheader_t *)cur_kmsgbuf();

  const mcn_portid_t send_remote_portid = send_reqh.msgh_remote;
  const mcn_portid_t send_local_portid = send_reqh.msgh_local;
  const mcn_msgsize_t send_size = send_reqh.msgh_size;
  struct portright send_right, reply_right;
  
  /*
    Check Size.

    Size is MSGBUF_SIZE - sizeof(mcn_seqno_t), because seqno is
    added to the message buffer when received, which is also
    MSGBUF_SIZE.
    Note: size should be exported to userspace.
  */
  if ((send_size < sizeof(mcn_msgheader_t)) || (send_size > MSGBUF_SIZE))
    return MSGIO_SEND_INVALID_DATA;

  ps = task_getportspace(cur_task());
  rc = portspace_resolve_sendmsg(ps,
				 MCN_MSGBITS_REMOTE(send_reqh.msgh_bits), send_remote_portid, &send_right,
				 MCN_MSGBITS_LOCAL(send_reqh.msgh_bits), send_local_portid, &reply_right);
  if (rc)
    {
      task_putportspace(cur_task(), ps);
      return rc;
    }

  struct port *p = portright_unsafe_get(&send_right);
  rc = port_enqueue(p, &send_reqh, &send_right, &reply_right, cur_kmsgbuf() + sizeof(mcn_msgheader_t), send_size - sizeof(mcn_msgheader_t));
  task_putportspace(cur_task(), ps);
  if (rc)
    return rc;
  rc = MSGIO_SUCCESS;

 _do_recv:
  if (!recv)
    goto _do_exit;

  struct portref recvref;

  ps = task_getportspace(cur_task());
  rc = portspace_resolve_receive(ps, recv_port, &recvref);
  if (rc)
    rc = MSGIO_RCV_INVALID_NAME;
  else
    {
      rc = port_dequeue(recv_port, REF_GET(recvref), ps, cur_kmsgbuf(), MSGBUF_SIZE);
    }
  task_putportspace(cur_task(), ps);
  portspace_print(&cur_task()->portspace);

 _do_exit:
  return rc;
}
