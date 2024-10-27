/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
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

#if 0
static inline mcn_return_t
port_send(struct portref *portref, mcn_msgid_t id, void *data, size_t size, struct portref *reply)
{
  mcn_return_t rc;
  struct port *p  = REF_GET(*portref);

  /* XXX: TMP */assert(p->type == PORT_KERNEL);
  switch (p->type)
    {
    case PORT_KERNEL:
      rc = p->kernel.msgsend(p->kernel.ctx, id, data, size, reply);
      break;
    case PORT_MESSAGE:
      fatal ("Unsupported type");
    }

  return rc;
}
#endif


static inline bool
portright_iskernel(struct portright *pr)
{
  return port_kernel(REF_GET(pr->portref));
}

mcn_msgioret_t ipc_msgio(mcn_msgopt_t opt, mcn_portid_t recv_port, unsigned long timeout, mcn_portid_t notify)
{
  const bool send = !!(opt & MCN_MSGOPT_SEND);
  //  const bool recv = !!(opt & MCN_MSGOPT_RECV);
  mcn_return_t rc;
  struct portspace *ps;

  /* Copy from user-accessible memory. */
  kstest_requests_t req = *(volatile kstest_requests_t *)cur_kmsgbuf();
  mcn_msgsend_t *reqh = (mcn_msgsend_t *)&req;

  if (send)
    {
      const bool reply_port_valid = (reqh->msgs_local != MCN_PORTID_NULL);
      struct portright send_right = {0}, reply_right = {0};
  
      if (reqh->msgs_size < sizeof(mcn_msgsend_t))
	return MSGIO_SEND_INVALID_DATA;
      if (reqh->msgs_size >= sizeof(mcn_msgsend_t))
	return MSGIO_SEND_INVALID_DATA;

      ps = task_getportspace(cur_task());

      switch (MCN_MSGBITS_REMOTE(reqh->msgs_bits))
	{
	case MCN_MSGTYPE_MOVESEND:
	  rc = portspace_movesend(ps, reqh->msgs_remote, &send_right);
	  if (rc)
	    goto error_remote_port;
	  break;
	case MCN_MSGTYPE_COPYSEND:
	  rc = portspace_copysend(ps, reqh->msgs_remote, &send_right);
	  if (rc)
	    goto error_remote_port;
	  break;
	case MCN_MSGTYPE_MOVEONCE:
	  rc = portspace_moveonce(ps, reqh->msgs_remote, &send_right);
	  if (rc)
	    goto error_remote_port;
	  break;
	case MCN_MSGTYPE_MAKESEND:
	  rc = portspace_makesend(ps, reqh->msgs_remote, &send_right);
	  if (rc)
	    goto error_remote_port;
	  break;
	case MCN_MSGTYPE_MAKEONCE:
	  rc = portspace_makeonce(ps, reqh->msgs_remote, &send_right);
	  if (rc)
	    goto error_remote_port;
	  break;
	default:
	  rc = MSGIO_SEND_INVALID_HEADER;
	  goto exit_remote_port;
	error_remote_port:
	  rc = MSGIO_SEND_INVALID_DEST;
	  goto exit_remote_port;
	exit_remote_port:
	  task_putportspace(cur_task(), ps);
	  return rc;
	}

      if (reply_port_valid)
	{

	  switch (MCN_MSGBITS_LOCAL(reqh->msgs_bits))
	    {
	      /*
		In Machina, you can only reply to a make-once port.
	      */
	    case MCN_MSGTYPE_MOVEONCE:
	      rc = portspace_moveonce(ps, reqh->msgs_local, &reply_right);
	      if (rc)
		goto error_reply_port;
	      break;
	    case MCN_MSGTYPE_MAKEONCE:
	      rc = portspace_makeonce(ps, reqh->msgs_local, &reply_right);
	      if (rc)
		goto error_reply_port;
	      break;
	    default:
	      rc = MSGIO_SEND_INVALID_HEADER;
	      goto exit_reply_port;
	    error_reply_port:
	      rc = MSGIO_SEND_INVALID_REPLY;
	      goto exit_reply_port;
	    exit_reply_port:
	      /* Restore send right. */
	      switch (MCN_MSGBITS_REMOTE(reqh->msgs_bits))
		{
		case MCN_MSGTYPE_MOVESEND:
		case MCN_MSGTYPE_MOVEONCE:
		  warn("GIANLUCA: READD THIS");
		  //		  assert(portspace_addsendright(ps, reqh->msgs_remote, &send_right) == KERN_SUCCESS);
		  break;
		case MCN_MSGTYPE_COPYSEND:
		case MCN_MSGTYPE_MAKESEND:
		case MCN_MSGTYPE_MAKEONCE:
		  portright_consume(&send_right);
		  break;
		}
	      task_putportspace(cur_task(), ps);
	      return rc;
	    }
	}
        task_putportspace(cur_task(), ps);

	
	if (portright_iskernel(&send_right))
	  {
  struct port *send;/*, *reply; */
  /*
    This is a Kernel Server IPC.
  */

  if (reply_port_valid && (reqh->msgs_local == recv_port))
    {
      /*
	Caller waits on the same receive port it expects a
	reply from.  Write the reply directly on the thread
	message buffer and return.

	This catches the default behaviour for Kernel Server
	interfaces in MIG.
      */

      send = portright_unsafe_get(&send_right);
      kstest_server(send, (mcn_msgsend_t *)&req, (mcn_msgrecv_t *)cur_kmsgbuf());
      portright_consume(&send_right);
      portright_consume(&reply_right);
      return MSGIO_SUCCESS;
    }
  else if (!reply_port_valid)
    {
      /* Ignore reply. */
      kstest_replies_t reply;

      send = portright_unsafe_get(&send_right);
      kstest_server(send, (mcn_msgsend_t *)&req, (mcn_msgrecv_t *)&reply);
      portright_consume(&send_right);
      /* reply_right NOT VALID. DO NOT CONSUME. */
      return MSGIO_SUCCESS;
    }
  else
    {
      struct port *p;
      void *b;
      size_t size;
      kstest_replies_t reply;

      /*
	Kernel IPC reply has to be queued in a port.
      */
      assert(reply_port_valid);
      send = portright_unsafe_get(&send_right);
      kstest_server(send, (mcn_msgsend_t *)&req, (mcn_msgrecv_t *)&reply);
      portright_consume(&send_right);

      size = ((mcn_msgrecv_t *)&reply)->msgr_size;
      if (size > sizeof(reply))
	{
	  fatal ("Kernel IPC produced a suspicious size %ld\n",
		 (unsigned long)size);
	}
      size += sizeof(struct msgq_entry);
      b = (void *)kmem_alloc(0, size);
      if (b == NULL)
	return KERN_RESOURCE_SHORTAGE;

      memcpy(b + sizeof(struct msgq_entry), &reply, size);
      p = portright_unsafe_get(&reply_right);
      port_enqueue(p, (struct msgq_entry *)b, size);
      portright_unsafe_put(&p);
      portright_consume(&reply_right);
    }
	  }
    }

    
  
  fatal("Implement me");
  
  //      rc = port_send(send_right.portref, reqh->msgs_msgid, ((struct mcn_msgsend *)cur_kmsgbuf())+1, reqh->msgs_size, reply_right.portref);
  return MSGIO_SUCCESS;
}
