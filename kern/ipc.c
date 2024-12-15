/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef IPC_DEBUG
#define IPC_PRINT(...)
#else
#define IPC_PRINT printf
#endif

#include "internal.h"
#include <machina/error.h>
#include <machina/message.h>

static inline mcn_msgtype_name_t
msgbits_sendrecv_intern (mcn_msgtype_name_t type)
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
      fatal ("Wrong type %d\n", type);
      return -1;
    }
}

static inline mcn_msgtype_name_t
msgbits_port_intern (mcn_msgtype_name_t type)
{
  switch (type)
    {
    case 0:
      return 0;

    case MCN_MSGTYPE_MOVERECV:
      return MCN_MSGTYPE_PORTRECV;

    case MCN_MSGTYPE_MOVESEND:
    case MCN_MSGTYPE_COPYSEND:
    case MCN_MSGTYPE_MAKESEND:
      return MCN_MSGTYPE_PORTSEND;

    case MCN_MSGTYPE_MOVEONCE:
    case MCN_MSGTYPE_MAKEONCE:
      return MCN_MSGTYPE_PORTONCE;

    default:
      fatal ("Wrong type %d\n", type);
      return -1;
    }
}

static inline bool
msgbits_is_port (mcn_msgtype_name_t type)
{
  switch (type)
    {
    default:
    case 0:
      return false;

    case MCN_MSGTYPE_MOVERECV:
    case MCN_MSGTYPE_MOVESEND:
    case MCN_MSGTYPE_MOVEONCE:
    case MCN_MSGTYPE_COPYSEND:
    case MCN_MSGTYPE_MAKESEND:
    case MCN_MSGTYPE_MAKEONCE:
      return true;
    }
}

static void
internalize_portarray (struct ipcspace *ps, uint8_t name, void *array,
		       size_t itemsz, unsigned number)
{
  IPC_PRINT ("port array: item size: %ld, number: %ld\n", itemsz, number);
  if (itemsz != sizeof (mcn_portid_t))
    {
      /*
         If the type is too small, just set it to zero. This is invalid.
       */
      memset (array, 0, itemsz * number);
      return;
    }
  IPC_PRINT ("here!\n");

  mcn_portid_t *idptr = array;

  for (unsigned i = 0; i < number; i++, idptr++)
    {
      mcn_return_t rc;
      struct portref portref;
      ipcspace_debug (ps);
      IPC_PRINT ("RESOLVE OP: %s %" PRIx64 "\n", typename_debug (name), *idptr);
      rc = ipcspace_resolve (ps, name, *idptr, &portref);
      ipcspace_debug (ps);
      if (rc)
	{
	  *idptr = MCN_PORTID_NULL;
	  continue;
	}
      IPC_PRINT ("writing to idptr %p\n", idptr);
      *idptr = portref_to_ipcport (&portref);
      IPC_PRINT ("new idptr: %lx\n", *idptr);
    }
}

static void
externalize_portarray (struct ipcspace *ps, uint8_t name, void *array,
		       size_t itemsz, unsigned number)
{
  IPC_PRINT ("port array: item size: %ld, number: %ld\n", itemsz, number);
  assert (itemsz == sizeof (mcn_portid_t));

  mcn_portid_t *idptr = array;

  for (unsigned i = 0; i < number; i++, idptr++)
    {
      mcn_return_t rc;
      ipcspace_debug (ps);

      if (ipcport_isnull (*idptr))
	continue;

      struct portref portref = ipcport_to_portref (idptr);
      struct portright pr = portright_from_portref (name, portref);
      ipcspace_debug (ps);
      IPC_PRINT ("INSERT OP: %s %" PRIx64 "\n", typename_debug (name), *idptr);
      rc = ipcspace_insertright (ps, &pr, idptr);
      ipcspace_debug (ps);
      if (rc)
	{
	  *idptr = MCN_PORTID_NULL;
	  portright_consume (&pr);
	}
    }
}

static void
consume_portarray (void *array, size_t itemsz, unsigned number)
{
  IPC_PRINT ("consuming port array: item size: %ld, number: %ld\n", itemsz,
	  number);
  assert (itemsz == sizeof (mcn_portid_t));

  mcn_portid_t *idptr = array;

  for (unsigned i = 0; i < number; i++, idptr++)
    {
      if (ipcport_isnull (*idptr))
	continue;

      struct portref portref = ipcport_to_portref (idptr);
      portref_consume (&portref);
    }
}

