/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log:	utils.c,v $
 * Revision 2.5  91/08/29  15:47:00  rpd
 * 	Changed my_mach_port_rename to check for equal names.
 * 	[91/08/15            rpd]
 * 
 * 	Updated for sequence numbers.
 * 	[91/08/11            rpd]
 * 
 * Revision 2.4  91/03/27  17:23:57  mrt
 * 	Changed mach.h include
 * 
 * Revision 2.3  91/03/19  12:18:27  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.2  90/09/12  16:31:09  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#include <mach.h>
#include <mach_error.h>
#include <mach/notify.h>
#include <mach/mig_errors.h>
#include <stdio.h>
#include "main.h"
#include "utils.h"

mach_msg_type_name_t
copyin_type(msgt_name)
	mach_msg_type_name_t msgt_name;
{
    switch (msgt_name)
    {
      case MACH_MSG_TYPE_MOVE_RECEIVE:
	return MACH_MSG_TYPE_PORT_RECEIVE;

      case MACH_MSG_TYPE_MOVE_SEND_ONCE:
      case MACH_MSG_TYPE_MAKE_SEND_ONCE:
	return MACH_MSG_TYPE_PORT_SEND_ONCE;

      case MACH_MSG_TYPE_MOVE_SEND:
      case MACH_MSG_TYPE_MAKE_SEND:
      case MACH_MSG_TYPE_COPY_SEND:
	return MACH_MSG_TYPE_PORT_SEND;

      default:
	return 0;
    }
}

void
fillin_msg_header(msg, complex, rtype, ltype, remote, local, seqno, id)
    mach_msg_header_t *msg;
    boolean_t complex;
    mach_msg_type_name_t rtype, ltype;
    mach_port_t remote, local;
    mach_port_seqno_t seqno;
    int id;
{
    msg->msgh_bits = ((complex ? MACH_MSGH_BITS_COMPLEX : 0) |
		      MACH_MSGH_BITS(rtype, ltype));
    msg->msgh_size = 0;
    msg->msgh_remote_port = remote;
    msg->msgh_local_port = local;
    msg->msgh_seqno = seqno;
    msg->msgh_id = id;
}

void
fillin_msg_header_all(msg, bits, size, remote, local, seqno, id)
    mach_msg_header_t *msg;
    mach_msg_bits_t bits;
    mach_msg_size_t size;
    mach_port_seqno_t seqno;
    mach_port_t local, remote;
    int id;
{
    msg->msgh_bits = bits;
    msg->msgh_size = size;
    msg->msgh_remote_port = remote;
    msg->msgh_local_port = local;
    msg->msgh_seqno = seqno;
    msg->msgh_id = id;
}

void
check_msg_header(msg, complex, rtype, ltype, size, remote, local, seqno, id)
    mach_msg_header_t *msg;
    boolean_t complex;
    mach_msg_type_name_t rtype, ltype;
    mach_msg_size_t size;
    mach_port_t remote, local;
    mach_port_seqno_t seqno;
    int id;
{
    mach_msg_bits_t bits;

    bits = ((complex ? MACH_MSGH_BITS_COMPLEX : 0) |
	    MACH_MSGH_BITS(rtype, ltype));

    if (msg->msgh_bits != bits)
	quit(1, "%s: check_msg_header: msgh_bits = %08x\n",
	     program, msg->msgh_bits);

    if (msg->msgh_size != size)
	quit(1, "%s: check_msg_header: msgh_size = %d\n",
	     program, msg->msgh_size);

    if (msg->msgh_remote_port != remote)
	quit(1, "%s: check_msg_header: msgh_remote_port = %d\n",
	     program, msg->msgh_remote_port);

    if (msg->msgh_local_port != local)
	quit(1, "%s: check_msg_header: msgh_local_port = %d\n",
	     program, msg->msgh_local_port);

    if (msg->msgh_seqno != seqno)
	quit(1, "%s: check_msg_header: msgh_seqno = %d\n",
	     program, msg->msgh_seqno);

    if (msg->msgh_id != id)
	quit(1, "%s: check_msg_header: msgh_id = %d\n",
	     program, msg->msgh_id);
}

void
fillin_msg_type(type, name, size, number, inlinet, deallocate)
    mach_msg_type_t *type;
    unsigned int name;
    unsigned int size;
    unsigned int number;
    boolean_t inlinet;
    boolean_t deallocate;
{
    type->msgt_name = name;
    type->msgt_size = size;
    type->msgt_number = number;
    type->msgt_inline = inlinet;
    type->msgt_longform = FALSE;
    type->msgt_deallocate = deallocate;
    type->msgt_unused = 0;
}

void
fillin_msg_type_all(type, name, size, number, inlinet,
		    longform, deallocate, unused)
    mach_msg_type_t *type;
    unsigned int name;
    unsigned int size;
    unsigned int number;
    boolean_t inlinet;
    boolean_t longform;
    boolean_t deallocate;
    unsigned int unused;
{
    type->msgt_name = name;
    type->msgt_size = size;
    type->msgt_number = number;
    type->msgt_inline = inlinet;
    type->msgt_longform = longform;
    type->msgt_deallocate = deallocate;
    type->msgt_unused = unused;
}

void
check_msg_type(type, name, size, number, inlinet, deallocate)
    mach_msg_type_t *type;
    unsigned int name;
    unsigned int size;
    unsigned int number;
    boolean_t inlinet;
    boolean_t deallocate;
{
    if (type->msgt_name != name)
	quit(1, "%s: check_msg_type: msgt_name = %d\n",
	     program, type->msgt_name);

    if (type->msgt_size != size)
	quit(1, "%s: check_msg_type: msgt_size = %d\n",
	     program, type->msgt_size);

    if (type->msgt_number != number)
	quit(1, "%s: check_msg_type: msgt_number = %d\n",
	     program, type->msgt_number);

    if (type->msgt_inline != inlinet)
	quit(1, "%s: check_msg_type: msgt_inline = %d\n",
	     program, type->msgt_inline);

    if (type->msgt_longform != FALSE)
	quit(1, "%s: check_msg_type: msgt_longform = %d\n",
	     program, type->msgt_longform);

    if (type->msgt_deallocate != deallocate)
	quit(1, "%s: check_msg_type: msgt_deallocate = %d\n",
	     program, type->msgt_deallocate);

    if (type->msgt_unused != 0)
	quit(1, "%s: check_msg_type: msgt_unused = %d\n",
	     program, type->msgt_unused);
}

