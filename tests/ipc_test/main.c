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
 * $Log:	main.c,v $
 * Revision 2.4  91/03/19  12:17:00  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.3  91/03/10  13:40:23  rpd
 * 	Added header_ports_test.
 * 	[91/01/26            rpd]
 * 
 * Revision 2.2  90/09/12  16:29:55  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#include <stdio.h>
#include <string.h>
#define EXPORT_BOOLEAN
#include <mach/boolean.h>

#include "main.h"
#include "mtest.h"
#include "ntest.h"
#include "ptest.h"
#include "utest.h"
#include "nstest.h"
#include "stest.h"

#include <nux/syscalls.h>
void
putchar (int c)
{
  (void) syscall1 (4096, c);
}

void
exit (int status)
{
  syscall1 (4097, status);
}

#define streql(a, b)	(strcmp((a), (b)) == 0)

char *program = NULL;

boolean_t MDebug = FALSE;
boolean_t Verbose = FALSE;

static void mtest();
static void ptest();
static void ntest();
static void utest();
static void nstest();

#if 0
static void
usage()
{
    quit(1, "usage: %s [{+,-}mdebug]\n", program);
}
#endif

void
main(argc, argv)
    int argc;
    char *argv[];
{
#if 0
    int i;

    program = rindex(argv[0], '/');
    if (program != NULL)
	program++;
    else
	program = argv[0];

    for (i = 1; i < argc; i++)
	if (streql(argv[i], "-v"))
	    Verbose = TRUE;
	else if (streql(argv[i], "-V"))
	    Verbose = FALSE;
	else if (streql(argv[i], "-mdebug"))
	    MDebug = FALSE;
	else if (streql(argv[i], "+mdebug"))
	    MDebug = TRUE;
	else if (streql(argv[i], "--"))
	{
	    i++;
	    break;
	}
	else if ((argv[i][0] == '-') || (argv[i][0] == '+'))
	    usage();
	else
	    break;

    if (streql(program, "stest")) {
	int seed = 1;
	unsigned int iterations = 0;

	switch (argc - i) {
	  case 2:
	    seed = atoi(argv[i+1]);
	  case 1:
	    iterations = (unsigned int) atoi(argv[i]);
	  case 0:
	    break;

	  default:
	    usage();
	}

	splay_test(seed, iterations);
    } else {
	if (i != argc)
	    usage();

	if (streql(program, "mtest"))
	    mtest();
	else if (streql(program, "ntest"))
	    ntest();
	else if (streql(program, "ptest"))
	    ptest();
	else if (streql(program, "utest"))
	    utest();
	else if (streql(program, "nstest"))
	    nstest();
	else {
#endif
	    mtest();
	    printf("\n\n");
	    ntest();
	    printf("\n\n");
	    ptest();
	    printf("\n\n");
	    utest();
	    printf("\n\n");
	    nstest();
	    printf("\n\n");
	    splay_test(1, 8192);
#if 0
	}
    }

    exit(0);
#endif
}

static void
mtest()
{
    complex_bit_test_1();
    printf("\n");
    complex_bit_test_2();
    printf("\n");
    complex_bit_test_3();
    printf("\n");
    bad_pointer_test();
    printf("\n");
    zero_pointer_test();
    printf("\n");
    bad_port_test();
    printf("\n");
    deallocate_bit_test_1();
    printf("\n");
    deallocate_bit_test_2();
    printf("\n");
    deallocate_bit_test_3();
    printf("\n");
    too_large_test_1();
    printf("\n");
    too_large_test_2();
    printf("\n");
    null_dest_test();
    printf("\n");
    msg_queue_test();
    printf("\n");
    null_port_test();
    printf("\n");
    dead_port_test();
    printf("\n");
    add_member_test();
    printf("\n");
    remove_member_test();
    printf("\n");
    enabled_receive_test_1();
    printf("\n");
    enabled_receive_test_2();
    printf("\n");
    enabled_receive_test_3();
    printf("\n");
    rcv_notify_test_1();
    printf("\n");
    rcv_notify_test_2();
    printf("\n");
    rcv_notify_test_3();
    printf("\n");
    rcv_notify_test_4();
    printf("\n");
    rcv_notify_test_5();
    printf("\n");
    send_notify_test();
    printf("\n");
    send_cancel_test_1();
    printf("\n");
    send_cancel_test_2();
    printf("\n");
    send_cancel_test_3();
    printf("\n");
    circular_test_1();
    printf("\n");
    circular_test_2();
    printf("\n");
    circular_test_3();
    printf("\n");
    circular_test_4();
    printf("\n");
    no_memory_test_1();
    printf("\n");
    no_memory_test_2();
    printf("\n");
    no_memory_test_3();
    printf("\n");
    no_space_test_1();
    printf("\n");
    no_space_test_2();
    printf("\n");
    header_ports_test();
}

static void
ntest()
{
    msg_accepted_test_1();
    printf("\n");
    msg_accepted_test_2();
    printf("\n");
    msg_accepted_test_3();
    printf("\n");
    msg_accepted_test_4();
    printf("\n");
    port_destroyed_test_1();
    printf("\n");
    port_destroyed_test_2();
    printf("\n");
    port_destroyed_test_3();
    printf("\n");
    port_deleted_test_1();
    printf("\n");
    port_deleted_test_2();
    printf("\n");
    port_deleted_test_3();
    printf("\n");
    port_deleted_test_4();
    printf("\n");
    port_deleted_test_5();
    printf("\n");
    port_deleted_test_6();
}

static void
ptest()
{
    set_test();
    printf("\n");
    port_type_test();
    printf("\n");
    rename_test_1();
    printf("\n");
    rename_test_2();
    printf("\n");
    rename_test_3();
    printf("\n");
    rename_test_4();
    printf("\n");
    extract_receive_test_1();
    printf("\n");
    extract_receive_test_2();
}

static void
utest()
{
    uref_overflow_test_1();
    printf("\n");
    uref_overflow_test_2();
    printf("\n");
    uref_overflow_test_3();
    printf("\n");
    uref_overflow_test_4();
    printf("\n");
    uref_overflow_test_5();
    printf("\n");
    uref_overflow_test_6();
    printf("\n");
    uref_underflow_test();
}

static void
nstest()
{
    no_senders_test_1();
    printf("\n");
    no_senders_test_2();
    printf("\n");
    no_senders_test_3();
    printf("\n");
    no_senders_test_4();
    printf("\n");
    no_senders_test_5();
    printf("\n");
    no_senders_test_6();
    printf("\n");
    no_senders_test_7();
}