enum msgitem_op
{
  MSGITEMOP_INTERNALIZE,
  MSGITEMOP_EXTERNALIZE,
  MSGITEMOP_CONSUME,
};

static bool
process_item (struct ipcspace *ps, void **from, void *end, enum msgitem_op op)
{
  mcn_msgtype_t *ty = *from;
  mcn_msgtype_long_t *longty = *from;
  assert (*from < end);

  /*
     Read the basic header.
   */
  if (((*from) + sizeof (mcn_msgtype_t)) > end)
    return false;

  /*
     Now we can read the msgtype header.
   */
  bool is_long = !!ty->msgt_longform;
  bool is_inline = !!ty->msgt_inline;

  /*
     Read the full header.
   */
  size_t hdr_size =
    is_long ? sizeof (mcn_msgtype_long_t) : sizeof (mcn_msgtype_t);

  if (((*from) + hdr_size) > end)
    return false;

  /*
     Now we can read the full header.
   */
  unsigned name = ty->msgt_longform ? longty->msgtl_name : ty->msgt_name;
  unsigned size = ty->msgt_longform ? longty->msgtl_size : ty->msgt_size;
  unsigned number =
    ty->msgt_longform ? longty->msgtl_number : ty->msgt_number;

  /*
     Align the header to the item size.

     MIG does this. It is architectural.
   */
  hdr_size += is_long ? 0 : ((size >> 3) == 8 ? 4 : 0);	/* Align. */

  if ((*from + hdr_size) > end)
    return false;

  /*
     We can read up to the item now.
   */

  /*
     Calculate full item size.
   */
  size_t item_size =
    (is_inline ? ((size >> 3) * number) : sizeof (mcn_vmaddr_t));

  bool is_port = msgbits_is_port (name)
    && ((size >> 3) == sizeof (mcn_portid_t));

  /*
     Note: If an IPC claims data contains a port but the item size is
     not correct, we ignore it.
   */

  if (((*from) + hdr_size + item_size) > end)
    return false;

  /*
     Now we can read the item.
   */
  if (is_port)
    {
      if (op == MSGITEMOP_INTERNALIZE)
	{
	  uint8_t newname = msgbits_port_intern (name);

	  if (ty->msgt_longform)
	    longty->msgtl_name = newname;
	  else
	    ty->msgt_name = newname;
	}

      /* XXX: ADD SUPPORT FOR NOT INLINE */
      assert (is_inline);
      if (is_inline)
	{
	  switch (op)
	    {
	    case MSGITEMOP_INTERNALIZE:
	      IPC_PRINT
		("internalizing portarray ps: %p name: %x ptr: %p size %d, number: %d\n",
		 ps, name, (*from) + hdr_size, size >> 3, number);
	      assert (ps != NULL);
	      internalize_portarray (ps, name, (*from) + hdr_size, size >> 3,
				     number);
	      break;
	    case MSGITEMOP_EXTERNALIZE:
	      assert (ps != NULL);
	      externalize_portarray (ps, name, (*from) + hdr_size, size >> 3,
				     number);
	      break;
	    case MSGITEMOP_CONSUME:
	      assert (ps == NULL);
	      consume_portarray ((*from) + hdr_size, size >> 3, number);
	      break;
	    }
	}
    }
  *from += hdr_size + item_size;
  return true;
}

static void
process_body (struct ipcspace *ps, void *body, size_t size,
	      enum msgitem_op op)
{
  void *ptr = body;
  void *end = body + size;

  IPC_PRINT ("ptr %p end %p\n", ptr, end);
  while ((ptr < end) && process_item (ps, &ptr, end, op))
    IPC_PRINT ("ptr %p end %p\n", ptr, end);
}