void
fillin_msg_type_long(type, inlinet, deallocate, name, size, number)
    mach_msg_type_long_t *type;
    boolean_t inlinet;
    boolean_t deallocate;
    unsigned int name;
    unsigned int size;
    unsigned int number;
{
    type->msgtl_header.msgt_name = 0;
    type->msgtl_header.msgt_size = 0;
    type->msgtl_header.msgt_number = 0;
    type->msgtl_header.msgt_inline = inlinet;
    type->msgtl_header.msgt_longform = TRUE;
    type->msgtl_header.msgt_deallocate = deallocate;
    type->msgtl_header.msgt_unused = 0;
    type->msgtl_name = name;
    type->msgtl_size = size;
    type->msgtl_number = number;
}

void
fillin_msg_type_long_all(type, name, size, number, inlinet,
			 longform, deallocate, unused,
			 lname, lsize, lnumber)
    mach_msg_type_long_t *type;
    unsigned int name;
    unsigned int size;
    unsigned int number;
    boolean_t inlinet;
    boolean_t longform;
    boolean_t deallocate;
    unsigned int unused;
    unsigned int lname;
    unsigned int lsize;
    unsigned int lnumber;
{
    type->msgtl_header.msgt_name = name;
    type->msgtl_header.msgt_size = size;
    type->msgtl_header.msgt_number = number;
    type->msgtl_header.msgt_inline = inlinet;
    type->msgtl_header.msgt_longform = longform;
    type->msgtl_header.msgt_deallocate = deallocate;
    type->msgtl_header.msgt_unused = unused;
    type->msgtl_name = lname;
    type->msgtl_size = lsize;
    type->msgtl_number = lnumber;
}

void
check_msg_type_long(type, inlinet, deallocate, name, size, number)
    mach_msg_type_long_t *type;
    boolean_t inlinet;
    boolean_t deallocate;
    unsigned int name;
    unsigned int size;
    unsigned int number;
{
    if (type->msgtl_header.msgt_name != 0)
	quit(1, "%s: check_msg_type_long: msgt_name = %d\n",
	     program, type->msgtl_header.msgt_name);

    if (type->msgtl_header.msgt_size != 0)
	quit(1, "%s: check_msg_type_long: msgt_size = %d\n",
	     program, type->msgtl_header.msgt_size);

    if (type->msgtl_header.msgt_number != 0)
	quit(1, "%s: check_msg_type_long: msgt_number = %d\n",
	     program, type->msgtl_header.msgt_number);

    if (type->msgtl_header.msgt_inline != inlinet)
	quit(1, "%s: check_msg_type_long: msgt_inline = %d\n",
	     program, type->msgtl_header.msgt_inline);

    if (type->msgtl_header.msgt_longform != TRUE)
	quit(1, "%s: check_msg_type_long: msgt_longform = %d\n",
	     program, type->msgtl_header.msgt_longform);

    if (type->msgtl_header.msgt_deallocate != deallocate)
	quit(1, "%s: check_msg_type_long: msgt_deallocate = %d\n",
	     program, type->msgtl_header.msgt_deallocate);

    if (type->msgtl_header.msgt_unused != 0)
	quit(1, "%s: check_msg_type_long: msgt_unused = %d\n",
	     program, type->msgtl_header.msgt_unused);

    if (type->msgtl_name != name)
	quit(1, "%s: check_msg_type_long: msgt_name = %d\n",
	     program, type->msgtl_name);

    if (type->msgtl_size != size)
	quit(1, "%s: check_msg_type_long: msgt_size = %d\n",
	     program, type->msgtl_size);

    if (type->msgtl_number != number)
	quit(1, "%s: check_msg_type_long: msgt_number = %d\n",
	     program, type->msgtl_number);
}

void
fillin_simple_header(msg, remote, local)
    mach_msg_header_t *msg;
    mach_port_t remote, local;
{
    fillin_msg_header(msg, FALSE,
		      MACH_MSG_TYPE_COPY_SEND,
		      (local == MACH_PORT_NULL) ? 0 : MACH_MSG_TYPE_MAKE_SEND_ONCE,
		      remote, local, 0, 0);
}

void
check_simple_header(msg, size, remote, local, seqno)
    mach_msg_header_t *msg;
    mach_msg_size_t size;
    mach_port_t remote, local;
    mach_port_seqno_t seqno;
{
    check_msg_header(msg, FALSE,
		     (remote == MACH_PORT_NULL) ? 0 : MACH_MSG_TYPE_PORT_SEND_ONCE,
		     MACH_MSG_TYPE_PORT_SEND,
		     size, remote, local, seqno, 0);
}

void
fillin_complex_header(msg, remote, local)
    mach_msg_header_t *msg;
    mach_port_t remote, local;
{
    fillin_msg_header(msg, TRUE,
		      MACH_MSG_TYPE_COPY_SEND,
		      (local == MACH_PORT_NULL) ? 0 : MACH_MSG_TYPE_MAKE_SEND_ONCE,
		      remote, local, 0, 0);
}

void
check_complex_header(msg, size, remote, local, seqno)
    mach_msg_header_t *msg;
    mach_msg_size_t size;
    mach_port_t remote, local;
    mach_port_seqno_t seqno;
{
    check_msg_header(msg, TRUE,
		     (remote == MACH_PORT_NULL) ? 0 : MACH_MSG_TYPE_PORT_SEND_ONCE,
		     MACH_MSG_TYPE_PORT_SEND,
		     size, remote, local, seqno, 0);
}

