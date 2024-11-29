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
 * $Log:	mtest.c,v $
 * Revision 2.6  91/08/29  15:45:50  rpd
 * 	Updated for sequence numbers.
 * 	[91/08/11            rpd]
 * 
 * Revision 2.5  91/03/27  17:23:07  mrt
 * 	Changed mach.h include
 * 
 * Revision 2.4  91/03/19  12:17:11  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.3  91/03/10  13:40:32  rpd
 * 	Added header_ports_test.
 * 	[91/01/26            rpd]
 * 
 * Revision 2.2  90/09/12  16:30:05  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#include <mach.h>
#include <mach_error.h>
#include "main.h"
#include "utils.h"
#include "mtest.h"

void
complex_bit_test_1()
{
    mach_msg_header_t msg;
    mach_port_t port = my_mach_port_allocate_send();

    fillin_complex_header(&msg, port, MACH_PORT_NULL);

    printf("Sending complex message.\n");
    do_simple_send(&msg, sizeof msg);

    printf("Receiving simple message.\n");
    do_simple_receive(&msg, sizeof msg, port);

    check_simple_header(&msg, sizeof msg, MACH_PORT_NULL, port, 0);
    my_mach_port_destroy(port);

    printf("Finished first complex bit test.\n");
}

void
complex_bit_test_2()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	char *data;
    } msg1;
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg2;

    mach_port_t port = my_mach_port_allocate_send();

    fillin_complex_header(&msg1.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_pointer(&msg1.type);
    msg1.data = (char *) 0;

    printf("Sending complex message with OOL data.\n");
    do_simple_send(&msg1.head, sizeof msg1);

    printf("Receiving complex message.\n");
    do_simple_receive(&msg1.head, sizeof msg1, port);

    check_complex_header(&msg1.head, sizeof msg1, MACH_PORT_NULL, port, 0);
    check_msg_type_dummy_pointer(&msg1.type);
    if (msg1.data != (char *) 0)
	quit(1, "%s: complex_bit_test_2: msg1.data = %x\n",
	     program, msg1.data);

    fillin_complex_header(&msg2.head, port, MACH_PORT_NULL);
    fillin_msg_type_port(&msg2.type);
    msg2.port = MACH_PORT_NULL;

    printf("Sending complex message with a port right.\n");
    do_simple_send(&msg2.head, sizeof msg2);

    printf("Receiving complex message.\n");
    do_simple_receive(&msg2.head, sizeof msg2, port);

    check_complex_header(&msg2.head, sizeof msg2, MACH_PORT_NULL, port, 1);
    check_msg_type_port(&msg2.type);
    if (msg2.port != MACH_PORT_NULL)
	quit(1, "%s: complex_bit_test_2: msg2.port = %x\n",
	     program, msg2.port);

    my_mach_port_destroy(port);
    printf("Finished second complex bit test.\n");
}

void
complex_bit_test_3()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	char *data;
    } msg1;
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg2;

    mach_port_t port = my_mach_port_allocate_send();

    fillin_simple_header(&msg1.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_pointer(&msg1.type);
    msg1.data = (char *) 0;

    printf("Sending simple message with OOL data.\n");
    do_simple_send(&msg1.head, sizeof msg1);

    printf("Receiving simple message.\n");
    do_simple_receive(&msg1.head, sizeof msg1, port);

    check_simple_header(&msg1.head, sizeof msg1, MACH_PORT_NULL, port, 0);
    check_msg_type(&msg1.type, MACH_MSG_TYPE_INTEGER_32, 32, 0, FALSE, FALSE);
    if (msg1.data != (char *) 0)
	quit(1, "%s: complex_bit_test_2: msg1.data = %x\n",
	     program, msg1.data);

    fillin_simple_header(&msg2.head, port, MACH_PORT_NULL);
    fillin_msg_type_port(&msg2.type);
    msg2.port = MACH_PORT_NULL;

    printf("Sending simple message with a port right.\n");
    do_simple_send(&msg2.head, sizeof msg2);

    printf("Receiving simple message.\n");
    do_simple_receive(&msg2.head, sizeof msg2, port);

    check_simple_header(&msg2.head, sizeof msg2, MACH_PORT_NULL, port, 1);
    check_msg_type(&msg2.type, MACH_MSG_TYPE_COPY_SEND, 32, 1, TRUE, FALSE);
    if (msg2.port != MACH_PORT_NULL)
	quit(1, "%s: complex_bit_test_2: msg2.port = %x\n",
	     program, msg2.port);

    my_mach_port_destroy(port);
    printf("Finished third complex bit test.\n");
}

void
bad_pointer_test()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	char *data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type(&msg.type, MACH_MSG_TYPE_INTEGER_32, 32, 1024,
		    FALSE, FALSE);
    msg.data = (char *) 0x87654321;

    printf("Trying bad out-of-line integers.\n");
    do_mach_msg_send(&msg.head, MACH_MSG_OPTION_NONE, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		     MACH_SEND_INVALID_MEMORY);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type(&msg.type, MACH_MSG_TYPE_COPY_SEND, 32, 1024,
		    FALSE, FALSE);
    msg.data = (char *) 0x87654321;

    printf("Trying bad out-of-line ports.\n");
    do_mach_msg_send(&msg.head, MACH_MSG_OPTION_NONE, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		     MACH_SEND_INVALID_MEMORY);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    my_mach_port_destroy(port);
    printf("Finished bad pointer test.\n");
}

