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
 * $Log:	utils.h,v $
 * Revision 2.4  91/08/29  15:47:12  rpd
 * 	Changed check_receive_status to check_port_status.
 * 	[91/08/11            rpd]
 * 
 * Revision 2.3  91/03/19  12:18:39  mrt
 * 	Changed to new copyright
 * 
 * Revision 2.2  90/09/12  16:31:15  rpd
 * 	First check-in.
 * 	[90/09/11            rpd]
 * 
 */

#ifndef	_UTILS_H_
#define	_UTILS_H_

#include <mach/port.h>
#include <mach/message.h>
#include <mach/machine/vm_types.h>

extern mach_msg_type_name_t copyin_type();

extern void fillin_msg_header();
extern void fillin_msg_header_all();
extern void check_msg_header();
extern void fillin_msg_type();
extern void fillin_msg_type_all();
extern void check_msg_type();
extern void fillin_msg_type_long();
extern void fillin_msg_type_long_all();
extern void check_msg_type_long();
extern void fillin_simple_header();
extern void check_simple_header();
extern void fillin_complex_header();
extern void check_complex_header();
extern void fillin_msg_type_integer();
extern void check_msg_type_integer();
extern void fillin_msg_type_dummy_pointer();
extern void check_msg_type_dummy_pointer();
extern void fillin_msg_type_dummy_ports();
extern void check_msg_type_dummy_ports();
extern void fillin_msg_type_port_name();
extern void check_msg_type_port_name();
extern void fillin_msg_type_page_dealloc();
extern void check_msg_type_page();

extern void do_mach_msg_send();
extern void do_mach_msg_receive();
extern void do_mach_msg_rpc();
extern void do_simple_send();
extern void do_simple_receive();
extern void send_simple_msg();
extern void rcv_simple_msg();
extern void rcv_mig_reply();

extern void my_mach_port_acquire_send();
extern void my_mach_port_allocate_name();
extern void my_mach_port_allocate_name_receive();
extern void my_mach_port_allocate_name_send();
extern void my_mach_port_allocate_name_set();
extern mach_port_t my_mach_port_allocate();
extern mach_port_t my_mach_port_allocate_receive();
extern mach_port_t my_mach_port_allocate_send();
extern mach_port_t my_mach_port_allocate_set();
extern mach_port_t my_mach_port_allocate_dead();
extern void my_mach_port_destroy();
extern void my_mach_port_deallocate();
extern void do_mach_port_mod_refs();
extern void my_mach_port_mod_refs();
extern void my_mach_port_deallocate_receive();
extern void my_mach_port_deallocate_dead();
extern void my_mach_port_deallocate_set();
extern void my_mach_port_deallocate_send();
extern void do_mach_port_move_member();
extern void my_mach_port_move_member();
extern void do_mach_port_request_notification();
extern void my_mach_port_request_notification();
extern void do_mach_port_rename();
extern void my_mach_port_rename();
extern void do_mach_port_insert_right();
extern void my_mach_port_insert_right();
extern mach_port_t my_mach_port_extract_right();
extern void send_mach_port_extract_right();
extern mach_port_t rcv_mach_port_extract_right();
extern mach_port_t rpc_mach_port_extract_right();
extern void rpc_mach_port_rename();
extern void set_mscount();
extern void set_qlimit();
extern void set_backup();
extern void set_dnrequest();
extern void check_port_type();
extern void check_port_status();
extern void check_port_refs();
extern void check_empty_set();
extern void check_singleton_set();
extern void check_invalid_name();
extern mach_port_t get_invalid_name();

extern void rcv_send_once();
extern void rcv_msg_accepted();
extern void rcv_port_deleted();
extern void rcv_dead_name();
extern void rcv_no_senders();
extern void rcv_port_destroyed();

extern vm_offset_t my_vm_allocate();
extern void my_vm_deallocate();
extern vm_offset_t my_vm_allocate_page();
extern void my_vm_deallocate_page();
extern void check_valid_page();
extern void check_invalid_page();

struct vm_region
{
    vm_offset_t offset;
    vm_size_t size;
};

extern struct vm_region vm_regions[];

extern void alloc_address_space();
extern void dealloc_address_space();

#endif	_UTILS_H_