void
fillin_msg_type_integer(type)
    mach_msg_type_t *type;
{
    fillin_msg_type(type, MACH_MSG_TYPE_INTEGER_32, 32, 1, TRUE, FALSE);
}

void
check_msg_type_integer(type)
    mach_msg_type_t *type;
{
    check_msg_type(type, MACH_MSG_TYPE_INTEGER_32, 32, 1, TRUE, FALSE);
}

void
fillin_msg_type_port(type)
    mach_msg_type_t *type;
{
    fillin_msg_type(type, MACH_MSG_TYPE_COPY_SEND, 32, 1, TRUE, FALSE);
}

void
check_msg_type_port(type)
    mach_msg_type_t *type;
{
    check_msg_type(type, MACH_MSG_TYPE_PORT_SEND, 32, 1, TRUE, FALSE);
}

void
fillin_msg_type_dummy_pointer(type)
    mach_msg_type_t *type;
{
    fillin_msg_type(type, MACH_MSG_TYPE_INTEGER_32, 32, 0, FALSE, FALSE);
}

void
check_msg_type_dummy_pointer(type)
    mach_msg_type_t *type;
{
    check_msg_type(type, MACH_MSG_TYPE_INTEGER_32, 32, 0, FALSE, TRUE);
}

void
fillin_msg_type_dummy_ports(type)
    mach_msg_type_t *type;
{
    fillin_msg_type(type, MACH_MSG_TYPE_COPY_SEND, 32, 0, FALSE, FALSE);
}

void
check_msg_type_dummy_ports(type)
    mach_msg_type_t *type;
{
    check_msg_type(type, MACH_MSG_TYPE_PORT_SEND, 32, 0, FALSE, TRUE);
}

void
fillin_msg_type_port_name(type)
    mach_msg_type_t *type;
{
    fillin_msg_type(type, MACH_MSG_TYPE_PORT_NAME, 32, 1, TRUE, FALSE);
}

void
check_msg_type_port_name(type)
    mach_msg_type_t *type;
{
    check_msg_type(type, MACH_MSG_TYPE_PORT_NAME, 32, 1, TRUE, FALSE);
}

void
fillin_msg_type_page_dealloc(type)
    mach_msg_type_long_t *type;
{
    fillin_msg_type_long(type, FALSE, TRUE,
			 MACH_MSG_TYPE_BYTE, 8, vm_page_size);
}

void
check_msg_type_page(type)
    mach_msg_type_long_t *type;
{
    check_msg_type_long(type, FALSE, TRUE,
			MACH_MSG_TYPE_BYTE, 8, vm_page_size);
}

void
do_mach_msg_send(msg, option, size, timeout, notify, result)
    mach_msg_header_t *msg;
    mach_msg_option_t option;
    mach_msg_size_t size;
    mach_msg_timeout_t timeout;
    mach_port_t notify;
    mach_msg_return_t result;
{
    mach_msg_return_t kr;

    kr = mach_msg(msg, MACH_SEND_MSG|option,
		  size, 0, MACH_PORT_NULL,
		  timeout, notify);
    if (kr != result)
	quit(1, "%s: mach_msg_send: %s\n",
	     program, mach_error_string(kr));
}

void
do_mach_msg_receive(msg, option, size, name, timeout, notify, result)
    mach_msg_header_t *msg;
    mach_msg_option_t option;
    mach_msg_size_t size;
    mach_port_t name;
    mach_msg_timeout_t timeout;
    mach_port_t notify;
    mach_msg_return_t result;
{
    mach_msg_return_t kr;

    kr = mach_msg(msg, MACH_RCV_MSG|option, 0, size, name, timeout, notify);
    if (kr != result)
	quit(1, "%s: mach_msg_receive: %s\n",
	     program, mach_error_string(kr));
}

void
do_mach_msg_rpc(msg, option, send_size, rcv_size, rcv_name,
		timeout, notify, result)
    mach_msg_header_t *msg;
    mach_msg_option_t option;
    mach_msg_size_t send_size;
    mach_msg_size_t rcv_size;
    mach_port_t rcv_name;
    mach_msg_timeout_t timeout;
    mach_port_t notify;
    mach_msg_return_t result;
{
    mach_msg_return_t kr;

    kr = mach_msg(msg, MACH_SEND_MSG|MACH_RCV_MSG|option,
		  send_size, rcv_size, rcv_name,
		  timeout, notify);
    if (kr != result)
	quit(1, "%s: mach_msg_rpc: %s\n",
	     program, mach_error_string(kr));
}

void
do_simple_send(msg, size)
    mach_msg_header_t *msg;
    mach_msg_size_t size;
{
    do_mach_msg_send(msg, MACH_MSG_OPTION_NONE, size, MACH_MSG_TIMEOUT_NONE,
		     MACH_PORT_NULL, MACH_MSG_SUCCESS);
}

void
do_simple_receive(msg, size, port)
    mach_msg_header_t *msg;
    mach_msg_size_t size;
    mach_port_t port;
{
    do_mach_msg_receive(msg, MACH_MSG_OPTION_NONE, size, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);
}

void
send_simple_msg(port, id, notify, ret)
    mach_port_t port;
    int id;
    mach_port_t notify;
    mach_msg_return_t ret;
{
    mach_msg_header_t msg;
    mach_msg_option_t option;

    fillin_msg_header(&msg, FALSE, MACH_MSG_TYPE_COPY_SEND, 0,
		      port, MACH_PORT_NULL, 0, id);

    option = MACH_SEND_TIMEOUT;
    if (notify != MACH_PORT_NULL)
	option |= MACH_SEND_NOTIFY;

    do_mach_msg_send(&msg, option, sizeof msg, MACH_MSG_TIMEOUT_NONE,
		     notify, ret);
}

