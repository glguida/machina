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
 * $Log:	mtest.h,v $
 * Revision 2.4  91/03/19  12:17:22  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.3  91/03/10  13:40:50  rpd
 * 	Added header_ports_test.
 * 	[91/01/26            rpd]
 * 
 * Revision 2.2  90/09/12  16:30:16  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#ifndef _MTEST_H_
#define _MTEST_H_

extern void complex_bit_test_1();
extern void complex_bit_test_2();
extern void complex_bit_test_3();

extern void bad_pointer_test();
extern void zero_pointer_test();
extern void bad_port_test();

extern void deallocate_bit_test_1();
extern void deallocate_bit_test_2();
extern void deallocate_bit_test_3();

extern void too_large_test_1();
extern void too_large_test_2();

extern void null_dest_test();
extern void msg_queue_test();
extern void null_port_test();
extern void dead_port_test();

extern void add_member_test();
extern void remove_member_test();

extern void enabled_receive_test_1();
extern void enabled_receive_test_2();
extern void enabled_receive_test_3();

extern void rcv_notify_test_1();
extern void rcv_notify_test_2();
extern void rcv_notify_test_3();
extern void rcv_notify_test_4();
extern void rcv_notify_test_5();

extern void send_notify_test();

extern void send_cancel_test_1();
extern void send_cancel_test_2();
extern void send_cancel_test_3();

extern void circular_test_1();
extern void circular_test_2();
extern void circular_test_3();
extern void circular_test_4();

extern void no_memory_test_1();
extern void no_memory_test_2();
extern void no_memory_test_3();

extern void no_space_test_1();
extern void no_space_test_2();

extern void header_ports_test();

#endif	_MTEST_H_
