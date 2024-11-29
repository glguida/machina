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
 * $Log:	nstest.c,v $
 * Revision 2.5  91/08/29  15:46:15  rpd
 * 	Updated for sequence numbers.
 * 	[91/08/11            rpd]
 * 
 * Revision 2.4  91/03/27  17:23:22  mrt
 * 	Changed mach.h include
 * 
 * Revision 2.3  91/03/19  12:17:29  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.2  90/09/12  16:30:20  rpd
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
#include "nstest.h"

void
no_senders_test_1()
{
    mach_port_t port = my_mach_port_allocate_receive();
    mach_port_t notify = my_mach_port_allocate_receive();
    mach_port_t previous;

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Setting mscount to one.\n");
    set_mscount(port, 1);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying immediate, reflexive no-senders request.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      port, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      &previous);
    if (previous != MACH_PORT_NULL)
	quit(1, "%s: no_senders_test_1: previous = %d\n",
	     program, previous);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving no-senders notification.\n");
    rcv_no_senders(port, 0, 1);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying immediate, non-reflexive no-senders request.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      &previous);
    if (previous != MACH_PORT_NULL)
	quit(1, "%s: no_senders_test_1: previous = %d\n",
	     program, previous);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving no-senders notification.\n");
    rcv_no_senders(notify, 0, 1);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying non-immediate, reflexive no-senders request.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 2,
				      port, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      &previous);
    if (previous != MACH_PORT_NULL)
	quit(1, "%s: no_senders_test_1: previous = %d\n",
	     program, previous);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, TRUE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Trying non-immediate, non-reflexive no-senders request.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 2,
				      notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      &previous);
    if (previous == MACH_PORT_NULL)
	quit(1, "%s: no_senders_test_1: previous = %d\n",
	     program, previous);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, TRUE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(previous, MACH_PORT_TYPE_SEND_ONCE);
    check_port_refs(previous, MACH_PORT_RIGHT_SEND_ONCE, 1);

    printf("Trying immediate no-senders request, using send-once.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      previous, MACH_MSG_TYPE_MOVE_SEND_ONCE,
				      &previous);
    if (previous == MACH_PORT_NULL)
	quit(1, "%s: no_senders_test_1: previous = %d\n",
	     program, previous);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(previous, MACH_PORT_TYPE_SEND_ONCE);
    check_port_refs(previous, MACH_PORT_RIGHT_SEND_ONCE, 1);

    printf("Receiving no-senders notification.\n");
    rcv_no_senders(port, 1, 1);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(previous, MACH_PORT_TYPE_SEND_ONCE);
    check_port_refs(previous, MACH_PORT_RIGHT_SEND_ONCE, 1);

    printf("Trying non-immediate, non-reflexive request, using send-once.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 2,
				      previous, MACH_MSG_TYPE_MOVE_SEND_ONCE,
				      &previous);
    if (previous != MACH_PORT_NULL)
	quit(1, "%s: no_senders_test_1: previous = %d\n",
	     program, previous);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, TRUE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Deallocating the notify port.\n");
    my_mach_port_deallocate_receive(notify);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, TRUE);

    printf("Retrieving dead no-senders request.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      MACH_PORT_NULL,
				      MACH_MSG_TYPE_MOVE_SEND_ONCE,
				      &previous);
    if (previous != MACH_PORT_DEAD)
	quit(1, "%s: no_senders_test_1: previous = %d\n",
	     program, previous);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(port);

    printf("Finished first no-senders test.\n");
}

void
no_senders_test_2()
{
    mach_port_t port = my_mach_port_allocate_send();

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Requesting no-senders notification.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      port, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, TRUE);

    printf("Sending test message.\n");
    send_simple_msg(port, 0, MACH_PORT_NULL, MACH_MSG_SUCCESS);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 2, FALSE, TRUE);

    printf("Deallocating send rights.\n");
    my_mach_port_deallocate_send(port);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 0);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 1, FALSE, TRUE);

    printf("Receiving the test message.\n");
    rcv_simple_msg(port, 0, 0, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving the no-senders notification.\n");
    rcv_no_senders(port, 1, 1);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(port);

    printf("Finished second no-senders test.\n");
}

