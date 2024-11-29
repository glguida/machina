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
 * $Log:	stest.c,v $
 * Revision 2.5  91/08/29  15:46:44  rpd
 * 	Updated for sequence numbers.
 * 	[91/08/11            rpd]
 * 
 * Revision 2.4  91/03/27  17:23:44  mrt
 * 	Changed mach.h include
 * 
 * Revision 2.3  91/03/19  12:18:06  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.2  90/09/12  16:30:53  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#include <mach.h>
#include <mach_error.h>
#include <mach/message.h>
#include <mach/notify.h>
#include "main.h"
#include "utils.h"
#include "stest.h"

extern long random();

#define	NUM_PORTS	128

static struct {
    mach_port_t name;
    boolean_t sentto;
    boolean_t receive;
} ports[NUM_PORTS];

static void
alloc_port(num, notify)
    int num;
    mach_port_t notify;
{
    mach_port_t name;
    kern_return_t kr;

    do {
	name = (mach_port_t) random();
	kr = mach_port_allocate_name(mach_task_self(),
				     MACH_PORT_RIGHT_RECEIVE, name);
    } while (kr != KERN_SUCCESS);

    if (Verbose)
	printf("Allocated port %08x.\n", name);

    my_mach_port_acquire_send(name);

    kr = mach_port_request_notification(mach_task_self(), name,
					MACH_NOTIFY_DEAD_NAME, TRUE,
					notify, MACH_MSG_TYPE_MAKE_SEND_ONCE,
					&notify);
    if (kr != KERN_SUCCESS)
	quit(1, "%s: mach_port_request_notification: %s\n",
	     program, mach_error_string(kr));
    else if (notify != MACH_PORT_NULL)
	quit(1, "%s: mach_port_request_notification: notify = %x\n",
	     program, notify);

    ports[num].name = name;
    ports[num].receive = TRUE;
    ports[num].sentto = FALSE;
}

static void
do_notify(notify)
    mach_port_t notify;
{
    for (;;) {
	mach_dead_name_notification_t msg;
	mach_port_t name;
	kern_return_t kr;
	unsigned int i;

	kr = mach_msg(&msg.not_header, MACH_RCV_TIMEOUT|MACH_RCV_MSG,
		      0, sizeof msg, notify, 0, MACH_PORT_NULL);
	if (kr == MACH_RCV_TIMED_OUT)
	    break;
	else if (kr != MACH_MSG_SUCCESS)
	    quit(1, "%s: do_notify: mach_msg: %s\n",
		 program, mach_error_string(kr));

	name = msg.not_port;

	for (i = 0; i < NUM_PORTS; i++)
	    if (ports[i].name == name)
		break;
	if (i == NUM_PORTS)
	    quit(1, "%s: do_notify: name = %x\n", program, name);

	if (ports[i].receive)
	    quit(1, "%s: do_notify: have receive\n", program);

	if (Verbose)
	    printf("Received notification for %08x.\n", name);

	kr = mach_port_mod_refs(mach_task_self(), name,
				MACH_PORT_RIGHT_DEAD_NAME, -2);
	if (kr != KERN_SUCCESS)
	    quit(1, "%s: do_notify: mach_port_mod_refs: %s\n",
		 program, mach_error_string(kr));

	alloc_port(i, notify);
    }
}

static int
find_random_receive()
{
    int index = random() % NUM_PORTS;

    for (;;) {
	if (ports[index].receive)
	    break;

	if (++index == NUM_PORTS)
	    index = 0;
    }

    return index;
}

static int
find_random_send()
{
    int index = random() % NUM_PORTS;

    for (;;) {
	if (!ports[index].sentto)
	    break;

	if (++index == NUM_PORTS)
	    index = 0;
    }

    return index;
}

static void
send_msg(dest, carried)
    mach_port_t dest, carried;
{
    struct {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg;

    if (Verbose)
	printf("Sending port %08x to port %08x.\n", carried, dest);

    fillin_msg_header(&msg.head, TRUE,
		      MACH_MSG_TYPE_COPY_SEND, 0,
		      dest, MACH_PORT_NULL, 0, 0);
    fillin_complex_header(&msg.head, dest, MACH_PORT_NULL);
    fillin_msg_type(&msg.type, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1, TRUE, FALSE);
    msg.port = carried;

    do_simple_send(&msg.head, sizeof msg);
}

static void
do_step(notify)
    mach_port_t notify;
{
    int sindex, rindex;

    sindex = find_random_send();
    rindex = find_random_receive();

    send_msg(ports[sindex].name, ports[rindex].name);
    ports[sindex].sentto = TRUE;
    ports[rindex].receive = FALSE;

    do_notify(notify);
}

void
splay_test(seed, iterations)
    int seed;
    unsigned int iterations;	/* zero means forever */
{
    mach_port_t notify;
    int index;

    printf("Starting splay test with seed %x.\n", seed);
    srandom(seed);

    notify = my_mach_port_allocate_receive();

    for (index = 0; index < NUM_PORTS; index++)
	alloc_port(index, notify);

    if (iterations != 0) {
	unsigned int i;

	for (i = 0; i < iterations; i++)
	    do_step(notify);
    } else
	for (;;)
	    do_step(notify);

    for (index = 0; index < NUM_PORTS; index++)
	my_mach_port_destroy(ports[index].name);
    my_mach_port_destroy(notify);

    printf("Finished splay test.\n");
}