void
ipc_intmsg_consume (mcn_msgheader_t * intmsg)
{
  assert (intmsg->msgh_size >= sizeof (mcn_msgheader_t));
  IPC_PRINT ("CONSUMING MESSAGE %p\n", intmsg);

  if (intmsg->msgh_remote != 0)
    {
      IPC_PRINT ("CONSUMING REMOTE PORT %lx\n", intmsg->msgh_remote);
      struct portref port = ipcport_to_portref (&intmsg->msgh_remote);
      portref_consume (&port);
    }
  if (intmsg->msgh_local != 0)
    {
      IPC_PRINT ("CONSUMING LOCAL PORT %lx\n", intmsg->msgh_local);
      struct portref port = ipcport_to_portref (&intmsg->msgh_local);
      portref_consume (&port);
    }

  if (intmsg->msgh_bits & MCN_MSGBITS_COMPLEX)
    {
      process_body (NULL, (void *) (intmsg + 1),
		    intmsg->msgh_size - sizeof (mcn_msgheader_t),
		    MSGITEMOP_CONSUME);
    }

}

static mcn_msgioret_t
externalize (struct ipcspace *ps, mcn_msgheader_t * intmsg,
	     volatile mcn_msgheader_t * extmsg, size_t size)
{
  mcn_msgioret_t rc;
  mcn_portid_t local, remote;

  assert (size >= sizeof (mcn_msgheader_t));
  assert (size <= MSGBUF_SIZE);

  struct portref local_pref = ipcport_to_portref (&intmsg->msgh_local);
  local = ipcspace_lookup (ps, portref_unsafe_get (&local_pref));
  portref_consume (&local_pref);

  remote = MCN_PORTID_NULL;
  if (intmsg->msgh_remote != 0)
    {
      const mcn_msgtype_name_t rembits =
	MCN_MSGBITS_REMOTE (intmsg->msgh_bits);
      struct portref portref = ipcport_to_portref (&intmsg->msgh_remote);
      struct portright pr = portright_from_portref (rembits, portref);
      rc = ipcspace_insertright (ps, &pr, &remote);
      if (rc)
	{
	  remote = MCN_PORTID_NULL;
	  portright_consume (&pr);
	}
    }

  if (intmsg->msgh_bits & MCN_MSGBITS_COMPLEX)
    {
      process_body (ps, (void *) (intmsg + 1),
		    size - sizeof (mcn_msgheader_t), MSGITEMOP_EXTERNALIZE);
    }


  extmsg->msgh_bits = intmsg->msgh_bits;
  extmsg->msgh_remote = remote;
  extmsg->msgh_local = local;
  extmsg->msgh_size = size;
  extmsg->msgh_seqno = 0;	/* XXX: SEQNO */
  extmsg->msgh_msgid = intmsg->msgh_msgid;

  memcpy ((void *) (extmsg + 1), (void *) (intmsg + 1),
	  size - sizeof (mcn_msgheader_t));

  return MSGIO_SUCCESS;
}



static mcn_msgioret_t
internalize (struct ipcspace *ps, volatile mcn_msgheader_t * extmsg,
	     mcn_msgheader_t * intmsg, size_t size)
{
  mcn_msgioret_t rc;

  assert (size >= sizeof (mcn_msgheader_t));
  assert (size <= MSGBUF_SIZE);

  const mcn_msgbits_t ext_bits = extmsg->msgh_bits;
  const mcn_portid_t ext_local = extmsg->msgh_local;
  const mcn_portid_t ext_remote = extmsg->msgh_remote;
  const mcn_msgid_t ext_msgid = extmsg->msgh_msgid;

  const mcn_msgtype_name_t ext_rembits = MCN_MSGBITS_REMOTE (ext_bits);
  const mcn_msgtype_name_t ext_locbits = MCN_MSGBITS_LOCAL (ext_bits);

  struct portref remote_pref, local_pref;
  rc = ipcspace_resolve_sendmsg (ps, ext_rembits, ext_remote, &remote_pref,
				 ext_locbits, ext_local, &local_pref);
  if (rc)
    return rc;

  /* Swap remote and local. */
  intmsg->msgh_bits =
    ext_bits & MCN_MSGBITS_COMPLEX ? MCN_MSGBITS_COMPLEX : 0;
  intmsg->msgh_bits |=
    MCN_MSGBITS (msgbits_sendrecv_intern (ext_locbits),
		 msgbits_sendrecv_intern (ext_rembits));
  intmsg->msgh_remote = portref_to_ipcport (&local_pref);
  intmsg->msgh_local = portref_to_ipcport (&remote_pref);
  intmsg->msgh_size = size;
  intmsg->msgh_seqno = 0;
  intmsg->msgh_msgid = ext_msgid;

  memcpy ((void *) (intmsg + 1), (void *) (extmsg + 1),
	  size - sizeof (mcn_msgheader_t));

  if (intmsg->msgh_bits & MCN_MSGBITS_COMPLEX)
    {
      process_body (ps, (void *) (intmsg + 1),
		    size - sizeof (mcn_msgheader_t), MSGITEMOP_INTERNALIZE);
    }

  return MSGIO_SUCCESS;
}

