/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"
#include <machina/error.h>
#include <machina/message.h>

mcn_return_t ipc_msgio(mcn_msgopt_t opt, mcn_portid_t recv, unsigned long timeout, mcn_portid_t notify)
{
  mcn_return_t rc;
  const bool send = !!(opt & MCN_MSGOPT_SEND);

  printf("send ? %d\n", send);

  if (send)
    {
      struct mcn_msgsend msgh;
      struct portspace *ps;
      struct sendright send_right, reply_right;

      /* Copy from user-accessible memory. */
      msgh = *(struct mcn_msgsend *)cur_kmsgbuf();

      /* Ensure atomicity in managin port right. */
      ps = task_getportspace(cur_task());
      printf("flags: %d\n", MCN_MSGFLAG_REMOTE(msgh.msgs_flag));
      switch (MCN_MSGFLAG_REMOTE(msgh.msgs_flag))
	{
	case MCN_MSGFLAG_REMOTE_MOVESEND:
	  rc = portspace_movesend(ps, msgh.msgs_remote, &send_right);
	  if (rc)
	    goto error_remote_port;
	  break;
	case MCN_MSGFLAG_REMOTE_COPYSEND:
	  rc = portspace_copysend(ps, msgh.msgs_remote, &send_right);
	  printf("[REM: %ld]RC: %d", msgh.msgs_remote, rc);
	  if (rc)
	    goto error_remote_port;
	  break;
	case MCN_MSGFLAG_REMOTE_MOVEONCE:
	  rc = portspace_moveonce(ps, msgh.msgs_remote, &send_right);
	  if (rc)
	    goto error_remote_port;
	  break;
	case MCN_MSGFLAG_REMOTE_MAKESEND:
	  rc = portspace_makesend(ps, msgh.msgs_remote, &send_right);
	  if (rc)
	    goto error_remote_port;
	  break;
	case MCN_MSGFLAG_REMOTE_MAKEONCE:
	  rc = portspace_makeonce(ps, msgh.msgs_remote, &send_right);
	  if (rc)
	    goto error_remote_port;
	  break;
	default:
	  rc = KERN_PORT_INVALID;
	error_remote_port:
	  task_putportspace(cur_task(), ps);
	  return rc;
	}

      printf("REPLY IS %d\n", MCN_MSGFLAG_LOCAL(msgh.msgs_flag));
      switch (MCN_MSGFLAG_LOCAL(msgh.msgs_flag))
	{
	case MCN_MSGFLAG_LOCAL_MOVESEND:
	  rc = portspace_movesend(ps, msgh.msgs_local, &reply_right);
	  if (rc)
	    goto error_reply_port;
	  break;
	case MCN_MSGFLAG_LOCAL_COPYSEND:
	  rc = portspace_copysend(ps, msgh.msgs_local, &reply_right);
	  if (rc)
	    goto error_reply_port;
	  break;
	case MCN_MSGFLAG_LOCAL_MOVEONCE:
	  rc = portspace_moveonce(ps, msgh.msgs_local, &reply_right);
	  if (rc)
	    goto error_reply_port;
	  break;
	case MCN_MSGFLAG_LOCAL_MAKESEND:
	  rc = portspace_makesend(ps, msgh.msgs_local, &reply_right);
	  if (rc)
	    goto error_reply_port;
	  break;
	case MCN_MSGFLAG_LOCAL_MAKEONCE:
	  rc = portspace_makeonce(ps, msgh.msgs_local, &reply_right);
	  if (rc)
	    goto error_reply_port;
	  break;
	case MCN_MSGFLAG_LOCAL_NONE:
	  /* XXX: CASE WHEN REPLY IS NONE. */
	  break;
	default:
	  rc = KERN_PORT_INVALID;
	error_reply_port:
	  printf("ERROR REPLY PORT!");
	  /* Restore send right. */
	  switch (MCN_MSGFLAG_REMOTE(msgh.msgs_flag))
	    {
	    case MCN_MSGFLAG_REMOTE_MOVESEND:
	    case MCN_MSGFLAG_REMOTE_MOVEONCE:
	      assert(portspace_addsendright(ps, msgh.msgs_remote, &send_right) == KERN_SUCCESS);
	      break;
	    case MCN_MSGFLAG_REMOTE_COPYSEND:
	    case MCN_MSGFLAG_REMOTE_MAKESEND:
	    case MCN_MSGFLAG_REMOTE_MAKEONCE:
	      sendright_destroy(&send_right);
	      break;
	    }
	  task_putportspace(cur_task(), ps);
	  return rc;
	}
      task_putportspace(cur_task(), ps);
      printf("HERE!\n");
      rc = port_send(send_right.portref, msgh.msgs_msgid, ((struct mcn_msgsend *)cur_kmsgbuf())+1, msgh.msgs_size, reply_right.portref);
    }

  return KERN_SUCCESS;
}