void
rcv_simple_msg(port, id, seqno, notify)
    mach_port_t port;
    mach_msg_id_t id;
    mach_port_seqno_t seqno;
    mach_port_t notify;
{
    mach_msg_header_t msg;
    mach_msg_option_t option;

    option = MACH_RCV_TIMEOUT;
    if (notify != MACH_PORT_NULL)
	option |= MACH_RCV_NOTIFY;

    do_mach_msg_receive(&msg, option, sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);

    check_msg_header(&msg, FALSE, 0, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, MACH_PORT_NULL, port, seqno, id);
}

void
rcv_mig_reply(port, id, seqno, ret)
    mach_port_t port;
    mach_msg_id_t id;
    mach_port_seqno_t seqno;
    kern_return_t ret;
{
    mig_reply_header_t msg;

    do_simple_receive(&msg.Head, sizeof msg, port);

    check_msg_header(&msg.Head, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, port, seqno, id);
    check_msg_type_integer(&msg.RetCodeType);
    if (msg.RetCode != ret)
	quit(1, "%s: rcv_mig_reply: RetCode = %d\n", program, msg.RetCode);
}

void
my_mach_port_acquire_send(port)
    mach_port_t port;
{
    kern_return_t kr;

    kr = mach_port_insert_right(mach_task_self(), port,
				port, MACH_MSG_TYPE_MAKE_SEND);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_insert_right: %s\n",
	     program, mach_error_string(kr));
}

void
my_mach_port_allocate_name(right, name)
    mach_port_right_t right;
    mach_port_t name;
{
    kern_return_t kr;

    kr = mach_port_allocate_name(mach_task_self(), right, name);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_allocate_name: %s\n",
	     program, mach_error_string(kr));
}

void
my_mach_port_allocate_name_receive(name)
    mach_port_t name;
{
   my_mach_port_allocate_name(MACH_PORT_RIGHT_RECEIVE, name);
}

void
my_mach_port_allocate_name_send(name)
{
    my_mach_port_allocate_name_receive(name);
    my_mach_port_acquire_send(name);
}

void
my_mach_port_allocate_name_set(name)
{
    my_mach_port_allocate_name(MACH_PORT_RIGHT_PORT_SET, name);
}

mach_port_t
my_mach_port_allocate(right)
    mach_port_right_t right;
{
    kern_return_t kr;
    mach_port_t port;

    kr = mach_port_allocate(mach_task_self(), right, &port);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_allocate: %s\n",
	     program, mach_error_string(kr));

    return port;
}

mach_port_t
my_mach_port_allocate_receive()
{
    return my_mach_port_allocate(MACH_PORT_RIGHT_RECEIVE);
}

mach_port_t
my_mach_port_allocate_send()
{
    mach_port_t port;

    port = my_mach_port_allocate_receive();
    my_mach_port_acquire_send(port);

    return port;
}

mach_port_t
my_mach_port_allocate_set()
{
    return my_mach_port_allocate(MACH_PORT_RIGHT_PORT_SET);
}

mach_port_t
my_mach_port_allocate_dead()
{
    return my_mach_port_allocate(MACH_PORT_RIGHT_DEAD_NAME);
}

void
my_mach_port_destroy(port)
    mach_port_t port;
{
    kern_return_t kr;
    
    kr = mach_port_destroy(mach_task_self(), port);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_destroy(%d): %s\n",
	     program, port, mach_error_string(kr));
}

void
my_mach_port_deallocate(port)
    mach_port_t port;
{
    kern_return_t kr;
    
    kr = mach_port_deallocate(mach_task_self(), port);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_deallocate(%d): %s\n",
	     program, port, mach_error_string(kr));
}

void
do_mach_port_mod_refs(port, right, delta, ret)
    mach_port_t port;
    mach_port_right_t right;
    int delta;
    kern_return_t ret;
{
    kern_return_t kr;

    kr = mach_port_mod_refs(mach_task_self(), port, right, delta);
    if (kr != ret)
	quit(1, "%s: mach_port_mod_refs(%d): %s\n",
	     program, port, mach_error_string(kr));
}

void
my_mach_port_mod_refs(port, right, delta)
    mach_port_t port;
    mach_port_right_t right;
    int delta;
{
    kern_return_t kr;

    kr = mach_port_mod_refs(mach_task_self(), port, right, delta);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_mod_refs(%d): %s\n",
	     program, port, mach_error_string(kr));
}

void
my_mach_port_deallocate_receive(port)
    mach_port_t port;
{
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_RECEIVE, -1);
}

void
my_mach_port_deallocate_dead(port)
    mach_port_t port;
{
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_DEAD_NAME, -1);
}

void
my_mach_port_deallocate_set(set)
    mach_port_t set;
{
    my_mach_port_mod_refs(set, MACH_PORT_RIGHT_PORT_SET, -1);
}

void
my_mach_port_deallocate_send(set)
    mach_port_t set;
{
    my_mach_port_mod_refs(set, MACH_PORT_RIGHT_SEND, -1);
}

void
do_mach_port_move_member(port, set, ret)
    mach_port_t port, set;
{
    kern_return_t kr;

    kr = mach_port_move_member(mach_task_self(), port, set);
    if (kr != ret)
	quit(1, "%s: mach_port_move_member(%d, %d): %s\n",
	     program, port, set, mach_error_string(kr));
}

void
my_mach_port_move_member(port, set)
    mach_port_t port, set;
{
    kern_return_t kr;

    kr = mach_port_move_member(mach_task_self(), port, set);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_move_member(%d, %d): %s\n",
	     program, port, set, mach_error_string(kr));
}

