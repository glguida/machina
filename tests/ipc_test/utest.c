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
 * $Log:	utest.c,v $
 * Revision 2.5  91/08/29  15:46:51  rpd
 * 	Updated for sequence numbers.
 * 	[91/08/11            rpd]
 * 
 * Revision 2.4  91/03/27  17:23:50  mrt
 * 	Changed mach.h include
 * 
 * Revision 2.3  91/03/19  12:18:15  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.2  90/09/12  16:31:00  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#include <mach.h>
#include <mach_error.h>
#include <mach/notify.h>
#include <stdio.h>
#include "main.h"
#include "utils.h"
#include "utest.h"

#define UREFS_MAX	((1 << 16) - 2)

void
uref_overflow_test_1()
{
    mach_port_t dead;

    printf("Allocating dead name.\n");
    dead = my_mach_port_allocate_dead();

    check_port_type(dead, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, 1);

    printf("Setting urefs to max.\n");
    my_mach_port_mod_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX);

    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX+1);

    printf("Trying to increment urefs.\n");
    do_mach_port_mod_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, 1,
			  KERN_UREFS_OVERFLOW);

    check_port_type(dead, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX+1);
    my_mach_port_destroy(dead);

    printf("Finished first uref overflow test.\n");
}

void
uref_overflow_test_2()
{
    mach_port_t dead = my_mach_port_allocate_dead();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, 1);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying delayed dead-name request.\n");
    do_mach_port_request_notification(dead, MACH_NOTIFY_DEAD_NAME, FALSE,
				      notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      KERN_INVALID_ARGUMENT);

    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, 1);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving send-once notification.\n");
    rcv_send_once(notify, 0);

    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, 1);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying immediate dead-name request.\n");
    my_mach_port_request_notification(dead, MACH_NOTIFY_DEAD_NAME, TRUE,
				      notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);

    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, 2);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving dead-name notification.\n");
    rcv_dead_name(notify, 1, dead);

    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, 2);
    check_port_status(notify, MACH_PORT_NULL, 2, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Setting urefs to max.\n");
    my_mach_port_mod_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX-1);

    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX+1);
    check_port_status(notify, MACH_PORT_NULL, 2, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying immediate dead-name request.\n");
    do_mach_port_request_notification(dead, MACH_NOTIFY_DEAD_NAME, TRUE,
				      notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      KERN_UREFS_OVERFLOW);

    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX+1);
    check_port_status(notify, MACH_PORT_NULL, 2, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving send-once notification.\n");
    rcv_send_once(notify, 2);

    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX+1);
    check_port_status(notify, MACH_PORT_NULL, 3, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_destroy(dead);
    my_mach_port_destroy(notify);

    printf("Finished second uref overflow test.\n");
}

void
uref_overflow_test_3()
{
    mach_port_t port = my_mach_port_allocate_receive();

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 0);
    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying zero delta, send rights.\n");
    do_mach_port_mod_refs(port, MACH_PORT_RIGHT_SEND, 0,
			  KERN_INVALID_RIGHT);

    printf("Trying zero delta, receive rights.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_RECEIVE, 0);

    printf("Inserting a send right, with make.\n");
    my_mach_port_insert_right(mach_task_self(), port,
			      port, MACH_MSG_TYPE_MAKE_SEND);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Inserting another send right, with copy.\n");
    my_mach_port_insert_right(mach_task_self(), port,
			      port, MACH_MSG_TYPE_COPY_SEND);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 2);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Inserting another send right, with move.\n");
    my_mach_port_insert_right(mach_task_self(), port,
			      port, MACH_MSG_TYPE_MOVE_SEND);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 2);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Setting urefs to max.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX-2);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Trying to insert another send right, with make.\n");
    do_mach_port_insert_right(mach_task_self(), port,
			      port, MACH_MSG_TYPE_MAKE_SEND,
			      KERN_UREFS_OVERFLOW);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX);
    check_port_status(port, MACH_PORT_NULL, 0, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Inserting another send right, with move.\n");
    my_mach_port_insert_right(mach_task_self(), port,
			      port, MACH_MSG_TYPE_MOVE_SEND);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX);
    check_port_status(port, MACH_PORT_NULL, 0, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Deallocating the send rights.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_SEND, -UREFS_MAX);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 0);
    check_port_status(port, MACH_PORT_NULL, 0, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Deallocating the receive rights.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_RECEIVE, -1);

    check_invalid_name(port);

    printf("Finished third uref overflow test.\n");
}

