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
 * $Log:	ntest.c,v $
 * Revision 2.5  91/08/29  15:46:24  rpd
 * 	Updated for sequence numbers.
 * 	[91/08/11            rpd]
 * 
 * Revision 2.4  91/03/27  17:23:29  mrt
 * 	Changed mach.h include
 * 
 * Revision 2.3  91/03/19  12:17:41  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.2  90/09/12  16:30:36  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#include <mach.h>
#include <mach_error.h>
#include "main.h"
#include "utils.h"
#include "ntest.h"

void
msg_accepted_test_1()
{
    mach_port_t port;
    mach_port_t notify;

    /* test simple MACH_SEND_NOTIFY use */

    port = my_mach_port_allocate_send();
    notify = my_mach_port_allocate_receive();
    set_qlimit(port, 1);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending setup message.\n");
    send_simple_msg(port, 1, MACH_PORT_NULL, KERN_SUCCESS);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 1, 0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying MACH_SEND_TIMEOUT test.\n");
    send_simple_msg(port, 2, MACH_PORT_NULL, MACH_SEND_TIMED_OUT);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 1, 0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying MACH_SEND_WILL_NOTIFY test.\n");
    send_simple_msg(port, 3, notify, MACH_SEND_WILL_NOTIFY);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 2, 0, 3, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Trying MACH_SEND_NOTIFY_IN_PROGRESS test.\n");
    send_simple_msg(port, 4, notify, MACH_SEND_NOTIFY_IN_PROGRESS);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 2, 0, 3, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Receiving setup message.\n");
    rcv_simple_msg(port, 1, 0, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      1, 1, 0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Receiving test message.\n");
    rcv_simple_msg(port, 3, 1, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving MACH_NOTIFY_MSG_ACCEPTED notification.\n");
    rcv_msg_accepted(notify, 0, port);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_destroy(port);
    my_mach_port_destroy(notify);

    printf("Finished first msg-accepted test.\n");
}

void
msg_accepted_test_2()
{
    mach_port_t port;
    mach_port_t notify;

    /* test destruction of port with queued notify-requiring-messages */

    port = my_mach_port_allocate_send();
    notify = my_mach_port_allocate_receive();
    set_qlimit(port, 1);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending setup message.\n");
    send_simple_msg(port, 1, MACH_PORT_NULL, KERN_SUCCESS);

    printf("Sending test message.\n");
    send_simple_msg(port, 2, notify, MACH_SEND_WILL_NOTIFY);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 2, 0, 3, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Trying port-deallocate test.\n");
    my_mach_port_deallocate_receive(port);

    check_port_type(port, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, 1);
    my_mach_port_deallocate_dead(port);

    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving null MACH_NOTIFY_MSG_ACCEPTED notification.\n");
    rcv_msg_accepted(notify, 0, MACH_PORT_NULL);

    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_destroy(notify);

    printf("Finished second msg-accepted test.\n");
}

void
msg_accepted_test_3()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg;

    mach_port_t port;
    mach_port_t notify;
    mach_port_t temp;

    port = my_mach_port_allocate_send();
    notify = my_mach_port_allocate_receive();
    set_qlimit(port, 1);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending setup message.\n");
    send_simple_msg(port, 1, MACH_PORT_NULL, KERN_SUCCESS);

    printf("Sending test message.\n");
    send_simple_msg(port, 2, notify, MACH_SEND_WILL_NOTIFY);

    check_port_type(port,
		    MACH_PORT_TYPE_SEND_RECEIVE|MACH_PORT_TYPE_MAREQUEST);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 2, 0, 3, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    temp = my_mach_port_allocate_send();

    check_port_status(temp, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, temp, MACH_PORT_NULL);
    fillin_msg_type(&msg.type, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1,
		    TRUE, FALSE);
    msg.port = port;

    printf("Sending receive right away.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_type(port, MACH_PORT_TYPE_SEND|MACH_PORT_TYPE_MAREQUEST);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(temp, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    printf("Deallocating the remaining send right.\n");
    my_mach_port_deallocate_send(port);

    check_invalid_name(port);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(temp, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    printf("Recovering the receive right.\n");
    do_simple_receive(&msg.head, sizeof msg, temp);

    check_msg_header(&msg.head, TRUE, 0, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, MACH_PORT_NULL, temp, 0, 0);
    check_msg_type(&msg.type, MACH_MSG_TYPE_PORT_RECEIVE, 32, 1,
		   TRUE, FALSE);
    port = msg.port;

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 0);
    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      1, 2, 0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(temp, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    my_mach_port_deallocate_send(temp);
    my_mach_port_deallocate_receive(temp);
    check_invalid_name(temp);

    printf("Receiving setup message.\n");
    rcv_simple_msg(port, 1, 0, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 1, 0,
		      1, 1, 0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Receiving test message.\n");
    rcv_simple_msg(port, 2, 1, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 2, 0,
		      1, 0, 0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving null MACH_NOTIFY_MSG_ACCEPTED notification.\n");
    rcv_msg_accepted(notify, 0, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 2, 0,
		      1, 0, 0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(port);
    my_mach_port_deallocate_receive(notify);

    printf("Finished third msg-accepted test.\n");
}

void
msg_accepted_test_4()
{
    mach_port_t port;
    mach_port_t notify;
    mach_port_t name;

    port = my_mach_port_allocate_send();
    notify = my_mach_port_allocate_receive();
    set_qlimit(port, 1);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending setup message.\n");
    send_simple_msg(port, 1, MACH_PORT_NULL, KERN_SUCCESS);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 1, 0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending test message.\n");
    send_simple_msg(port, 2, notify, MACH_SEND_WILL_NOTIFY);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 2, 0, 3, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Receiving setup message.\n");
    rcv_simple_msg(port, 1, 0, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      1, 1, 0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    name = my_mach_port_allocate_receive();
    my_mach_port_deallocate_receive(name);
    check_invalid_name(name);

    printf("Renaming the send right, from %d to %d.\n", port, name);
    my_mach_port_rename(port, name);

    check_port_type(name,
		    MACH_PORT_TYPE_SEND_RECEIVE|MACH_PORT_TYPE_MAREQUEST);
    check_port_refs(name, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(name, MACH_PORT_NULL, 1, 1,
		      1, 1, 0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Receiving test message.\n");
    rcv_simple_msg(name, 2, 1, MACH_PORT_NULL);

    check_port_type(name, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_status(name, MACH_PORT_NULL, 2, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving MACH_NOTIFY_MSG_ACCEPTED notification.\n");
    rcv_msg_accepted(notify, 0, name);

    check_port_status(name, MACH_PORT_NULL, 2, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_destroy(name);
    my_mach_port_destroy(notify);

    printf("Finished fourth msg-accepted test.\n");
}

void
port_destroyed_test_1()
{
    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t backup = my_mach_port_allocate_receive();

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(backup, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Allocated port %d.\n", port);
    set_backup(port, backup);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, TRUE, FALSE);
    check_port_status(backup, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Sending setup message to port %d.\n", port);
    send_simple_msg(port, 0, MACH_PORT_NULL, KERN_SUCCESS);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, TRUE, FALSE);
    check_port_status(backup, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Generating MACH_NOTIFY_PORT_DESTROYED.\n");
    my_mach_port_destroy(port);

    check_invalid_name(port);
    check_port_status(backup, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    rcv_port_destroyed(backup, 0, &port);
    printf("Received port %d in MACH_NOTIFY_PORT_DESTROYED.\n", port);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);
    check_port_status(backup, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    set_backup(port, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);
    check_port_status(backup, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving setup message from port %d.\n", port);
    rcv_simple_msg(port, 0, 0, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(backup, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_destroy(port);
    my_mach_port_destroy(backup);

    printf("Finished first port-destroyed test.\n");
}

void
port_destroyed_test_2()
{
    mach_port_t port = my_mach_port_allocate_receive();
    mach_port_t backup = my_mach_port_allocate_receive();

    set_backup(port, backup);
    my_mach_port_destroy(backup);

    printf("Trying second port-destroyed test.\n");
    my_mach_port_destroy(port);

    printf("Finished second port-destroyed test.\n");
}

void
port_destroyed_test_3()
{
    int i;

    printf("Trying third port-destroyed test.\n");

    for (i = 0; i < 100; i++) {
	mach_port_t port = my_mach_port_allocate_receive();

	set_backup(port, port);
	my_mach_port_destroy(port);
    }

    printf("Finished third port-destroyed test.\n");
}

void
port_deleted_test_1()
{
    mach_port_t port = my_mach_port_allocate_receive();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Requesting dead-name notification.\n");
    set_dnrequest(port, notify);

    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Deallocating the port.\n");
    my_mach_port_deallocate_receive(port);

    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving the port-deleted notification.\n");
    rcv_port_deleted(notify, 0, port);

    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(notify);

    printf("Finished first port-deleted test.\n");
}

void
port_deleted_test_2()
{
    mach_msg_header_t msg;
    mach_port_t port = my_mach_port_allocate_receive();
    mach_port_t notify;
    mach_port_t name = get_invalid_name();

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_invalid_name(name);

    printf("Creating a send-once right.\n");
    my_mach_port_insert_right(mach_task_self(), name,
			      port, MACH_MSG_TYPE_MAKE_SEND_ONCE);

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(name, MACH_PORT_TYPE_SEND_ONCE);
    check_port_refs(name, MACH_PORT_RIGHT_SEND_ONCE, 1);

    printf("Requesting dead-name notification.\n");
    notify = my_mach_port_allocate_receive();
    set_dnrequest(name, notify);

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(name, MACH_PORT_TYPE_SEND_ONCE|MACH_PORT_TYPE_DNREQUEST);
    check_port_refs(name, MACH_PORT_RIGHT_SEND_ONCE, 1);

    printf("Sending test message to the send-once right.\n");
    fillin_msg_header(&msg, FALSE, MACH_MSG_TYPE_MOVE_SEND_ONCE, 0,
		      name, MACH_PORT_NULL, 0, 0);
    do_simple_send(&msg, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);
    check_invalid_name(name);

    printf("Receiving the port-deleted notification.\n");
    rcv_port_deleted(notify, 0, name);

    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(notify);

    printf("Receiving the test message.\n");
    do_simple_receive(&msg, sizeof msg, port);

    check_msg_header(&msg, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, port, 0, 0);
    check_port_status(port, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(port);

    printf("Finished second port-deleted test.\n");
}

void
port_deleted_test_3()
{
    mach_msg_header_t msg;
    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t notify;
    mach_port_t name = get_invalid_name();

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_invalid_name(name);

    printf("Creating a send-once right.\n");
    my_mach_port_insert_right(mach_task_self(), name,
			      port, MACH_MSG_TYPE_MAKE_SEND_ONCE);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, FALSE);
    check_port_type(name, MACH_PORT_TYPE_SEND_ONCE);
    check_port_refs(name, MACH_PORT_RIGHT_SEND_ONCE, 1);

    printf("Requesting dead-name notification.\n");
    notify = my_mach_port_allocate_receive();
    set_dnrequest(name, notify);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(name,
		    MACH_PORT_TYPE_SEND_ONCE|MACH_PORT_TYPE_DNREQUEST);
    check_port_refs(name, MACH_PORT_RIGHT_SEND_ONCE, 1);

    printf("Sending test message, with send-once reply port.\n");
    fillin_msg_header(&msg, FALSE,
		      MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MOVE_SEND_ONCE,
		      port, name, 0, 0);
    do_simple_send(&msg, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);
    check_invalid_name(name);

    printf("Receiving the port-deleted notification.\n");
    rcv_port_deleted(notify, 0, name);

    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(notify);

    printf("Receiving the test message.\n");
    do_simple_receive(&msg, sizeof msg, port);
    name = msg.msgh_remote_port;

    check_msg_header(&msg, FALSE,
		     MACH_MSG_TYPE_PORT_SEND_ONCE, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, name, port, 0, 0);
    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, FALSE);
    check_port_type(name, MACH_PORT_TYPE_SEND_ONCE);
    check_port_refs(name, MACH_PORT_RIGHT_SEND_ONCE, 1);

    printf("Deallocating the receive right.\n");
    my_mach_port_deallocate_receive(port);

    check_port_type(port, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, 1);
    check_port_type(name, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(name, MACH_PORT_RIGHT_DEAD_NAME, 1);

    printf("Deallocating the dead-names.\n");
    my_mach_port_deallocate_dead(port);
    my_mach_port_deallocate_dead(name);

    printf("Finished third port-deleted test.\n");
}

static void
port_deleted_test(port, right, type)
    mach_port_t port;
    mach_port_right_t right;
    unsigned int type;
{
    struct
    {
	mach_msg_header_t h;
	mach_msg_type_t t;
	mach_port_t p;
    } msg;

    mach_port_t dest;
    mach_port_t notify;

    check_port_type(port, MACH_PORT_TYPE(right));
    check_port_refs(port, right, 1);

    printf("Requesting dead-name notification.\n");
    notify = my_mach_port_allocate_receive();
    set_dnrequest(port, notify);

    check_port_type(port, MACH_PORT_TYPE(right)|MACH_PORT_TYPE_DNREQUEST);
    check_port_refs(port, right, 1);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    dest = my_mach_port_allocate_send();
    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_msg_header(&msg.h, TRUE, MACH_MSG_TYPE_COPY_SEND, 0,
		      dest, MACH_PORT_NULL, 0, 0);
    fillin_msg_type(&msg.t, type, 32, 1, TRUE, FALSE);
    msg.p = port;

    printf("Sending right away in message.\n");
    do_simple_send(&msg.h, sizeof msg);

    check_invalid_name(port);
    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving the port-deleted notification.\n");
    rcv_port_deleted(notify, 0, port);

    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(notify);

    printf("Receiving test message.\n");
    do_simple_receive(&msg.h, sizeof msg, dest);

    check_msg_header(&msg.h, TRUE, 0, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, MACH_PORT_NULL, dest, 0, 0);
    check_msg_type(&msg.t, type, 32, 1, TRUE, FALSE);
    if (msg.p != port)
	my_mach_port_rename(msg.p, port);

    check_port_type(dest, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(dest, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(dest, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    check_port_type(port, MACH_PORT_TYPE(right));
    check_port_refs(port, right, 1);
    set_dnrequest(port, MACH_PORT_NULL);

    my_mach_port_deallocate(dest);
    check_port_type(dest, MACH_PORT_TYPE_RECEIVE);
    check_port_status(dest, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(dest);
    check_invalid_name(dest);
}

void
port_deleted_test_4()
{
#if	0
    mach_port_t port = name_server_port;

    port_deleted_test(port, MACH_PORT_RIGHT_SEND,
		      MACH_MSG_TYPE_MOVE_SEND);
#endif

    printf("Finished fourth port-deleted test.\n");
}

void
port_deleted_test_5()
{
    mach_port_t port = my_mach_port_allocate_receive();
    mach_port_t name = get_invalid_name();

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_invalid_name(name);

    printf("Creating a send-once right.\n");
    my_mach_port_insert_right(mach_task_self(), name,
			      port, MACH_MSG_TYPE_MAKE_SEND_ONCE);

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    port_deleted_test(name, MACH_PORT_RIGHT_SEND_ONCE,
		      MACH_MSG_TYPE_MOVE_SEND_ONCE);

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Deallocating the send-once right.\n");

    my_mach_port_deallocate_receive(port);
    check_port_type(name, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(name, MACH_PORT_RIGHT_DEAD_NAME, 1);
    my_mach_port_deallocate_dead(name);

    printf("Finished fifth port-deleted test.\n");
}

void
port_deleted_test_6()
{
    mach_port_t port = my_mach_port_allocate_receive();

    port_deleted_test(port, MACH_PORT_RIGHT_RECEIVE,
		      MACH_MSG_TYPE_MOVE_RECEIVE);
    my_mach_port_deallocate_receive(port);

    printf("Finished sixth port-deleted test.\n");
}