void
no_senders_test_3()
{
    mach_port_t port = my_mach_port_allocate_send();

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Requesting no-senders notification.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      port, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, TRUE);

    printf("Deallocating send rights, with mach_port_mod_refs.\n");
    my_mach_port_deallocate_send(port);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 0);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving the no-senders notification.\n");
    rcv_no_senders(port, 0, 1);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(port);

    printf("Finished third no-senders test.\n");
}

void
no_senders_test_4()
{
    mach_port_t port = my_mach_port_allocate_send();

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Requesting no-senders notification.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      port, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, TRUE);

    printf("Deallocating send rights, with mach_port_deallocate.\n");
    my_mach_port_deallocate(port);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 0);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving the no-senders notification.\n");
    rcv_no_senders(port, 0, 1);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(port);

    printf("Finished fourth no-senders test.\n");
}

void
no_senders_test_5()
{
    struct
    {
	mach_msg_header_t h;
	mach_msg_type_t t;
	mach_port_t p;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t dest = my_mach_port_allocate_send();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Requesting no-senders notification.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, TRUE);
    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    fillin_msg_header(&msg.h, TRUE, MACH_MSG_TYPE_COPY_SEND, 0,
		      dest, MACH_PORT_NULL, 0, 0);
    fillin_msg_type(&msg.t, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1, TRUE, FALSE);
    msg.p = port;

    printf("Sending receive right away in message.\n");
    do_simple_send(&msg.h, sizeof msg);

    check_port_type(port, MACH_PORT_TYPE_SEND);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Deallocating send rights, with mach_port_destroy.\n");
    my_mach_port_destroy(port);

    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving the no-senders notification.\n");
    rcv_no_senders(notify, 0, 0);

    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(notify);

    printf("Destroying port with queued message.\n");
    my_mach_port_destroy(dest);

    printf("Finished fifth no-senders test.\n");
}

void
no_senders_test_6()
{
    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Requesting no-senders notification.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, TRUE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Requesting port-destroyed notification.\n");
    set_backup(port, notify);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, TRUE, TRUE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      2, 0, FALSE, FALSE);

    printf("Destroying port.\n");
    my_mach_port_destroy(port);

    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 2,
		      2, 0, FALSE, FALSE);

    printf("Receiving port-destroyed notification.\n");
    rcv_port_destroyed(notify, 0, &port);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving no-senders notification.\n");
    rcv_no_senders(notify, 1, 1);

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 2, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(port);
    my_mach_port_deallocate_receive(notify);

    printf("Finished sixth no-senders test.\n");
}

void
no_senders_test_7()
{
    struct
    {
	mach_msg_header_t h;
	mach_msg_type_t t;
	mach_port_t p;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t dest = my_mach_port_allocate_send();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Requesting no-senders notification.\n");
    my_mach_port_request_notification(port, MACH_NOTIFY_NO_SENDERS, 0,
				      port, MACH_MSG_TYPE_MAKE_SEND_ONCE,
				      (mach_port_t *) NULL);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, TRUE);
    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_msg_header(&msg.h, TRUE, MACH_MSG_TYPE_COPY_SEND, 0,
		      dest, MACH_PORT_NULL, 0, 0);
    fillin_msg_type(&msg.t, MACH_MSG_TYPE_MOVE_SEND, 32, 1, TRUE, FALSE);
    msg.p = port;

    printf("Sending send right away in message.\n");
    do_simple_send(&msg.h, sizeof msg);

    check_port_type(port, MACH_PORT_TYPE_RECEIVE);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, TRUE);
    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    printf("Deallocating send right, by destroying queued message.\n");
    my_mach_port_destroy(dest);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving the no-senders notification.\n");
    rcv_no_senders(port, 0, 1);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(port);

    printf("Finished seventh no-senders test.\n");
}
