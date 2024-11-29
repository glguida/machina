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
 * $Log:	ptest.c,v $
 * Revision 2.5  91/08/29  15:46:33  rpd
 * 	Updated for sequence numbers.
 * 	[91/08/11            rpd]
 * 
 * Revision 2.4  91/03/27  17:23:39  mrt
 * 	Changed mach.h include
 * 
 * Revision 2.3  91/03/19  12:17:51  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.2  90/09/12  16:30:45  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#include <mach.h>
#include <mach_error.h>
#include <mach/notify.h>
#include "main.h"
#include "utils.h"
#include "ptest.h"

void
set_test()
{
    mach_port_t port, setA, setB;
    
    port = my_mach_port_allocate_receive();
    printf("Allocated port %d.\n", port);
    
    setA = my_mach_port_allocate_set();
    printf("Allocated port set %d.\n", setA);
    check_empty_set(setA);

    printf("Adding port %d to port set %d.\n", port, setA);
    my_mach_port_move_member(port, setA);
    check_singleton_set(setA, port);

    printf("Adding port %d to port set %d.\n", port, setA);
    my_mach_port_move_member(port, setA);
    check_singleton_set(setA, port);
    
    setB = my_mach_port_allocate_set();
    printf("Allocated port set %d.\n", setB);
    check_empty_set(setB);
    
    printf("Moving port %d into port set %d.\n", port, setB);
    my_mach_port_move_member(port, setB);
    check_empty_set(setA);
    check_singleton_set(setB, port);
    
    printf("Removing port %d from port set %d.\n", port, setB);
    my_mach_port_move_member(port, MACH_PORT_NULL);
    check_empty_set(setB);
    
    printf("Adding port %d to port set %d.\n", port, setA);
    my_mach_port_move_member(port, setA);
    check_singleton_set(setA, port);
    
    printf("Deallocating port %d.\n", port);
    my_mach_port_deallocate_receive(port);
    check_empty_set(setA);
    
    port = my_mach_port_allocate_receive();
    printf("Allocated port %d.\n", port);
    
    printf("Adding port %d to port set %d.\n", port, setA);
    my_mach_port_move_member(port, setA);
    check_singleton_set(setA, port);

    printf("Deallocating port set %d.\n", setA);
    my_mach_port_deallocate_set(setA);

    printf("Trying mach_port_move_member KERN_NOT_IN_SET test.\n");
    do_mach_port_move_member(port, MACH_PORT_NULL, KERN_NOT_IN_SET);

    printf("Deallocating port %d.\n", port);
    my_mach_port_deallocate_receive(port);
    
    printf("Trying mach_port_move_member KERN_INVALID_NAME test.\n");
    do_mach_port_move_member(port, MACH_PORT_NULL, KERN_INVALID_NAME);
    
    printf("Trying mach_port_move_member KERN_INVALID_RIGHT test.\n");
    do_mach_port_move_member(mach_task_self(), MACH_PORT_NULL, KERN_INVALID_RIGHT);

    printf("Deallocating port set %d.\n", setB);
    my_mach_port_deallocate_set(setB);
}

void
port_type_test()
{
    mach_port_t port, notify;

    port = my_mach_port_allocate_receive();
    notify = my_mach_port_allocate_receive();

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);

    printf("Requesting dead-name notification.\n");
    set_dnrequest(port, notify);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE|MACH_PORT_TYPE_DNREQUEST);

    printf("Deallocating the notify port.\n");
    my_mach_port_deallocate_receive(notify);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE|MACH_PORT_TYPE_DNREQUEST);

    printf("Retrieving registered send-once right.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_DEAD_NAME,
				      FALSE, MACH_PORT_NULL,
				      MACH_MSG_TYPE_MOVE_SEND_ONCE,
				      &notify);

    if (notify != MACH_PORT_DEAD)
	quit(1, "%s: port_type_test: notify not dead\n", program);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    my_mach_port_deallocate_receive(port);

    printf("Finished port-type test.\n");
}