void
zero_pointer_test()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	char *data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_pointer(&msg.type);
    msg.data = (char *) 0;

    printf("Trying zero pointer to integers.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_dummy_pointer(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: zero_pointer_test: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_pointer(&msg.type);
    msg.type.msgt_deallocate = TRUE;
    msg.data = (char *) 0;

    printf("Trying zero pointer to integers, with dealloc.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 1);
    check_msg_type_dummy_pointer(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: zero_pointer_test: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_pointer(&msg.type);
    msg.data = (char *) 0x87654321;

    printf("Trying non-zero pointer to integers.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 2);
    check_msg_type_dummy_pointer(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: zero_pointer_test: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 3, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_pointer(&msg.type);
    msg.type.msgt_deallocate = TRUE;
    msg.data = (char *) 0x87654321;

    printf("Trying non-zero pointer to integers, with dealloc.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 3, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 3);
    check_msg_type_dummy_pointer(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: zero_pointer_test: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 4, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_ports(&msg.type);
    msg.data = (char *) 0;

    printf("Trying zero pointer to ports.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 4, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 4);
    check_msg_type_dummy_ports(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: zero_pointer_test: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 5, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_ports(&msg.type);
    msg.type.msgt_deallocate = TRUE;
    msg.data = (char *) 0;

    printf("Trying zero pointer to ports, with dealloc.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 5, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 5);
    check_msg_type_dummy_ports(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: zero_pointer_test: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 6, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_ports(&msg.type);
    msg.data = (char *) 0x87654321;

    printf("Trying non-zero pointer to ports.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 6, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 6);
    check_msg_type_dummy_ports(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: zero_pointer_test: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 7, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_ports(&msg.type);
    msg.type.msgt_deallocate = TRUE;
    msg.data = (char *) 0x87654321;

    printf("Trying non-zero pointer to ports, with dealloc.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 7, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 7);
    check_msg_type_dummy_ports(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: zero_pointer_test: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 8, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    my_mach_port_destroy(port);
    printf("Finished zero pointer test.\n");
}

void
bad_port_test()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t ports[3];
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type(&msg.type, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 3, TRUE, FALSE);
    msg.ports[0] = my_mach_port_allocate_send();
    msg.ports[2] = my_mach_port_allocate_send();
    msg.ports[1] = get_invalid_name();

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_type(msg.ports[0], MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_type(msg.ports[2], MACH_PORT_TYPE_SEND_RECEIVE);

    printf("Sending message with bad port right.\n");
    do_mach_msg_send(&msg.head, MACH_MSG_OPTION_NONE, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		     MACH_SEND_INVALID_RIGHT);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_type(msg.ports[0], MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(msg.ports[0], MACH_PORT_RIGHT_DEAD_NAME, 1);
    check_port_type(msg.ports[2], MACH_PORT_TYPE_SEND_RECEIVE);

    my_mach_port_destroy(port);
    my_mach_port_destroy(msg.ports[0]);
    my_mach_port_destroy(msg.ports[2]);

    printf("Finished bad port test.\n");
}

void
deallocate_bit_test_1()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	char *data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    printf("Setting queue-limit to zero.\n");
    set_qlimit(port, 0);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      0, 0, 0, 1, FALSE, FALSE);
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_dummy_pointer(&msg.type);
    msg.data = (char *) 0;

    printf("Sending and pseudo-receiving test message.\n");
    do_mach_msg_send(&msg.head, MACH_SEND_TIMEOUT, sizeof msg,
		     0, MACH_PORT_NULL, MACH_SEND_TIMED_OUT);

    check_msg_header(&msg.head, TRUE, MACH_MSG_TYPE_PORT_SEND, 0,
		     sizeof msg, port, MACH_PORT_NULL, 0, 0);
    check_msg_type_dummy_pointer(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: deallocate_bit_test_1: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      0, 0, 0, 1, FALSE, FALSE);
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 2);

    printf("Setting queue-limit to one.\n");
    set_qlimit(port, 1);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 2);

    printf("Resending test message.\n");
    do_mach_msg_send(&msg.head, MACH_SEND_TIMEOUT, sizeof msg,
		     0, MACH_PORT_NULL, MACH_MSG_SUCCESS);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      1, 1, 0, 2, FALSE, FALSE);
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    printf("Receiving test message.\n");
    do_simple_receive(&msg.head, sizeof msg, port);

    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_dummy_pointer(&msg.type);
    if (msg.data != (char *) 0)
	quit(1, "%s: deallocate_bit_test_1: data not zero\n", program);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      1, 0, 0, 1, FALSE, FALSE);
    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);

    my_mach_port_destroy(port);
    printf("Finished first deallocate bit test.\n");
}

void
deallocate_bit_test_2()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_long_t type;
	vm_offset_t data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_page_dealloc(&msg.type);
    msg.data = my_vm_allocate_page();
    printf("Allocated page %08x.\n", msg.data);

    printf("Sending message, moving the page.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_invalid_page(msg.data);

    printf("Receiving message.\n");
    do_simple_receive(&msg.head, sizeof msg, port);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_page(&msg.type);
    check_valid_page(msg.data);
    printf("Received page %08x.\n", msg.data);

    my_vm_deallocate_page(msg.data);
    my_mach_port_destroy(port);

    printf("Finished second deallocate bit test.\n");
}

void
deallocate_bit_test_3()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_long_t type;
	vm_offset_t data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_long(&msg.type, FALSE, TRUE,
			 MACH_MSG_TYPE_COPY_SEND, 32, vm_page_size/4);
    msg.data = my_vm_allocate_page();
    printf("Allocated page %08x.\n", msg.data);

    printf("Sending message, moving the page of ports.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_invalid_page(msg.data);

    printf("Receiving message.\n");
    do_simple_receive(&msg.head, sizeof msg, port);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_long(&msg.type, FALSE, TRUE,
			MACH_MSG_TYPE_PORT_SEND, 32, vm_page_size/4);
    check_valid_page(msg.data);
    printf("Received page %08x.\n", msg.data);

    my_vm_deallocate_page(msg.data);
    my_mach_port_destroy(port);

    printf("Finished third deallocate bit test.\n");
}

void
too_large_test_1()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	int data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    fillin_simple_header(&msg.head, port, MACH_PORT_NULL);
    msg.head.msgh_size = 0;
    fillin_msg_type_integer(&msg.type);
    msg.data = 42;

    printf("Sending setup message.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    printf("Trying MACH_RCV_TOO_LARGE receive.\n");
    do_mach_msg_receive(&msg.head, MACH_MSG_OPTION_NONE,
			sizeof msg.head, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_RCV_TOO_LARGE);

    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    my_mach_port_destroy(port);

    printf("Finished first too-large test.\n");
}

void
too_large_test_2()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	int data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    fillin_simple_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_integer(&msg.type);
    msg.data = 42;

    printf("Sending setup message.\n");
    do_simple_send(&msg.head, sizeof msg);

    printf("Trying MACH_RCV_LARGE/MACH_RCV_TOO_LARGE receive.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_LARGE,
			sizeof msg.head, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_RCV_TOO_LARGE);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    if (msg.head.msgh_size != sizeof msg)
	quit(1, "%s: too_large_test_2: size = %d\n",
	     program, msg.head.msgh_size);

    printf("Trying MACH_MSG_SUCCESS receive.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_TIMEOUT,
			sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_integer(&msg.type);
    if (msg.data != 42)
	quit(1, "%s: too_large_test: bad data\n", program);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    my_mach_port_destroy(port);

    printf("Finished second too-large test.\n");
}

void
null_dest_test()
{
    mach_msg_header_t msg;

    fillin_simple_header(&msg, MACH_PORT_NULL, MACH_PORT_NULL);

    printf("Trying null destination test.\n");
    do_mach_msg_send(&msg, MACH_MSG_OPTION_NONE, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		     MACH_SEND_INVALID_DEST);
}

void
msg_queue_test()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	int data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_simple_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_integer(&msg.type);

    printf("Sending first message.\n");
    msg.data = 0;
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    printf("Sending second message.\n");
    msg.data = 1;
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 2,
		      0, 3, FALSE, FALSE);

    printf("Sending third message.\n");
    msg.data = 2;
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 3,
		      0, 4, FALSE, FALSE);

    printf("Trying first receive.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_TIMEOUT,
			sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_integer(&msg.type);
    if (msg.data != 0)
	quit(1, "%s: msg_queue_test: bad data\n", program);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 2,
		      0, 3, FALSE, FALSE);

    printf("Trying second receive.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_TIMEOUT,
			sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 1);
    check_msg_type_integer(&msg.type);
    if (msg.data != 1)
	quit(1, "%s: msg_queue_test: bad data\n", program);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    printf("Trying third receive.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_TIMEOUT,
			sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);

    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 2);
    check_msg_type_integer(&msg.type);
    if (msg.data != 2)
	quit(1, "%s: msg_queue_test: bad data\n", program);

    check_port_status(port, MACH_PORT_NULL, 3, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Trying fourth receive.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_TIMEOUT,
			sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_RCV_TIMED_OUT);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 3, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    my_mach_port_destroy(port);

    printf("Finished msg-queueing test.\n");
}

void
null_port_test()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    static struct
    {
	unsigned int intype, outtype;
	char *name;
    } tests[] =
	{
      {
		MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_PORT_SEND,
		"MACH_MSG_TYPE_COPY_SEND",
      },
	  {
		MACH_MSG_TYPE_MAKE_SEND, MACH_MSG_TYPE_PORT_SEND,
		"MACH_MSG_TYPE_MAKE_SEND",
	  },
	  {
		MACH_MSG_TYPE_MOVE_SEND, MACH_MSG_TYPE_PORT_SEND,
		"MACH_MSG_TYPE_MOVE_SEND",
	  },
	  {
		MACH_MSG_TYPE_MAKE_SEND_ONCE, MACH_MSG_TYPE_PORT_SEND_ONCE,
		"MACH_MSG_TYPE_MAKE_SEND_ONCE",
	  },
	  {
		MACH_MSG_TYPE_MOVE_SEND_ONCE, MACH_MSG_TYPE_PORT_SEND_ONCE,
		"MACH_MSG_TYPE_MOVE_SEND_ONCE",
	  },
	  {
		MACH_MSG_TYPE_MOVE_RECEIVE, MACH_MSG_TYPE_PORT_RECEIVE,
		"MACH_MSG_TYPE_MOVE_RECEIVE",
	  },
	  {
		MACH_MSG_TYPE_PORT_NAME, MACH_MSG_TYPE_PORT_NAME,
		"MACH_MSG_TYPE_PORT_NAME",
	  },
    };

    int i;

    for (i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
	boolean_t complex = MACH_MSG_TYPE_PORT_ANY(tests[i].intype);

	fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
	fillin_msg_type(&msg.type, tests[i].intype, 32, 1,
			TRUE, FALSE);
	msg.port = MACH_PORT_NULL;

	printf("Trying %s null port test.\n", tests[i].name);
	do_simple_send(&msg.head, sizeof msg);
	do_simple_receive(&msg.head, sizeof msg, port);

	check_msg_header(&msg.head, complex, 0, MACH_MSG_TYPE_PORT_SEND,
			 sizeof msg, MACH_PORT_NULL, port, i, 0);
	check_msg_type(&msg.type, tests[i].outtype, 32, 1,
		       TRUE, FALSE);
	if (msg.port != MACH_PORT_NULL)
	    quit(1, "%s: null_port_test: port not null\n", program);
    }

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, i, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    my_mach_port_destroy(port);
}

void
dead_port_test()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();

    static struct
    {
	unsigned int intype, outtype;
	char *name;
    } tests[] =
	{
	  {
		MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_PORT_SEND,
		"MACH_MSG_TYPE_COPY_SEND",
	  },
	  {
		MACH_MSG_TYPE_MAKE_SEND, MACH_MSG_TYPE_PORT_SEND,
		"MACH_MSG_TYPE_MAKE_SEND",
	  },
	  {
		MACH_MSG_TYPE_MOVE_SEND, MACH_MSG_TYPE_PORT_SEND,
		"MACH_MSG_TYPE_MOVE_SEND",
	  },
	  {
		MACH_MSG_TYPE_MAKE_SEND_ONCE, MACH_MSG_TYPE_PORT_SEND_ONCE,
		"MACH_MSG_TYPE_MAKE_SEND_ONCE",
	  },
	  {
		MACH_MSG_TYPE_MOVE_SEND_ONCE, MACH_MSG_TYPE_PORT_SEND_ONCE,
		"MACH_MSG_TYPE_MOVE_SEND_ONCE",
	  },
	  {
		MACH_MSG_TYPE_MOVE_RECEIVE, MACH_MSG_TYPE_PORT_RECEIVE,
		"MACH_MSG_TYPE_MOVE_RECEIVE",
	  },
	  {
		MACH_MSG_TYPE_PORT_NAME, MACH_MSG_TYPE_PORT_NAME,
		"MACH_MSG_TYPE_PORT_NAME",
	  }
    };

    int i;

    for (i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
	boolean_t complex = MACH_MSG_TYPE_PORT_ANY(tests[i].intype);

	fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
	fillin_msg_type(&msg.type, tests[i].intype, 32, 1,
			TRUE, FALSE);
	msg.port = MACH_PORT_DEAD;

	printf("Trying %s dead port test.\n", tests[i].name);
	do_simple_send(&msg.head, sizeof msg);
	do_simple_receive(&msg.head, sizeof msg, port);

	check_msg_header(&msg.head, complex, 0, MACH_MSG_TYPE_PORT_SEND,
			 sizeof msg, MACH_PORT_NULL, port, i, 0);
	check_msg_type(&msg.type, tests[i].outtype, 32, 1,
		       TRUE, FALSE);
	if (msg.port != MACH_PORT_DEAD)
	    quit(1, "%s: dead_port_test: port not null\n", program);
    }

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, i, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    my_mach_port_destroy(port);
}

void
add_member_test()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	int data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t set = my_mach_port_allocate_set();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_empty_set(set);

    fillin_simple_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_integer(&msg.type);

    printf("Sending three setup messages to port.\n");
    msg.data = 0;
    do_simple_send(&msg.head, sizeof msg);
    msg.data = 1;
    do_simple_send(&msg.head, sizeof msg);
    msg.data = 2;
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 3,
		      0, 4, FALSE, FALSE);
    check_empty_set(set);

    printf("Moving port into set.\n");
    my_mach_port_move_member(port, set);

    check_port_status(port, set, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 3,
		      0, 4, FALSE, FALSE);
    check_singleton_set(set, port);

    printf("Receiving messages from set.\n");

    do_simple_receive(&msg.head, sizeof msg, set);
    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_integer(&msg.type);
    if (msg.data != 0)
	quit(1, "%s: add_member_test: bad data\n", program);

    do_simple_receive(&msg.head, sizeof msg, set);
    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 1);
    check_msg_type_integer(&msg.type);
    if (msg.data != 1)
	quit(1, "%s: add_member_test: bad data\n", program);

    do_simple_receive(&msg.head, sizeof msg, set);
    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 2);
    check_msg_type_integer(&msg.type);
    if (msg.data != 2)
	quit(1, "%s: add_member_test: bad data\n", program);

    check_port_status(port, set, 3, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_singleton_set(set, port);

    my_mach_port_destroy(port);
    check_empty_set(set);
    my_mach_port_destroy(set);

    printf("Finished add-member test.\n");
}