void
do_mach_port_request_notification(port, msg_id, sync, notify, msg_type, ret)
    mach_port_t port;
    int msg_id;
    unsigned int sync;
    mach_port_t notify;
    unsigned int msg_type;
    kern_return_t ret;
{
    mach_port_t actual_previous;
    kern_return_t kr;

    kr = mach_port_request_notification(mach_task_self(), port, msg_id, sync,
					notify, msg_type, &actual_previous);
    if (kr != ret)
	quit(1, "%s: mach_port_request_notification(%d): %s\n",
	     program, port, mach_error_string(kr));
}

void
my_mach_port_request_notification(port, msg_id, sync,
				  notify, msg_type, previous)
    mach_port_t port;
    int msg_id;
    unsigned int sync;
    mach_port_t notify;
    unsigned int msg_type;
    mach_port_t *previous;
{
    mach_port_t actual_previous;
    kern_return_t kr;

    kr = mach_port_request_notification(mach_task_self(), port, msg_id, sync,
					notify, msg_type, &actual_previous);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_request_notification(%d): %s\n",
	     program, port, mach_error_string(kr));

    if (previous == NULL) {
	if (actual_previous != MACH_PORT_NULL)
	    quit(1, "%s: mach_port_request_notification: previous = %d\n",
		 program, actual_previous);
    } else
	*previous = actual_previous;
}

void
do_mach_port_rename(old_name, new_name, ret)
    mach_port_t old_name, new_name;
    kern_return_t ret;
{
    kern_return_t kr;

    kr = mach_port_rename(mach_task_self(), old_name, new_name);
    if (kr != ret)
	quit(1, "%s: mach_port_rename(%d, %d): %s\n",
	     program, old_name, new_name,
	     mach_error_string(kr));
}

void
my_mach_port_rename(old_name, new_name)
    mach_port_t old_name, new_name;
{
    do_mach_port_rename(old_name, new_name,
			((old_name == new_name) ?
			 KERN_NAME_EXISTS : KERN_SUCCESS));
}

void
do_mach_port_insert_right(target, name, port, type, ret)
    task_t target;
    mach_port_t name;
    mach_port_t port;
    mach_msg_type_name_t type;
    kern_return_t ret;
{
    kern_return_t kr;

    kr = mach_port_insert_right(target, name, port, type);
    if (kr != ret)
	quit(1, "%s: mach_port_insert_right: %s\n",
	     program, mach_error_string(kr));
}

void
my_mach_port_insert_right(target, name, port, type)
    task_t target;
    mach_port_t name;
    mach_port_t port;
    mach_msg_type_name_t type;
{
    kern_return_t kr;

    kr = mach_port_insert_right(target, name, port, type);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_insert_right: %s\n",
	     program, mach_error_string(kr));
}

mach_port_t
my_mach_port_extract_right(target, name, msgt_name)
    task_t target;
    mach_port_t name;
    mach_msg_type_name_t msgt_name;
{
    mach_port_t right;
    mach_msg_type_name_t right_name;
    kern_return_t kr;

    kr = mach_port_extract_right(target, name, msgt_name,
				 &right, &right_name);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_extract_right: %s\n",
	     program, mach_error_string(kr));

    if (right_name != copyin_type(msgt_name))
	quit(1, "%s: mach_port_extract_right: %s\n",
	     program, mach_error_string(kr));

    return right;
}

void
send_mach_port_extract_right(target, reply, reply_type, name, msgt_name)
    mach_port_t target, reply;
    mach_msg_type_name_t reply_type;
    mach_port_t name;
    mach_msg_type_name_t msgt_name;
{
    struct
    {
	mach_msg_header_t Head;
	mach_msg_type_t nameType;
	mach_port_t name;
	mach_msg_type_t msgt_nameType;
	mach_msg_type_name_t msgt_name;
    } msg;

    fillin_msg_header(&msg.Head, FALSE, MACH_MSG_TYPE_COPY_SEND, reply_type,
		      target, reply, 0, 3216);

    fillin_msg_type_port_name(&msg.nameType);
    msg.name = name;

    fillin_msg_type_integer(&msg.msgt_nameType);
    msg.msgt_name = msgt_name;

    do_simple_send(&msg.Head, sizeof msg);
}

mach_port_t
rcv_mach_port_extract_right(seqno, reply, reply_type, poly_type)
    mach_port_seqno_t seqno;
    mach_port_t reply;
    mach_msg_type_name_t reply_type;
    mach_msg_type_name_t poly_type;
{
    struct
    {
	mach_msg_header_t Head;
	mach_msg_type_t RetCodeType;
	kern_return_t RetCode;
	mach_msg_type_t polyType;
	mach_port_t poly;
    } msg;

    do_simple_receive(&msg.Head, sizeof msg, reply);

    check_msg_header(&msg.Head, TRUE, 0, reply_type,
		     sizeof msg, MACH_PORT_NULL, reply,
		     seqno, 3316);
    check_msg_type_integer(&msg.RetCodeType);
    if (msg.RetCode != KERN_SUCCESS)
	quit(1, "%s: rcv_mach_port_extract_right: %s\n",
	     program, mach_error_string(msg.RetCode));
    check_msg_type(&msg.polyType, poly_type, 32, 1, TRUE, FALSE);

    return msg.poly;
}