void
rename_test_1()
{
    mach_port_t name, invalid;

    name = my_mach_port_allocate_receive();
    invalid = get_invalid_name();
    printf("Allocated port %x.\n", name);

    printf("Renaming %x to %x.\n", name, name);
    my_mach_port_rename(name, name);

    printf("Renaming %x to %x.\n", name, name+1);
    my_mach_port_rename(name, name+1);

    printf("Renaming %x to %x.\n", name+1, invalid);
    my_mach_port_rename(name+1, invalid);

    printf("Renaming %x to %x.\n", invalid, 23894723);
    my_mach_port_rename(invalid, 23894723);

    printf("Renaming %x to %x.\n", 23894723, 23894722);
    my_mach_port_rename(23894723, 23894722);

    printf("Renaming %x to %x.\n", 23894722, name);
    my_mach_port_rename(23894722, name);

    my_mach_port_deallocate_receive(name);
    printf("Destroyed port %x.\n", name);

    printf("Finished first rename test.\n");
}

void
rename_test_2()
{
    mach_port_t port, pset, invalid;

    port = my_mach_port_allocate_receive();
    pset = my_mach_port_allocate_set();
    printf("Allocated port %x and pset %x.\n", port, pset);

    printf("Adding port %x to pset %x.\n", port, pset);
    my_mach_port_move_member(port, pset);

    check_singleton_set(pset, port);
    check_port_status(port, pset, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Swapping names %x and %x.\n", port, pset);
    invalid = get_invalid_name();
    my_mach_port_rename(port, invalid);
    my_mach_port_rename(pset, port);
    my_mach_port_rename(invalid, pset);

    invalid = port;
    port = pset;
    pset = invalid;

    check_singleton_set(pset, port);
    check_port_status(port, pset, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(port);
    my_mach_port_deallocate_set(pset);

    printf("Finished second rename test.\n");
}

void
rename_test_3()
{
    struct
    {
	mach_msg_header_t h;
	mach_msg_type_t t;
	mach_port_t p;
    } msg;

    mach_port_t port, dest, invalid;

    port = my_mach_port_allocate_send();
    printf("Allocated port %x.\n", port);
    dest = my_mach_port_allocate_receive();

    check_port_status(dest, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);

    fillin_msg_header(&msg.h, TRUE,
		      MACH_MSG_TYPE_MAKE_SEND_ONCE, 0,
		      dest, MACH_PORT_NULL, 0, 0);
    fillin_msg_type(&msg.t, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1, TRUE, FALSE);
    msg.p = port;

    printf("Sending test message with receive right.\n");
    do_simple_send(&msg.h, sizeof msg);

    check_port_status(dest, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);
    check_port_type(port, MACH_PORT_TYPE_SEND);

    invalid = get_invalid_name();
    printf("Renaming port %x to %x.\n", port, invalid);
    my_mach_port_rename(port, invalid);

    check_port_status(dest, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);
    check_invalid_name(port);
    check_port_type(invalid, MACH_PORT_TYPE_SEND);

    printf("Receiving test message.\n");
    do_simple_receive(&msg.h, sizeof msg, dest);

    check_msg_header(&msg.h, TRUE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, dest, 0, 0);
    check_msg_type(&msg.t, MACH_MSG_TYPE_PORT_RECEIVE, 32, 1, TRUE, FALSE);
    if (msg.p != invalid)
	quit(1, "%s: rename_test_3: bad port %x\n", program, msg.p);

    check_port_status(dest, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_type(invalid, MACH_PORT_TYPE_SEND_RECEIVE);

    my_mach_port_deallocate_receive(dest);
    my_mach_port_deallocate_receive(invalid);
    my_mach_port_deallocate(invalid);
    check_invalid_name(invalid);

    printf("Finished third rename test.\n");
}

void
rename_test_4()
{
    mach_port_t reply, newreply;
    mach_port_t unused;

    reply = my_mach_port_allocate_receive();
    printf("Allocated reply port %x.\n", reply);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    unused = get_invalid_name();

    printf("Using RPC to rename reply port from %x to %x.\n", reply, unused);
    rpc_mach_port_rename(mach_task_self(),
			 reply, MACH_MSG_TYPE_MAKE_SEND_ONCE, reply,
			 reply, unused, 0, MACH_RCV_INVALID_NAME);

    reply = unused;
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    newreply = my_mach_port_allocate_receive();
    printf("Allocated new reply port %x.\n", newreply);
    check_port_status(newreply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    unused = get_invalid_name();

    printf("Receiving previous reply and renaming reply port from %x to %x.\n",
	   reply, unused);
    rpc_mach_port_rename(mach_task_self(),
			 newreply, MACH_MSG_TYPE_MAKE_SEND_ONCE, unused,
			 reply, unused, 0, MACH_MSG_SUCCESS);

    reply = unused;
    check_port_status(reply, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(reply);
    check_port_status(newreply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Generating an invalid-name reply message.\n");
    rpc_mach_port_rename(mach_task_self(),
			 newreply, MACH_MSG_TYPE_MAKE_SEND_ONCE, newreply,
			 newreply, newreply, 0, MACH_MSG_SUCCESS);

    check_port_status(newreply, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving last reply message.\n");
    rcv_mig_reply(newreply, 3302, 1, KERN_NAME_EXISTS);

    check_port_status(newreply, MACH_PORT_NULL, 2, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(newreply);

    printf("Finished fourth rename test.\n");
}

void
extract_receive_test_1()
{
    mach_port_t port = my_mach_port_allocate_receive();
    mach_port_t reply = my_mach_port_allocate_receive();

    printf("Allocated port %x.\n", port);

    port = my_mach_port_extract_right(mach_task_self(), port,
				      MACH_MSG_TYPE_MOVE_RECEIVE);
    printf("Extracted receive right as %x.\n", port);

    printf("Acquiring send rights.\n");
    my_mach_port_acquire_send(port);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    port = my_mach_port_extract_right(mach_task_self(), port,
				      MACH_MSG_TYPE_MOVE_RECEIVE);
    printf("Extracted receive right as %x.\n", port);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    printf("Sending extract-right request message.\n");
    send_mach_port_extract_right(mach_task_self(), reply,
				 MACH_MSG_TYPE_MAKE_SEND_ONCE,
				 port, MACH_MSG_TYPE_MOVE_RECEIVE);

    check_port_type(port, MACH_PORT_TYPE_SEND);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    port = rcv_mach_port_extract_right(0, reply, MACH_MSG_TYPE_PORT_SEND_ONCE,
				       MACH_MSG_TYPE_PORT_RECEIVE);
    printf("Received %x in reply message.\n", port);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    my_mach_port_destroy(port);
    my_mach_port_destroy(reply);

    printf("Finished first extract-receive test.\n");
}

void
extract_receive_test_2()
{
    mach_port_t port;

    port = my_mach_port_allocate_send();
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    printf("Sending extract-right request that would create loop.\n");
    send_mach_port_extract_right(mach_task_self(), port,
				 MACH_MSG_TYPE_MAKE_SEND_ONCE,
				 port, MACH_MSG_TYPE_MOVE_RECEIVE);

    check_port_type(port, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, 1);

    my_mach_port_deallocate(port);

    port = my_mach_port_allocate_send();
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    printf("Sending extract-right request with null reply port.\n");
    send_mach_port_extract_right(mach_task_self(), MACH_PORT_NULL, 0,
				 port, MACH_MSG_TYPE_MOVE_RECEIVE);

    check_port_type(port, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, 1);

    my_mach_port_deallocate(port);

    port = my_mach_port_allocate_send();
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    printf("Sending extract-right request with normal RPC.\n");
    (void) rpc_mach_port_extract_right(mach_task_self(), port,
				       MACH_MSG_TYPE_MAKE_SEND_ONCE,
				       port, MACH_MSG_TYPE_MOVE_RECEIVE,
				       0, MACH_RCV_INVALID_NAME);

    check_port_type(port, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, 1);

    my_mach_port_deallocate(port);

    printf("Finished second extract-receive test.\n");
}