void
uref_overflow_test_4()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type(&msg.type, MACH_MSG_TYPE_MOVE_SEND, 32, 1,
		    TRUE, FALSE);
    msg.port = port;

    printf("Sending send-right carrying message.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 0);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    printf("Inserting a send right, with make.\n");
    my_mach_port_insert_right(mach_task_self(), port,
			      port, MACH_MSG_TYPE_MAKE_SEND);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 3, FALSE, FALSE);

    printf("Setting urefs to max.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX-1);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX);
    check_port_status(port, MACH_PORT_NULL, 0, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 3, FALSE, FALSE);

    printf("Receiving the send right.\n");
    do_simple_receive(&msg.head, sizeof msg, port);

    check_msg_header(&msg.head, TRUE, 0, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, MACH_PORT_NULL, port, 0, 0);
    check_msg_type(&msg.type, MACH_MSG_TYPE_PORT_SEND, 32, 1,
		   TRUE, FALSE);
    if (msg.port != port)
	quit(1, "%s: uref_overflow_test_4: bad port\n", program);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX);
    check_port_status(port, MACH_PORT_NULL, 1, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    my_mach_port_destroy(port);
    check_invalid_name(port);

    printf("Finished fourth uref overflow test.\n");
}

void
uref_overflow_test_5()
{
    mach_msg_header_t msg;
    mach_port_t port = my_mach_port_allocate_send();

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Sending simple send-right carrying message.\n");
    fillin_msg_header(&msg, FALSE,
		      MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MOVE_SEND,
		      port, port, 0, 0);
    do_simple_send(&msg, sizeof msg);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 0);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    printf("Inserting a send right, with make.\n");
    my_mach_port_insert_right(mach_task_self(), port,
			      port, MACH_MSG_TYPE_MAKE_SEND);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 3, FALSE, FALSE);

    printf("Setting urefs to max.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX-1);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX);
    check_port_status(port, MACH_PORT_NULL, 0, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 3, FALSE, FALSE);

    printf("Receiving the send right.\n");
    do_simple_receive(&msg, sizeof msg, port);

    check_msg_header(&msg, FALSE,
		     MACH_MSG_TYPE_PORT_SEND, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, port, port, 0, 0);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX);
    check_port_status(port, MACH_PORT_NULL, 1, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    my_mach_port_destroy(port);
    check_invalid_name(port);

    printf("Finished fifth uref overflow test.\n");
}

void
uref_overflow_test_6()
{
    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Setting urefs to max.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX-1);

    check_port_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Requesting dead-name notification.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_DEAD_NAME, FALSE,
				      notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);

    check_port_refs(port, MACH_PORT_RIGHT_SEND, UREFS_MAX);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Deallocating receive rights.\n");
    my_mach_port_deallocate_receive(port);

    check_port_type(port, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX+1);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Trying to increment urefs.\n");
    do_mach_port_mod_refs(port, MACH_PORT_RIGHT_DEAD_NAME, 1,
			  KERN_UREFS_OVERFLOW);

    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX+1);

    printf("Trying zero delta.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_DEAD_NAME, 0);

    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX+1);

    printf("Trying to decrement urefs.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_DEAD_NAME, -1);

    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX);

    printf("Receiving dead-name notification.\n");
    rcv_dead_name(notify, 0, port);

    check_port_type(port, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(port, MACH_PORT_RIGHT_DEAD_NAME, UREFS_MAX);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Deallocating the dead name.\n");
    my_mach_port_mod_refs(port, MACH_PORT_RIGHT_DEAD_NAME, -UREFS_MAX);

    check_invalid_name(port);
    my_mach_port_deallocate_receive(notify);
    check_invalid_name(notify);

    printf("Finished sixth uref overflow test.\n");
}

void
uref_underflow_test()
{
    mach_port_t dead;

    printf("Allocating dead name.\n");
    dead = my_mach_port_allocate_dead();

    check_port_type(dead, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, 1);

    printf("Trying to underflow urefs.\n");
    do_mach_port_mod_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, -2,
			  KERN_INVALID_VALUE);

    check_port_type(dead, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(dead, MACH_PORT_RIGHT_DEAD_NAME, 1);
    my_mach_port_destroy(dead);

    printf("Finished uref underflow test.\n");
}