void
remove_member_test()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	int data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t set = my_mach_port_allocate_set();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_empty_set(set);

    printf("Moving port into set.\n");
    my_mach_port_move_member(port, set);

    check_port_status(port, set, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_singleton_set(set, port);

    fillin_simple_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_integer(&msg.type);

    printf("Sending three setup messages to port.\n");
    msg.data = 0;
    do_simple_send(&msg.head, sizeof msg);
    msg.data = 1;
    do_simple_send(&msg.head, sizeof msg);
    msg.data = 2;
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, set, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 3,
		      0, 4, FALSE, FALSE);
    check_singleton_set(set, port);

    printf("Removing port from set.\n");
    my_mach_port_move_member(port, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 3,
		      0, 4, FALSE, FALSE);
    check_empty_set(set);

    printf("Receiving messages from port.\n");

    do_simple_receive(&msg.head, sizeof msg, port);
    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_integer(&msg.type);
    if (msg.data != 0)
	quit(1, "%s: add_member_test: bad data\n", program);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 1);
    check_msg_type_integer(&msg.type);
    if (msg.data != 1)
	quit(1, "%s: add_member_test: bad data\n", program);

    do_simple_receive(&msg.head, sizeof msg, port);
    check_simple_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 2);
    check_msg_type_integer(&msg.type);
    if (msg.data != 2)
	quit(1, "%s: add_member_test: bad data\n", program);

    check_port_status(port, MACH_PORT_NULL, 3, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_empty_set(set);

    my_mach_port_destroy(port);
    my_mach_port_destroy(set);

    printf("Finished remove-member test.\n");
}