mcn_return_t
mcn_msg_send_from_kernel (mcn_msgheader_t * hdr, mcn_msgsize_t size)
{
  mcn_return_t rc;
  mcn_msgheader_t *int_msg;

  message_debug (hdr);
  int_msg = (mcn_msgheader_t *) kmem_alloc (0, size);
  memcpy (int_msg, hdr, size);
  rc = port_enqueue (int_msg, 0, true);
  if (rc)
    {
      ipc_intmsg_consume (int_msg);
      kmem_free (0, (vaddr_t) int_msg, size);
    }
  return rc;
}

mcn_msgioret_t
ipc_msgsend (mcn_msgopt_t opt, unsigned long timeout, mcn_portid_t notify)
{
  mcn_msgioret_t rc;
  struct ipcspace *ps;

  volatile mcn_msgheader_t *ext_msg =
    (volatile mcn_msgheader_t *) cur_kmsgbuf ();
  const mcn_msgsize_t ext_size = ext_msg->msgh_size;

  if ((ext_size < sizeof (mcn_msgheader_t)) || (ext_size > MSGBUF_SIZE))
    return MSGIO_SEND_INVALID_DATA;

  message_debug ((mcn_msgheader_t *) ext_msg);

  mcn_msgheader_t *int_msg = (mcn_msgheader_t *) kmem_alloc (0, ext_size);
  ps = task_getipcspace (cur_task ());
  rc = internalize (ps, ext_msg, int_msg, ext_size);
  task_putipcspace (cur_task (), ps);
  if (rc)
    {
      kmem_free (0, (vaddr_t) int_msg, ext_size);
      return rc;
    }

  message_debug (int_msg);

  rc = port_enqueue (int_msg, timeout, false);
  if (rc)
    {
      ipc_intmsg_consume (int_msg);
      kmem_free (0, (vaddr_t) int_msg, ext_size);
      return rc;
    }

  return MSGIO_SUCCESS;
}

mcn_msgioret_t
ipc_msgrecv (mcn_portid_t recv_port, mcn_msgopt_t opt, unsigned long timeout,
	     mcn_portid_t notify)
{
  mcn_msgioret_t rc;
  struct ipcspace *ps;
  struct portref recv_pref;
  mcn_msgheader_t *intmsg;

  ps = task_getipcspace (cur_task ());
  rc = ipcspace_resolve_receive (ps, recv_port, &recv_pref);
  if (rc)
    {
      task_putipcspace (cur_task (), ps);
      return MSGIO_RCV_INVALID_NAME;
    }

  rc = port_dequeue (portref_unsafe_get (&recv_pref), timeout, &intmsg);
  portref_consume (&recv_pref);
  if (rc)
    {
      task_putipcspace (cur_task (), ps);
      return rc;
    }

  const mcn_msgsize_t size = intmsg->msgh_size;
  IPC_PRINT ("Internal received %d bytes\n", size);
  message_debug (intmsg);
  externalize (ps, intmsg, (volatile mcn_msgheader_t *) cur_kmsgbuf (), size);
  message_debug ((mcn_msgheader_t *) cur_kmsgbuf ());
  task_putipcspace (cur_task (), ps);

  /*
     References to local and remote should have been cleared.
   */
  assert (intmsg->msgh_remote == 0);
  assert (intmsg->msgh_local == 0);

  kmem_free (0, (vaddr_t) intmsg, size);
  return MSGIO_SUCCESS;
}


void
__ipc_build_assert (void)
{
  BUILD_ASSERT (sizeof (mcn_msgtype_t) == MCN_MSGTYPE_SIZE);
  BUILD_ASSERT (sizeof (mcn_msgtype_long_t) == MCN_MSGTYPE_LONG_SIZE);
#ifdef __LP64__
  BUILD_ASSERT (sizeof (mcn_msgheader_t) == MCN_MSGHEADER_SIZE_64);
#else
  BUILD_ASSERT (sizeof (mcn_msgheader_t) == MCN_MSGHEADER_SIZE_32);
#endif
}