mach_port_t
rpc_mach_port_extract_right(target, reply, reply_type, name, msgt_name,
			    seqno, result)
    mach_port_t target, reply;
    mach_msg_type_name_t reply_type;
    mach_port_t name;
    mach_msg_type_name_t msgt_name;
    mach_port_seqno_t seqno;
    mach_msg_return_t result;
{
    union
    {
	struct
	{
	    mach_msg_header_t Head;
	    mach_msg_type_t nameType;
	    mach_port_t name;
	    mach_msg_type_t msgt_nameType;
	    mach_msg_type_name_t msgt_name;
	} request;
	struct
	{
	    mach_msg_header_t Head;
	    mach_msg_type_t RetCodeType;
	    kern_return_t RetCode;
	    mach_msg_type_t polyType;
	    mach_port_t poly;
	} reply;
    } msg;

    fillin_msg_header(&msg.request.Head, FALSE,
		      MACH_MSG_TYPE_COPY_SEND, reply_type,
		      target, reply, 0, 3216);

    fillin_msg_type_port_name(&msg.request.nameType);
    msg.request.name = name;

    fillin_msg_type_integer(&msg.request.msgt_nameType);
    msg.request.msgt_name = msgt_name;

    do_mach_msg_rpc(&msg.request.Head, MACH_MSG_OPTION_NONE,
		    sizeof msg.request, sizeof msg.reply, reply,
		    MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL, result);

    if (result == MACH_MSG_SUCCESS)
    {
	check_msg_header(&msg.reply.Head, TRUE, 0, copyin_type(reply_type),
			 sizeof msg.reply, MACH_PORT_NULL, reply,
			 seqno, 3316);
	check_msg_type_integer(&msg.reply.RetCodeType);
	if (msg.reply.RetCode != KERN_SUCCESS)
	    quit(1, "%s: rpc_mach_port_extract_right: %s\n",
		 program, mach_error_string(msg.reply.RetCode));
	check_msg_type(&msg.reply.polyType, copyin_type(msgt_name),
		       32, 1, TRUE, FALSE);

	return msg.reply.poly;
    }
    else
	return MACH_PORT_NULL;
}

void
rpc_mach_port_rename(target, reply, reply_type, rcv_name,
		     old_name, new_name, seqno, ret)
    mach_port_t target;
    mach_port_t reply;
    mach_msg_type_name_t reply_type;
    mach_port_t rcv_name;
    mach_port_t old_name, new_name;
    mach_port_seqno_t seqno;
    kern_return_t ret;
{
    union
    {
	struct
	{
	    mach_msg_header_t Head;
	    mach_msg_type_t old_nameType;
	    mach_port_t old_name;
	    mach_msg_type_t new_nameType;
	    mach_port_t new_name;
	} request;
	struct
	{
	    mach_msg_header_t Head;
	    mach_msg_type_t RetCodeType;
	    kern_return_t RetCode;
	} reply;
    } msg;

    fillin_msg_header(&msg.request.Head, FALSE,
		      MACH_MSG_TYPE_COPY_SEND, reply_type,
		      target, reply, 0, 3202);

    fillin_msg_type_port_name(&msg.request.old_nameType);
    msg.request.old_name = old_name;

    fillin_msg_type_port_name(&msg.request.new_nameType);
    msg.request.new_name = new_name;

    do_mach_msg_rpc(&msg.request.Head, MACH_MSG_OPTION_NONE,
		    sizeof msg.request, sizeof msg.reply, rcv_name,
		    MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL, ret);

    if (ret == MACH_MSG_SUCCESS)
    {
	check_msg_header(&msg.reply.Head, FALSE, 0, copyin_type(reply_type),
			 sizeof msg.reply, MACH_PORT_NULL, rcv_name,
			 seqno, 3302);
	check_msg_type_integer(&msg.reply.RetCodeType);
	if (msg.reply.RetCode != KERN_SUCCESS)
	    quit(1, "%s: rpc_mach_port_rename: %s\n",
		 program, mach_error_string(msg.reply.RetCode));
    }
}

void
set_mscount(port, mscount)
    mach_port_t port;
    unsigned int mscount;
{
    kern_return_t kr;

    kr = mach_port_set_mscount(mach_task_self(), port, mscount);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_set_mscount(%d, %u): %s\n",
	     program, port, mscount, mach_error_string(kr));
}

void
set_qlimit(port, qlimit)
    mach_port_t port;
    unsigned int qlimit;
{
    kern_return_t kr;

    kr = mach_port_set_qlimit(mach_task_self(), port, qlimit);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_set_qlimit(%d, %u): %s\n",
	     program, port, qlimit, mach_error_string(kr));
}

void
set_backup(port, backup)
    mach_port_t port;
    mach_port_t backup;
{
    my_mach_port_request_notification(port, MACH_NOTIFY_PORT_DESTROYED, FALSE,
				      backup, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);
}

void
set_dnrequest(port, notify)
    mach_port_t port;
    mach_port_t notify;
{
    my_mach_port_request_notification(port, MACH_NOTIFY_DEAD_NAME, FALSE,
				      notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);
}

void
check_port_type(port, type)
    mach_port_t port;
    mach_port_type_t type;
{
    mach_port_type_t actual_type;
    kern_return_t kr;

    kr = mach_port_type(mach_task_self(), port, &actual_type);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_type(%d): %s\n",
	     program, port, mach_error_string(kr));

    if (actual_type != type)
	quit(1, "%s: check_port_type: type = %d\n",
	     program, actual_type);
}