void
enabled_receive_test_1()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	int data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t set = my_mach_port_allocate_set();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_empty_set(set);

    fillin_simple_header(&msg.head, port, port);
    fillin_msg_type_integer(&msg.type);
    msg.data = 0;

    printf("Sending setup message to port.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 2, FALSE, FALSE);
    check_empty_set(set);

    printf("Moving port into set.\n");
    my_mach_port_move_member(port, set);

    check_port_status(port, set, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 2, FALSE, FALSE);
    check_singleton_set(set, port);

    printf("Attempting receive from port.\n");
    do_mach_msg_receive(&msg.head, MACH_MSG_OPTION_NONE,
			sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_RCV_IN_SET);

    check_port_status(port, set, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 2, FALSE, FALSE);
    check_singleton_set(set, port);

    printf("Receiving from set.\n");

    do_simple_receive(&msg.head, sizeof msg, set);
    check_simple_header(&msg.head, sizeof msg,
			msg.head.msgh_remote_port, port, 0);
    check_msg_type_integer(&msg.type);
    if (msg.data != 0)
	quit(1, "%s: enabled_receive_test_1: bad data\n", program);

    check_port_type(msg.head.msgh_remote_port, MACH_PORT_TYPE_SEND_ONCE);

    check_port_status(port, set, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, FALSE);
    check_singleton_set(set, port);

    printf("Deallocating reply port.\n");
    my_mach_port_deallocate(msg.head.msgh_remote_port);

    check_invalid_name(msg.head.msgh_remote_port);
    check_port_status(port, set, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 1, FALSE, FALSE);
    check_singleton_set(set, port);

    printf("Removing port from set.\n");
    my_mach_port_move_member(port, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 1, FALSE, FALSE);
    check_empty_set(set);

    printf("Receiving send-once notification.\n");
    rcv_send_once(port, 1);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_empty_set(set);

    my_mach_port_destroy(port);
    my_mach_port_destroy(set);

    printf("Finished first enabled-receive test.\n");
}

void
enabled_receive_test_2()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	int data;
    } msg;

    mach_port_t port = my_mach_port_allocate_send();
    mach_port_t set = my_mach_port_allocate_set();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_empty_set(set);

    printf("Moving port into set.\n");
    my_mach_port_move_member(port, set);

    check_port_status(port, set, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_singleton_set(set, port);

    fillin_simple_header(&msg.head, port, port);
    fillin_msg_type_integer(&msg.type);
    msg.data = 1;

    printf("Attempting rpc to port, local equals remote.\n");
    do_mach_msg_rpc(&msg.head, MACH_MSG_OPTION_NONE,
		    sizeof msg, sizeof msg, port,
		    MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		    MACH_RCV_IN_SET);

    check_port_status(port, set, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 2, FALSE, FALSE);
    check_singleton_set(set, port);

    printf("Removing port from set.\n");
    my_mach_port_move_member(port, MACH_PORT_NULL);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 2, FALSE, FALSE);
    check_empty_set(set);

    printf("Receiving from port.\n");

    do_simple_receive(&msg.head, sizeof msg, port);
    check_simple_header(&msg.head, sizeof msg,
			msg.head.msgh_remote_port, port, 0);
    check_msg_type_integer(&msg.type);
    if (msg.data != 1)
	quit(1, "%s: enabled_receive_test_2: bad data\n", program);

    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 1, FALSE, FALSE);
    check_empty_set(set);

    my_mach_port_destroy(port);
    my_mach_port_destroy(set);

    check_port_type(msg.head.msgh_remote_port, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(msg.head.msgh_remote_port, MACH_PORT_RIGHT_DEAD_NAME, 1);

    printf("Deallocating dead reply port.\n");
    my_mach_port_deallocate(msg.head.msgh_remote_port);

    printf("Finished second enabled-receive test.\n");
}

