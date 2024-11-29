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
 * $Log:	ntest.h,v $
 * Revision 2.3  91/03/19  12:17:47  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.2  90/09/12  16:30:41  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#ifndef	_NTEST_H_
#define _NTEST_H_

extern void msg_accepted_test_1();
extern void msg_accepted_test_2();
extern void msg_accepted_test_3();
extern void msg_accepted_test_4();

extern void port_destroyed_test_1();
extern void port_destroyed_test_2();
extern void port_destroyed_test_3();

extern void port_deleted_test_1();
extern void port_deleted_test_2();
extern void port_deleted_test_3();
extern void port_deleted_test_4();
extern void port_deleted_test_5();
extern void port_deleted_test_6();

#endif	_NTEST_H_