void
check_port_status(port, enabled, seqno, mscount, qlimit, msgs,
		  sorights, srights, pdrequest, nsrequest)
    mach_port_t port;
    mach_port_t enabled;
    mach_port_seqno_t seqno;
    mach_port_mscount_t mscount;
    mach_port_msgcount_t qlimit, msgs;
    mach_port_rights_t sorights, srights;
    boolean_t pdrequest, nsrequest;
{
    mach_port_status_t status;
    mach_port_rights_t actual_rights;
    kern_return_t kr;

    kr = mach_port_get_receive_status(mach_task_self(), port, &status);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_get_receive_status(%d): %s\n",
	     program, port, mach_error_string(kr));

    if (enabled != status.mps_pset)
	quit(1, "%s: check_port_status(%d): enabled = %x\n",
	     program, port, status.mps_pset);
    if (seqno != status.mps_seqno)
	quit(1, "%s: check_port_status(%d): seqno = %u\n",
	     program, port, status.mps_seqno);
    if (mscount != status.mps_mscount)
	quit(1, "%s: check_port_status(%d): mscount = %u\n",
	     program, port, status.mps_mscount);
    if (qlimit != status.mps_qlimit)
	quit(1, "%s: check_port_status(%d): qlimit = %u\n",
	     program, port, status.mps_qlimit);
    if (msgs != status.mps_msgcount)
	quit(1, "%s: check_port_status(%d): msgs = %u\n",
	     program, port, status.mps_msgcount);
    if (sorights != status.mps_sorights)
	quit(1, "%s: check_port_status(%d): sorights = %u\n",
	     program, port, status.mps_sorights);
    if ((srights != 0) != status.mps_srights)
	quit(1, "%s: check_port_status(%d): srights = %d\n",
	     program, port, status.mps_srights);
    if (pdrequest != status.mps_pdrequest)
	quit(1, "%s: check_port_status(%d): pdrequest = %d\n",
	     program, port, status.mps_pdrequest);
    if (nsrequest != status.mps_nsrequest)
	quit(1, "%s: check_port_status(%d): nsrequest = %d\n",
	     program, port, status.mps_nsrequest);

    kr = mach_port_get_srights(mach_task_self(), port, &actual_rights);
    if (kr == KERN_SUCCESS) {
	if (srights != actual_rights)
	    quit(1, "%s: check_port_status(%d): rights = %u\n",
		 program, port, actual_rights);
    } else if (MDebug)
	quit(1, "%s: mach_port_get_srights(%d): %s\n",
	     program, port, mach_error_string(kr));

}

void
check_port_refs(port, right, refs)
    mach_port_t port;
    mach_port_right_t right;
    unsigned int refs;
{
    unsigned int actual_refs;
    kern_return_t kr;

    kr = mach_port_get_refs(mach_task_self(), port, right, &actual_refs);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_get_refs(%d): %s\n",
	     program, port, mach_error_string(kr));

    if (refs != actual_refs)
	quit(1, "%s: check_port_refs(%d): refs = %d\n",
	     program, port, actual_refs);
}

void
check_empty_set(port_set)
    mach_port_t port_set;
{
    mach_port_array_t members;
    unsigned int membersCnt;
    kern_return_t kr;

    kr = mach_port_get_set_status(mach_task_self(), port_set,
				  &members, &membersCnt);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_get_set_status: %s\n",
	     program, mach_error_string(kr));
    if ((membersCnt != 0) || (members != 0))
	quit(1, "%s: mach_port_get_set_status: %d not empty\n",
	     program, port_set);
}

void
check_singleton_set(port_set, port)
    mach_port_t port_set;
    mach_port_t port;
{
    mach_port_array_t members;
    unsigned int membersCnt;
    kern_return_t kr;
    
    kr = mach_port_get_set_status(mach_task_self(), port_set,
				  &members, &membersCnt);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_get_set_status(%d): %s\n",
	     program, port_set, mach_error_string(kr));
    if ((membersCnt != 1) || (members[0] != port))
	quit(1, "%s: mach_port_get_set_status(%d): port %d not member\n",
	     program, port_set, port);

    kr = vm_deallocate(mach_task_self(),
		       (vm_address_t) members,
		       (vm_size_t) (membersCnt * sizeof(mach_port_t)));
    if (kr != KERN_SUCCESS)
	quit(1, "%s: vm_deallocate: %s\n",
	     program, mach_error_string(kr));
}

void
check_invalid_name(name)
    mach_port_t name;
{
    mach_port_type_t type;
    kern_return_t kr;

    kr = mach_port_type(mach_task_self(), name, &type);
    if (kr != KERN_INVALID_NAME)
	quit(1, "%s: mach_port_type(%d): %s\n",
	     program, name, mach_error_string(kr));
}

mach_port_t
get_invalid_name()
{
    mach_port_t name;

    name = my_mach_port_allocate_dead();
    my_mach_port_deallocate_dead(name);
    check_invalid_name(name);

    return name;
}