void
enabled_receive_test_3()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	int data;
    } msg;

    mach_port_t dest = my_mach_port_allocate_send();
    mach_port_t reply = my_mach_port_allocate_receive();
    mach_port_t set = my_mach_port_allocate_set();

    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_empty_set(set);

    printf("Moving reply port into set.\n");
    my_mach_port_move_member(reply, set);

    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, set, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_singleton_set(set, reply);

    fillin_simple_header(&msg.head, dest, reply);
    fillin_msg_type_integer(&msg.type);
    msg.data = 2;

    printf("Attempting rpc to dest, local not equal to remote.\n");
    do_mach_msg_rpc(&msg.head, MACH_MSG_OPTION_NONE,
		    sizeof msg, sizeof msg, reply,
		    MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		    MACH_RCV_IN_SET);

    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(reply, set, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_singleton_set(set, reply);

    printf("Receiving from dest.\n");

    do_simple_receive(&msg.head, sizeof msg, dest);
    check_simple_header(&msg.head, sizeof msg,
			msg.head.msgh_remote_port, dest, 0);
    check_msg_type_integer(&msg.type);
    if (msg.data != 2)
	quit(1, "%s: enabled_receive_test_3: bad data\n", program);

    check_port_status(dest, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, set, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_singleton_set(set, reply);

    my_mach_port_destroy(dest);
    my_mach_port_destroy(set);

    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    check_port_type(msg.head.msgh_remote_port, MACH_PORT_TYPE_SEND_ONCE);

    printf("Deallocating reply port send-once right.\n");
    my_mach_port_deallocate(msg.head.msgh_remote_port);

    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving send-once notification.\n");
    rcv_send_once(reply, 0);

    check_port_status(reply, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(reply);

    printf("Finished third enabled-receive test.\n");
}

void
rcv_notify_test_1()
{
    mach_msg_header_t msg;

    mach_port_t server = my_mach_port_allocate_send();
    mach_port_t reply = my_mach_port_allocate_receive();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending request message.\n");
    fillin_simple_header(&msg, server, reply);
    do_simple_send(&msg, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving request message with MACH_RCV_NOTIFY.\n");
    do_mach_msg_receive(&msg, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_simple_header(&msg, sizeof msg, msg.msgh_remote_port, server, 0);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(msg.msgh_remote_port,
		    MACH_PORT_TYPE_SEND_ONCE|MACH_PORT_TYPE_DNREQUEST);

    printf("Sending reply message.\n");
    fillin_msg_header(&msg, FALSE, MACH_MSG_TYPE_MOVE_SEND_ONCE, 0,
		      msg.msgh_remote_port, MACH_PORT_NULL,
		      0, 0);
    do_simple_send(&msg, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving port-deleted notification.\n");
    rcv_port_deleted(notify, 0, msg.msgh_remote_port);

    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving reply message.\n");
    do_mach_msg_receive(&msg, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, reply,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_msg_header(&msg, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, reply, 0, 0);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
			 0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(notify);
    check_invalid_name(notify);
    my_mach_port_deallocate_receive(reply);
    check_invalid_name(reply);
    my_mach_port_deallocate_receive(server);
    check_port_type(server, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(server, MACH_PORT_RIGHT_DEAD_NAME, 1);
    my_mach_port_deallocate_dead(server);
    check_invalid_name(server);

    printf("Finished first rcv-notify test.\n");
}

void
rcv_notify_test_2()
{
    mach_msg_header_t msg;

    mach_port_t server = my_mach_port_allocate_send();
    mach_port_t reply = my_mach_port_allocate_receive();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending request message.\n");
    fillin_simple_header(&msg, server, reply);
    do_simple_send(&msg, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving request message with MACH_RCV_NOTIFY.\n");
    do_mach_msg_receive(&msg, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_simple_header(&msg, sizeof msg, msg.msgh_remote_port, server, 0);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(msg.msgh_remote_port,
		    MACH_PORT_TYPE_SEND_ONCE|MACH_PORT_TYPE_DNREQUEST);
    check_port_refs(msg.msgh_remote_port, MACH_PORT_RIGHT_SEND_ONCE, 1);

    printf("Deallocating the reply port.\n");
    my_mach_port_deallocate_receive(reply);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_invalid_name(reply);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);
    check_port_type(msg.msgh_remote_port, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(msg.msgh_remote_port, MACH_PORT_RIGHT_DEAD_NAME, 2);

    printf("Receiving the dead-name notification.\n");
    rcv_dead_name(notify, 0, msg.msgh_remote_port);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Deallocating the dead reply port.\n");
    my_mach_port_mod_refs(msg.msgh_remote_port, MACH_PORT_RIGHT_DEAD_NAME, -2);
    check_invalid_name(msg.msgh_remote_port);

    my_mach_port_deallocate_receive(notify);
    check_invalid_name(notify);
    my_mach_port_deallocate_receive(server);
    check_port_type(server, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(server, MACH_PORT_RIGHT_DEAD_NAME, 1);
    my_mach_port_deallocate_dead(server);
    check_invalid_name(server);

    printf("Finished second rcv-notify test.\n");
}

void
rcv_notify_test_3()
{
    mach_msg_header_t msg;

    mach_port_t server = my_mach_port_allocate_send();
    mach_port_t reply = my_mach_port_allocate_receive();

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending request message.\n");
    fillin_simple_header(&msg, server, reply);
    do_simple_send(&msg, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);

    printf("Trying MACH_RCV_NOTIFY receive, with null notify port.\n");
    do_mach_msg_receive(&msg, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_RCV_INVALID_NOTIFY);
    check_msg_header(&msg, FALSE,
		     MACH_MSG_TYPE_PORT_SEND_ONCE, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, MACH_PORT_NULL, server, 0, 0);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Sending request message.\n");
    fillin_simple_header(&msg, server, reply);
    do_simple_send(&msg, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      2, 0, FALSE, FALSE);

    printf("Trying MACH_RCV_NOTIFY receive, with bad notify port.\n");
    do_mach_msg_receive(&msg, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, mach_task_self(),
			MACH_RCV_INVALID_NOTIFY);
    check_msg_header(&msg, FALSE,
		     MACH_MSG_TYPE_PORT_SEND_ONCE, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, MACH_PORT_NULL, server, 1, 0);

    check_port_status(server, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 2,
		      2, 0, FALSE, FALSE);

    printf("Sending request message.\n");
    fillin_simple_header(&msg, server, reply);
    do_simple_send(&msg, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 2, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 2,
		      3, 0, FALSE, FALSE);

    printf("Trying normal receive, with bad notify port.\n");
    do_mach_msg_receive(&msg, MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, mach_task_self(),
			MACH_MSG_SUCCESS);
    check_simple_header(&msg, sizeof msg, msg.msgh_remote_port, server, 2);

    check_port_status(server, MACH_PORT_NULL, 3, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 2,
		      3, 0, FALSE, FALSE);
    check_port_type(msg.msgh_remote_port, MACH_PORT_TYPE_SEND_ONCE);

    printf("Sending reply message.\n");
    fillin_msg_header(&msg, FALSE, MACH_MSG_TYPE_MOVE_SEND_ONCE, 0,
		      msg.msgh_remote_port, MACH_PORT_NULL,
		      0, 0);
    do_simple_send(&msg, sizeof msg);

    printf("Deallocating the service port.\n");
    my_mach_port_deallocate_receive(server);

    check_port_type(server, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(server, MACH_PORT_RIGHT_DEAD_NAME, 1);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 3,
		      3, 0, FALSE, FALSE);

    printf("Receiving send-once notification.\n");
    rcv_send_once(reply, 0);

    check_port_status(reply, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 2,
		      2, 0, FALSE, FALSE);

    printf("Receiving send-once notification.\n");
    rcv_send_once(reply, 1);

    check_port_status(reply, MACH_PORT_NULL, 2, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving reply message.\n");
    do_mach_msg_receive(&msg, MACH_RCV_TIMEOUT,
			sizeof msg, reply,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_MSG_SUCCESS);
    check_msg_header(&msg, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, reply, 2, 0);

    check_port_status(reply, MACH_PORT_NULL, 3, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_dead(server);
    check_invalid_name(server);
    my_mach_port_deallocate_receive(reply);
    check_invalid_name(reply);

    printf("Finished third rcv-notify test.\n");
}

void
rcv_notify_test_4()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg;

    mach_port_t server = my_mach_port_allocate_receive();
    mach_port_t reply = my_mach_port_allocate_receive();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_status(server, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending request message.\n");
    fillin_msg_header(&msg.head, TRUE,
		      MACH_MSG_TYPE_MAKE_SEND, MACH_MSG_TYPE_MAKE_SEND,
		      server, reply, 0, 0);
    fillin_msg_type(&msg.type, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1,
		    TRUE, FALSE);
    msg.port = reply;
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);
    check_invalid_name(reply);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving request message with MACH_RCV_NOTIFY.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_msg_header(&msg.head, TRUE,
		     MACH_MSG_TYPE_PORT_SEND, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, msg.head.msgh_remote_port, server, 0, 0);
    check_msg_type(&msg.type, MACH_MSG_TYPE_PORT_RECEIVE, 32, 1,
		   TRUE, FALSE);
    reply = msg.head.msgh_remote_port;
    if (msg.port != reply)
	quit(1, "%s: reply port doesn't match receive right\n", program);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(reply,
		    MACH_PORT_TYPE_SEND_RECEIVE|MACH_PORT_TYPE_DNREQUEST);
    my_mach_port_deallocate_receive(server);
    check_invalid_name(server);

    printf("Sending reply message.\n");
    fillin_msg_header(&msg.head, FALSE, MACH_MSG_TYPE_MOVE_SEND, 0,
		      reply, MACH_PORT_NULL, 0, 0);
    do_simple_send(&msg.head, sizeof msg.head);

    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(reply, MACH_PORT_TYPE_RECEIVE|MACH_PORT_TYPE_DNREQUEST);

    printf("Receiving reply message.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, reply,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_msg_header(&msg.head, FALSE, 0, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg.head, MACH_PORT_NULL, reply, 0, 0);

    check_port_status(reply, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(reply, MACH_PORT_TYPE_RECEIVE|MACH_PORT_TYPE_DNREQUEST);

    printf("Deallocating reply port.\n");
    my_mach_port_deallocate_receive(reply);

    check_invalid_name(reply);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving the port-deleted notification.\n");
    rcv_port_deleted(notify, 0, reply);

    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(notify);
    check_invalid_name(notify);

    printf("Finished fourth rcv-notify test.\n");
}

void
rcv_notify_test_5()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg;

    mach_port_t server = my_mach_port_allocate_receive();
    mach_port_t reply = my_mach_port_allocate_receive();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_status(server, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending request message.\n");
    fillin_msg_header(&msg.head, TRUE,
		      MACH_MSG_TYPE_MAKE_SEND, MACH_MSG_TYPE_MAKE_SEND,
		      server, reply, 0, 0);
    fillin_msg_type(&msg.type, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1,
		    TRUE, FALSE);
    msg.port = reply;
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);
    check_invalid_name(reply);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving request message with MACH_RCV_NOTIFY.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_msg_header(&msg.head, TRUE,
		     MACH_MSG_TYPE_PORT_SEND, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, msg.head.msgh_remote_port, server, 0, 0);
    check_msg_type(&msg.type, MACH_MSG_TYPE_PORT_RECEIVE, 32, 1,
		   TRUE, FALSE);
    reply = msg.head.msgh_remote_port;
    if (msg.port != reply)
	quit(1, "%s: reply port doesn't match receive right\n", program);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(reply,
		    MACH_PORT_TYPE_SEND_RECEIVE|MACH_PORT_TYPE_DNREQUEST);
    check_port_refs(reply, MACH_PORT_RIGHT_SEND, 1);
    my_mach_port_deallocate_receive(server);
    check_invalid_name(server);

    printf("Deallocating the reply port.\n");
    my_mach_port_deallocate_receive(reply);

    check_port_type(reply, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(reply, MACH_PORT_RIGHT_DEAD_NAME, 2);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);

    printf("Receiving the dead-name notification.\n");
    rcv_dead_name(notify, 0, reply);

    check_port_type(reply, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(reply, MACH_PORT_RIGHT_DEAD_NAME, 2);
    check_port_status(notify, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Deallocating the dead reply port.\n");
    my_mach_port_mod_refs(reply, MACH_PORT_RIGHT_DEAD_NAME, -2);
    check_invalid_name(reply);

    my_mach_port_deallocate_receive(notify);
    check_invalid_name(notify);

    printf("Finished fifth rcv-notify test.\n");
}

void
send_notify_test()
{
    mach_msg_header_t msg;

    mach_port_t port = my_mach_port_allocate_send();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Setting queue-limit to zero.\n");
    set_qlimit(port, 0);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      0, 0, 0, 1, FALSE, FALSE);

    printf("Trying MACH_SEND_NOTIFY, with null notify port.\n");
    fillin_simple_header(&msg, port, MACH_PORT_NULL);
    do_mach_msg_send(&msg, MACH_SEND_NOTIFY, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		     MACH_SEND_INVALID_NOTIFY);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      0, 0, 0, 1, FALSE, FALSE);

    printf("Trying MACH_SEND_NOTIFY, with bad notify port.\n");
    fillin_simple_header(&msg, port, MACH_PORT_NULL);
    do_mach_msg_send(&msg, MACH_SEND_NOTIFY, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, mach_task_self(),
		     MACH_SEND_INVALID_NOTIFY);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      0, 0, 0, 1, FALSE, FALSE);

    printf("Trying reflexive MACH_SEND_NOTIFY.\n");
    fillin_simple_header(&msg, port, MACH_PORT_NULL);
    do_mach_msg_send(&msg, MACH_SEND_NOTIFY, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, port,
		     MACH_SEND_WILL_NOTIFY);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      0, 1, 1, 2, FALSE, FALSE);

    printf("Receiving test message.\n");
    do_simple_receive(&msg, sizeof msg, port);

    check_simple_header(&msg, sizeof msg, MACH_PORT_NULL, port, 0);
    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      0, 1, 1, 1, FALSE, FALSE);

    printf("Receiving msg-accepted notification.\n");
    rcv_msg_accepted(port, 1, port);

    check_port_status(port, MACH_PORT_NULL, 2, 1,
		      0, 0, 0, 1, FALSE, FALSE);
    my_mach_port_destroy(port);

    printf("Finished send-notify test.\n");
}

void
send_cancel_test_1()
{
    mach_msg_header_t msg;

    mach_port_t port = my_mach_port_allocate_send();

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Trying MACH_SEND_CANCEL, with null notify port.\n");
    fillin_simple_header(&msg, port, MACH_PORT_NULL);
    do_mach_msg_send(&msg, MACH_SEND_CANCEL, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		     MACH_SEND_INVALID_NOTIFY);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Trying MACH_SEND_CANCEL, with bad notify port.\n");
    fillin_simple_header(&msg, port, MACH_PORT_NULL);
    do_mach_msg_send(&msg, MACH_SEND_CANCEL, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, mach_task_self(),
		     MACH_SEND_INVALID_NOTIFY);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    printf("Trying reflexive MACH_SEND_CANCEL.\n");
    fillin_simple_header(&msg, port, MACH_PORT_NULL);
    do_mach_msg_send(&msg, MACH_SEND_CANCEL, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, port,
		     MACH_MSG_SUCCESS);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);

    printf("Receiving test message.\n");
    do_simple_receive(&msg, sizeof msg, port);

    check_simple_header(&msg, sizeof msg, MACH_PORT_NULL, port, 0);
    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    my_mach_port_destroy(port);

    printf("Finished first send-cancel test.\n");
}

void
send_cancel_test_2()
{
    mach_msg_header_t msg;

    mach_port_t server = my_mach_port_allocate_send();
    mach_port_t reply = my_mach_port_allocate_receive();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending request message.\n");
    fillin_simple_header(&msg, server, reply);
    do_simple_send(&msg, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 2, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving request message with MACH_RCV_NOTIFY.\n");
    do_mach_msg_receive(&msg, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_simple_header(&msg, sizeof msg, msg.msgh_remote_port, server, 0);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(msg.msgh_remote_port,
		    MACH_PORT_TYPE_SEND_ONCE|MACH_PORT_TYPE_DNREQUEST);

    printf("Sending reply message with MACH_SEND_CANCEL.\n");
    fillin_msg_header(&msg, FALSE, MACH_MSG_TYPE_MOVE_SEND_ONCE, 0,
		      msg.msgh_remote_port, MACH_PORT_NULL,
		      0, 0);
    do_mach_msg_send(&msg, MACH_SEND_CANCEL, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, notify, MACH_MSG_SUCCESS);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      1, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving reply message.\n");
    do_mach_msg_receive(&msg, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, reply,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_msg_header(&msg, FALSE, 0, MACH_MSG_TYPE_PORT_SEND_ONCE,
		     sizeof msg, MACH_PORT_NULL, reply, 0, 0);

    check_port_status(server, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    my_mach_port_deallocate_receive(notify);
    check_invalid_name(notify);
    my_mach_port_deallocate_receive(reply);
    check_invalid_name(reply);
    my_mach_port_deallocate_receive(server);
    check_port_type(server, MACH_PORT_TYPE_DEAD_NAME);
    check_port_refs(server, MACH_PORT_RIGHT_DEAD_NAME, 1);
    my_mach_port_deallocate_dead(server);
    check_invalid_name(server);

    printf("Finished second send-cancel test.\n");
}

void
send_cancel_test_3()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_t type;
	mach_port_t port;
    } msg;

    mach_port_t server = my_mach_port_allocate_receive();
    mach_port_t reply = my_mach_port_allocate_receive();
    mach_port_t notify = my_mach_port_allocate_receive();

    check_port_status(server, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending request message.\n");
    fillin_msg_header(&msg.head, FALSE,
		      MACH_MSG_TYPE_MAKE_SEND, MACH_MSG_TYPE_MAKE_SEND,
		      server, reply, 0, 0);
    do_simple_send(&msg.head, sizeof msg.head);

    check_port_status(server, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Sending message carrying receive right for reply.\n");
    fillin_msg_header(&msg.head, TRUE,
		      MACH_MSG_TYPE_MAKE_SEND, 0,
		      server, MACH_PORT_NULL, 0, 0);
    fillin_msg_type(&msg.type, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1,
		    TRUE, FALSE);
    msg.port = reply;
    do_simple_send(&msg.head, sizeof msg);

    check_port_status(server, MACH_PORT_NULL, 0, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 2,
		      0, 2, FALSE, FALSE);
    check_invalid_name(reply);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving request message with MACH_RCV_NOTIFY.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_msg_header(&msg.head, FALSE,
		     MACH_MSG_TYPE_PORT_SEND, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg.head, msg.head.msgh_remote_port, server, 0, 0);
    reply = msg.head.msgh_remote_port;

    check_port_status(server, MACH_PORT_NULL, 1, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      1, 0, FALSE, FALSE);
    check_port_type(reply, MACH_PORT_TYPE_SEND|MACH_PORT_TYPE_DNREQUEST);

    printf("Sending reply message with MACH_SEND_CANCEL.\n");
    fillin_msg_header(&msg.head, FALSE, MACH_MSG_TYPE_MOVE_SEND, 0,
		      reply, MACH_PORT_NULL, 0, 0);
    do_mach_msg_send(&msg.head, MACH_SEND_CANCEL, sizeof msg.head,
		     MACH_MSG_TIMEOUT_NONE, notify, MACH_MSG_SUCCESS);

    check_port_status(server, MACH_PORT_NULL, 1, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);
    check_invalid_name(reply);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    printf("Receiving reply right message with MACH_RCV_NOTIFY.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, server,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_msg_header(&msg.head, TRUE, 0, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg, MACH_PORT_NULL, server, 1, 0);
    check_msg_type(&msg.type, MACH_MSG_TYPE_PORT_RECEIVE, 32, 1,
		   TRUE, FALSE);
    reply = msg.port;
    if (!MACH_PORT_VALID(reply))
	quit(1, "%s: received reply port isn't valid\n", program);

    check_port_status(server, MACH_PORT_NULL, 2, 2,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(server);

    printf("Receiving reply message with MACH_RCV_NOTIFY.\n");
    do_mach_msg_receive(&msg.head, MACH_RCV_NOTIFY|MACH_RCV_TIMEOUT,
			sizeof msg, reply,
			MACH_MSG_TIMEOUT_NONE, notify,
			MACH_MSG_SUCCESS);
    check_msg_header(&msg.head, FALSE, 0, MACH_MSG_TYPE_PORT_SEND,
		     sizeof msg.head, MACH_PORT_NULL, reply, 0, 0);

    check_port_status(reply, MACH_PORT_NULL, 1, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_status(notify, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    check_port_type(reply, MACH_PORT_TYPE_RECEIVE);

    my_mach_port_deallocate_receive(reply);
    my_mach_port_deallocate_receive(notify);

    printf("Finished third send-cancel test.\n");
}

void
circular_test_1()
{
    struct
    {
	mach_msg_header_t h;
	mach_msg_type_t t;
	mach_port_t p;
    } msg;

    mach_port_t port;

    port = my_mach_port_allocate_receive();

    fillin_msg_header(&msg.h, TRUE, MACH_MSG_TYPE_MAKE_SEND, 0,
		      port, MACH_PORT_NULL, 0, 0);
    fillin_msg_type(&msg.t, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1,
		    TRUE, FALSE);
    msg.p = port;

    printf("Sending test message to create loop.\n");
    do_mach_msg_send(&msg.h, MACH_MSG_OPTION_NONE,
		     sizeof msg, MACH_MSG_TIMEOUT_NONE,
		     MACH_PORT_NULL, MACH_MSG_SUCCESS);

    printf("Finished first circular test.\n");
}

void
circular_test_2()
{
    struct
    {
	mach_msg_header_t h;
	mach_msg_type_t t;
	mach_port_t p;
    } msg;

    mach_port_t port;

    port = my_mach_port_allocate_receive();
    printf("Allocated port %x.\n", port);

    printf("Setting queue limit to zero.\n");
    set_qlimit(port, 0);

    fillin_msg_header(&msg.h, TRUE, MACH_MSG_TYPE_MAKE_SEND, 0,
		      port, MACH_PORT_NULL, 0, 0);
    fillin_msg_type(&msg.t, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1,
		    TRUE, FALSE);
    msg.p = port;

    printf("Sending test message to create loop.\n");
    do_mach_msg_send(&msg.h, MACH_SEND_TIMEOUT,
		     sizeof msg, 0,
		     MACH_PORT_NULL, MACH_SEND_TIMED_OUT);

    port = msg.h.msgh_remote_port;
    printf("Pseudo-received port %x.\n", port);

    check_msg_header(&msg.h, TRUE, MACH_MSG_TYPE_PORT_SEND, 0,
		     sizeof msg, port, MACH_PORT_NULL, 0, 0);
    check_msg_type(&msg.t, MACH_MSG_TYPE_PORT_RECEIVE, 32, 1,
		   TRUE, FALSE);
    if (msg.p != port)
	quit(1, "%s: check_circular_test_2: bad port\n", program);

    check_port_type(port, MACH_PORT_TYPE_SEND_RECEIVE);
    check_port_refs(port, MACH_PORT_RIGHT_SEND, 1);
    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      0, 0, 0, 1, FALSE, FALSE);

    my_mach_port_destroy(port);
    printf("Finished second circular test.\n");
}

void
circular_test_3()
{
    struct
    {
	mach_msg_header_t h;
	mach_msg_type_t t;
	mach_port_t p;
    } msg;

    mach_port_t base, prev, port;
    int i;

    base = my_mach_port_allocate_send();
    printf("Created base port %x.\n", base);

    printf("Creating a chain of 100 messages...\n");
    for (prev = base, i = 0; i < 100; prev = port, i++)
    {
	port = my_mach_port_allocate_send();
	fillin_msg_header(&msg.h, TRUE, MACH_MSG_TYPE_MOVE_SEND, 0,
			  prev, MACH_PORT_NULL, 0, 0);
	fillin_msg_type(&msg.t, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1,
			TRUE, FALSE);
	msg.p = port;

	do_mach_msg_send(&msg.h, MACH_MSG_OPTION_NONE,
			 sizeof msg, MACH_MSG_TIMEOUT_NONE,
			 MACH_PORT_NULL, MACH_MSG_SUCCESS);
    }
    printf("Finished creating chain of messages.\n");

    fillin_msg_header(&msg.h, TRUE, MACH_MSG_TYPE_MOVE_SEND, 0,
		      port, MACH_PORT_NULL, 0, 0);
    fillin_msg_type(&msg.t, MACH_MSG_TYPE_MOVE_RECEIVE, 32, 1,
		    TRUE, FALSE);
    msg.p = base;

    printf("Sending base port to end of chain.\n");
    do_mach_msg_send(&msg.h, MACH_MSG_OPTION_NONE,
		     sizeof msg, MACH_MSG_TIMEOUT_NONE,
		     MACH_PORT_NULL, MACH_MSG_SUCCESS);

    check_invalid_name(base);
    check_invalid_name(port);

    printf("Finished third circular test.\n");
}

void
circular_test_4()
{
    mach_msg_header_t msg;
    mach_port_t port = my_mach_port_allocate_receive();

    check_port_status(port, MACH_PORT_NULL, 0, 0,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);

    fillin_msg_header_all(&msg, (MACH_MSGH_BITS_CIRCULAR |
				 MACH_MSGH_BITS(MACH_MSG_TYPE_MAKE_SEND, 0)),
			  0, port, MACH_PORT_NULL,
			  0, 0);

    printf("Sending message with circular bit.\n");
    do_simple_send(&msg, sizeof msg);

    check_port_status(port, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 1, FALSE, FALSE);

    printf("Receiving message.\n");
    do_simple_receive(&msg, sizeof msg, port);

    check_simple_header(&msg, sizeof msg, MACH_PORT_NULL, port, 0);
    check_port_status(port, MACH_PORT_NULL, 1, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 0, FALSE, FALSE);
    my_mach_port_deallocate_receive(port);

    printf("Finished fourth circular test.\n");
}

#define	VERY_VERY_LARGE		(128 * 1024 * 1024)

void
no_memory_test_1()
{
    printf("Trying to send 0x%x-byte message.\n", VERY_VERY_LARGE);
    do_mach_msg_send((mach_msg_header_t *) 0, MACH_MSG_OPTION_NONE,
		     VERY_VERY_LARGE, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		     MACH_SEND_NO_BUFFER);

    printf("Finished first no-memory test.\n");
}

void
no_memory_test_2()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_long_t type;
	vm_offset_t data;
    } msg;

    mach_port_t port;

    port = my_mach_port_allocate_send();

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_long(&msg.type, FALSE, TRUE,
			 MACH_MSG_TYPE_INTEGER_32, 32, VERY_VERY_LARGE/4);
    msg.data = my_vm_allocate(VERY_VERY_LARGE);

    printf("Sending message with 0x%x-byte OOL integers.\n", VERY_VERY_LARGE);
    do_simple_send(&msg.head, sizeof msg);

    printf("Receiving message.\n");
    do_simple_receive(&msg.head, sizeof msg, port);

    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_long(&msg.type, FALSE, TRUE,
			MACH_MSG_TYPE_INTEGER_32, 32, VERY_VERY_LARGE/4);

    my_vm_deallocate(msg.data, VERY_VERY_LARGE);
    my_mach_port_destroy(port);

    printf("Finished second no-memory test.\n");
}

void
no_memory_test_3()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_long_t type;
	vm_offset_t data;
    } msg;

    mach_port_t port;

    port = my_mach_port_allocate_send();

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_long(&msg.type, FALSE, TRUE,
			 MACH_MSG_TYPE_COPY_SEND, 32, VERY_VERY_LARGE/4);
    msg.data = my_vm_allocate(VERY_VERY_LARGE);

    printf("Trying to send message with 0x%x-byte OOL ports.\n",
	   VERY_VERY_LARGE);
    do_mach_msg_send(&msg.head, MACH_MSG_OPTION_NONE, sizeof msg,
		     MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
		     MACH_SEND_INVALID_MEMORY);

    my_vm_deallocate(msg.data, VERY_VERY_LARGE);
    my_mach_port_destroy(port);

    printf("Finished third no-memory test.\n");
}

void
no_space_test_1()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_long_t type;
	vm_offset_t data;
    } msg;

    mach_port_t port;

    port = my_mach_port_allocate_send();

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_page_dealloc(&msg.type);
    msg.data = my_vm_allocate_page();

    printf("Sending message with OOL page of bytes.\n");
    do_simple_send(&msg.head, sizeof msg);

    printf("Allocating address space.\n");
    alloc_address_space();

    printf("Receiving message.\n");
    do_mach_msg_receive(&msg.head, MACH_MSG_OPTION_NONE,
			sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_RCV_BODY_ERROR|MACH_MSG_VM_SPACE);

    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_long(&msg.type, FALSE, TRUE,
			MACH_MSG_TYPE_BYTE, 0, vm_page_size);
    if (msg.data != 0)
	quit(1, "%s: no_space_test_1: data = 0x%x\n", program, msg.data);

    printf("Deallocating address space.\n");
    dealloc_address_space();
    my_mach_port_destroy(port);

    printf("Finished first no-space test.\n");
}

void
no_space_test_2()
{
    struct
    {
	mach_msg_header_t head;
	mach_msg_type_long_t type;
	mach_port_t *data;
    } msg;

    mach_port_t port, carried;

    port = my_mach_port_allocate_send();
    carried = my_mach_port_allocate_send();

    fillin_complex_header(&msg.head, port, MACH_PORT_NULL);
    fillin_msg_type_long(&msg.type, FALSE, TRUE,
			 MACH_MSG_TYPE_MOVE_RECEIVE, 32, vm_page_size/4);
    msg.data = (mach_port_t *) my_vm_allocate_page();
    msg.data[0] = carried;

    printf("Sending message with OOL page of ports.\n");
    do_simple_send(&msg.head, sizeof msg);

    check_port_type(carried, MACH_PORT_TYPE_SEND);

    printf("Allocating address space.\n");
    alloc_address_space();

    printf("Receiving message.\n");
    do_mach_msg_receive(&msg.head, MACH_MSG_OPTION_NONE,
			sizeof msg, port,
			MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
			MACH_RCV_BODY_ERROR|MACH_MSG_VM_SPACE);

    check_complex_header(&msg.head, sizeof msg, MACH_PORT_NULL, port, 0);
    check_msg_type_long(&msg.type, FALSE, TRUE,
			MACH_MSG_TYPE_PORT_RECEIVE, 0, vm_page_size/4);
    if (msg.data != 0)
	quit(1, "%s: no_space_test_2: data = 0x%x\n", program, msg.data);
    check_port_type(carried, MACH_PORT_TYPE_DEAD_NAME);

    printf("Deallocating address space.\n");
    dealloc_address_space();
    my_mach_port_destroy(port);
    my_mach_port_destroy(carried);

    printf("Finished second no-space test.\n");
}

void
header_ports_test()
{
    mach_msg_header_t msg;
    mach_port_t dest, reply;

    dest = my_mach_port_allocate_send();
    reply = my_mach_port_allocate_send();

    fillin_msg_header(&msg, FALSE,
		      MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_COPY_SEND,
		      dest, dest, 0, 0);

    printf("Trying copy-send/copy-send (dest==reply) test.\n");
    do_simple_send(&msg, sizeof msg);

    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 1,
		      0, 3, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 1, FALSE, FALSE);

    fillin_msg_header(&msg, FALSE,
		      MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_COPY_SEND,
		      dest, reply, 0, 0);

    printf("Trying copy-send/copy-send (dest!=reply) test.\n");
    do_simple_send(&msg, sizeof msg);

    check_port_status(dest, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 2,
		      0, 4, FALSE, FALSE);
    check_port_status(reply, MACH_PORT_NULL, 0, 1,
		      MACH_PORT_QLIMIT_DEFAULT, 0,
		      0, 2, FALSE, FALSE);

    my_mach_port_destroy(dest);
    my_mach_port_destroy(reply);

    printf("Finished header-ports test.\n");
}