void
rcv_send_once(port, seqno)
    mach_port_t port;
    mach_port_seqno_t seqno;
{
    mach_send_once_notification_t msg;

    do_mach_msg_receive(&msg.not_header, MACH_RCV_TIMEOUT,
			sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_msg_header(&msg.not_header, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, port,
		     seqno, MACH_NOTIFY_SEND_ONCE);
}

void
rcv_msg_accepted(notify, seqno, port)
    mach_port_t notify;
    mach_port_seqno_t seqno;
    mach_port_t port;
{
    mach_msg_accepted_notification_t msg;

    do_mach_msg_receive(&msg.not_header, MACH_RCV_TIMEOUT,
			sizeof msg, notify,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_msg_header(&msg.not_header, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, notify,
		     seqno, MACH_NOTIFY_MSG_ACCEPTED);

    check_msg_type_port_name(&msg.not_type);

    if (msg.not_port != port)
	quit(1, "%s: rcv_msg_accepted: not_port = %d\n",
	     program, msg.not_port);
}

void
rcv_port_deleted(notify, seqno, port)
    mach_port_t notify;
    mach_port_seqno_t seqno;
    mach_port_t port;
{
    mach_port_deleted_notification_t msg;

    do_mach_msg_receive(&msg.not_header, MACH_RCV_TIMEOUT,
			sizeof msg, notify,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_msg_header(&msg.not_header, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, notify,
		     seqno, MACH_NOTIFY_PORT_DELETED);

    check_msg_type_port_name(&msg.not_type);

    if (msg.not_port != port)
	quit(1, "%s: rcv_port_deleted: not_port = %d\n",
	     program, msg.not_port);
}

void
rcv_dead_name(notify, seqno, port)
    mach_port_t notify;
    mach_port_seqno_t seqno;
    mach_port_t port;
{
    mach_dead_name_notification_t msg;

    do_mach_msg_receive(&msg.not_header, MACH_RCV_TIMEOUT,
			sizeof msg, notify,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_msg_header(&msg.not_header, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, notify,
		     seqno, MACH_NOTIFY_DEAD_NAME);

    check_msg_type_port_name(&msg.not_type);

    if (msg.not_port != port)
	quit(1, "%s: rcv_dead_name: not_port = %d\n",
	     program, msg.not_port);
}

void
rcv_no_senders(notify, seqno, mscount)
    mach_port_t notify;
    mach_port_seqno_t seqno;
    unsigned int mscount;
{
    mach_no_senders_notification_t msg;

    do_mach_msg_receive(&msg.not_header, MACH_RCV_TIMEOUT,
			sizeof msg, notify,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_msg_header(&msg.not_header, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, notify,
		     seqno, MACH_NOTIFY_NO_SENDERS);

    check_msg_type(&msg.not_type, MACH_MSG_TYPE_INTEGER_32,
		    32, 1, TRUE, FALSE);

    if (msg.not_count != mscount)
	quit(1, "%s: rcv_no_senders: not_count = %d\n",
	     program, msg.not_count);
}

void
rcv_port_destroyed(backup, seqno, port)
    mach_port_t backup;
    mach_port_seqno_t seqno;
    mach_port_t *port;
{
    mach_port_destroyed_notification_t msg;

    do_mach_msg_receive(&msg.not_header, MACH_RCV_TIMEOUT,
			sizeof msg, backup,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_msg_header(&msg.not_header, TRUE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, backup,
		     seqno, MACH_NOTIFY_PORT_DESTROYED);

    check_msg_type(&msg.not_type, MACH_MSG_TYPE_MOVE_RECEIVE,
		   32, 1, TRUE, FALSE);

    if (msg.not_port == MACH_PORT_NULL)
	quit(1, "%s: rcv_port_destroyed: null port\n", program);

    *port = msg.not_port;
}

vm_offset_t
my_vm_allocate(size)
    vm_size_t size;
{
    vm_offset_t offset;
    kern_return_t kr;

    kr = vm_allocate(mach_task_self(), &offset, size, TRUE);
    if (kr != KERN_SUCCESS)
	quit(1, "%s; vm_allocate: %s\n", program, mach_error_string(kr));

    return offset;
}

void
my_vm_deallocate(offset, size)
    vm_offset_t offset;
    vm_size_t size;
{
    kern_return_t kr;

    kr = vm_deallocate(mach_task_self(), offset, size);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: vm_deallocate: %s\n", program, mach_error_string(kr));
}

vm_offset_t
my_vm_allocate_page()
{
    return my_vm_allocate(vm_page_size);
}

void
my_vm_deallocate_page(offset)
    vm_offset_t offset;
{
    my_vm_deallocate(offset, vm_page_size);
}

void
check_valid_page(offset)
    vm_offset_t offset;
{
    vm_offset_t addr;
    vm_size_t size;
    vm_prot_t prot, max_prot;
    vm_inherit_t inherit;
    boolean_t shared;
    mach_port_t name;
    vm_offset_t offset_in_object;
    kern_return_t kr;

    addr = offset;
    kr = vm_region(mach_task_self(), &addr, &size, &prot, &max_prot,
		   &inherit, &shared, &name, &offset_in_object);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: vm_region: %s\n", program, mach_error_string(kr));

    if ((offset < addr) ||
	((addr + size) < (offset + vm_page_size)))
	quit(1, "%s: check_valid_page: offset=%08x, addr=%08x, size=%x\n",
	     program, offset, addr, size);
}

void
check_invalid_page(offset)
    vm_offset_t offset;
{
    vm_offset_t addr;
    vm_size_t size;
    vm_prot_t prot, max_prot;
    vm_inherit_t inherit;
    boolean_t shared;
    mach_port_t name;
    vm_offset_t offset_in_object;
    kern_return_t kr;

    addr = offset;
    kr = vm_region(mach_task_self(), &addr, &size, &prot, &max_prot,
		   &inherit, &shared, &name, &offset_in_object);
    if (kr == KERN_NO_SPACE)
	return;
    else if (kr != KERN_SUCCESS)
	quit(1, "%s: vm_region: %s\n", program, mach_error_string(kr));

    if ((addr <= offset) &&
	((offset + vm_page_size) <= (addr + size)))
	quit(1, "%s: check_invalid_page: offset=%08x, addr=%08x, size=%x\n",
	     program, offset, addr, size);
}

#define MAX_REGIONS	1024

struct vm_region vm_regions[MAX_REGIONS];

void
alloc_address_space()
{
    int i;
    vm_size_t size;

    i = 0;
    size = 1 << 31;

    for (;;) {
	vm_offset_t offset;
	kern_return_t kr;

	kr = vm_allocate(mach_task_self(), &offset, size, TRUE);
	if (kr == KERN_SUCCESS)
	{
#if	0
	    printf("Allocated offset %08x, size %x.\n", offset, size);
#endif

	    vm_regions[i].offset = offset;
	    vm_regions[i].size = size;

	    if (++i == MAX_REGIONS)
		quit(1, "%s: alloc_address_space: too many regions\n",
		     program);
	}
	else if (kr == KERN_NO_SPACE)
	{
	    if ((size >>= 1) == 0)
		break;
	}
	else
	    quit(1, "%s: vm_allocate: %s\n",
		 program, mach_error_string(kr));
    }
}

void
dealloc_address_space()
{
    int i;

    for (i = 0; i < MAX_REGIONS; i++)
	if (vm_regions[i].size != 0)
	{
	    kern_return_t kr;

#if	0
	    printf("Deallocating offset %08x, size %x.\n",
		   vm_regions[i].offset, vm_regions[i].size);
#endif

	    kr = vm_deallocate(mach_task_self(), vm_regions[i].offset,
			       vm_regions[i].size);
	    if (kr != KERN_SUCCESS)
		quit(1, "%s: vm_deallocate: %s\n",
		     program, mach_error_string(kr));

	    vm_regions[i].size = 0;
	}
}
